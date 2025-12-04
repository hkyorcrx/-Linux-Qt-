#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <iostream>
#include <sys/wait.h>
#include <sys/stat.h>

#include <sys/epoll.h>
#include <time.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>

#include <thread>
#include <chrono>   // 时间
#include <dirent.h> // 目录遍历

#include "protocol.h"

#define PORT 9090
#define BUF_SIZE 4096
#define MAX_EVENTS 128
#define FILE_DIR "./temp/"
#define CLEAN_INTERVAL_HOURS 1         // 检查间隔：1小时
#define EXPIRE_SECONDS (7 * 24 * 3600) // 过期时间：7天

const int LISTENQ = 128;

struct Clientcontext
{
    int fd;
    std::vector<char> buffer; // 接收缓冲区 (处理粘包)

    // --- 上传状态 (接收客户端文件) ---
    std::ofstream uploadOfs;
    std::string uploadingName;
    uint64_t uploadRemaining;
    std::string uploadUser; // 上传者

        // --- 下载状态 (发送文件给客户端) ---
    std::ifstream downloadIfs; // 读取流
    bool isSendingFile;        // 是否正在发送

    Clientcontext(int f) : fd(f), uploadRemaining(0), isSendingFile(false) {}
};


// 存储客户端连接
std::map<int, Clientcontext *> clients;
// 简单的文件存储：FileID -> Filename
std::map<int, std::string> file_storage;
int g_file_id_counter = 0;

int epoll_fd = 0;

// 修改 epoll 事件 (用于开启/关闭 EPOLLOUT)
void modEpollEvent(int fd, uint32_t events)
{
    struct epoll_event ev;
    ev.events = events | EPOLLET; // 保持边缘触发
    ev.data.fd = fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

void sendPacket(int fd, uint32_t type, const void *data, uint32_t len)
{
    PacketHeader header;
    header.type = type;
    header.length = len;

    // 简单发送逻辑。生产环境应处理 send 返回 EAGAIN 的情况。
    // 这里为了聚焦逻辑，假设控制信令较小，能直接发出。
    send(fd, &header, sizeof(header), 0);
    if (len > 0 && data)
        send(fd, data, len, 0);
}

void handleWrite(int fd)
{
    Clientcontext *ctx = clients[fd];
    if (!ctx || !ctx->isSendingFile)
    {
        modEpollEvent(fd, EPOLLIN);
        return;
    }

    char fileBuf[BUF_SIZE];
    if (!ctx->downloadIfs.eof())
    {
        ctx->downloadIfs.read(fileBuf, sizeof(fileBuf));
        int readLen = ctx->downloadIfs.gcount();
        if (readLen > 0)
        {
            PacketHeader header;
            header.type = MSG_FILE_DATA;
            header.length = readLen;
            send(fd, &header, sizeof(header), 0);
            send(fd, fileBuf, readLen, 0);
        }
    }

    if (ctx->downloadIfs.eof() || !ctx->downloadIfs.good())
    {
        ctx->downloadIfs.close();
        ctx->isSendingFile = false;
        modEpollEvent(fd, EPOLLIN);
    }
}

void broadcastFileNotify(int senderFd, const char *filename, uint64_t size, const char *uploader)
{
    FileMeta meta;
    memset(&meta, 0, sizeof(meta));
    meta.fileSize = size;
    strncpy(meta.fileName, filename, 127);
    strncpy(meta.uploader, uploader, 31);

    for(auto &pair : clients)
    {
        if(pair.first != senderFd)
        {
            sendPacket(pair.first, MSG_FILE_NOTIFY, &meta, sizeof(meta));
        }
    }
}


// 处理完整的应用层数据包
void processPacket(Clientcontext *ctx, PacketHeader *header, const char *body)
{
    if(header->type == MSG_TEXT)
    {
        if(header->length < sizeof(TextBody))
            return;
        TextBody *tb = (TextBody *)body;
        std::string content(body + sizeof(TextBody), header->length - sizeof(TextBody));
        std::cout << "[" << tb->sender << "]: " << content << std::endl;
        // 广播给其他客户端
        for(auto &pair : clients)
        {
            if(pair.first != ctx->fd){
                sendPacket(pair.first, MSG_TEXT, body, header->length);
            }
        }
    }
    else if(header->type == MSG_FILE_META)
    {
        FileMeta *meta = (FileMeta *)body;
        std::string path = std::string(FILE_DIR) + meta->fileName;

        ctx->uploadOfs.open(path, std::ios::binary);
        ctx->uploadingName = meta->fileName;
        ctx->uploadRemaining = meta->fileSize;
        ctx->uploadUser = meta->uploader;
        std::cout << "[Upload] Receiving file: " << meta->fileName << " by " << meta->uploader << std::endl;
    }
    else if(header->type == MSG_FILE_DATA)
    {
        if(ctx->uploadOfs.is_open())
        {
            ctx->uploadOfs.write(body, header->length);
            ctx->uploadRemaining -= header->length;
            if(ctx->uploadRemaining <= 0)
            {
                ctx->uploadOfs.close();
                // 广播通知
                struct stat st;
                std::string path = std::string(FILE_DIR) + ctx->uploadingName;
                stat(path.c_str(), &st);
                broadcastFileNotify(ctx->fd, ctx->uploadingName.c_str(), st.st_size, ctx->uploadUser.c_str());
                std::cout << "[Upload] File received: " << ctx->uploadingName << std::endl;
            }
        }
    }
    else if (header->type == MSG_DOWNLOAD_REQ)
    {
        // 开始下载
        std::string filename(body,header->length);
        std::string path = std::string(FILE_DIR) + filename;

        ctx->downloadIfs.open(path, std::ios::binary | std::ios::ate);
        if (ctx->downloadIfs.is_open())
        {
            uint64_t size = ctx->downloadIfs.tellg();
            ctx->downloadIfs.seekg(0, std::ios::beg);

            // 1.发送头部通知
            FileMeta meta;
            meta.fileSize = size;
            strncpy(meta.fileName, filename.c_str(), 127);
            sendPacket(ctx->fd, MSG_DOWNLOAD_START, &meta, sizeof(meta));

            // 2.标记真正发送，开启EPOLLOUT监听
            ctx->isSendingFile = true;
            modEpollEvent(ctx->fd, EPOLLIN | EPOLLOUT);
            std::cout << "[Download] Start sending: " << filename << std::endl;
        }
    }
    else if(header->type == MSG_CANCEL_DOWNLOAD)
    {
        // 取消下载
        if(ctx->isSendingFile)
        {
            ctx->downloadIfs.close();
            ctx->isSendingFile = false;
            modEpollEvent(ctx->fd, EPOLLIN); // 关闭写事件监听
            std::cout << "[Download] Canceled sending to " << ctx->fd << std::endl;
        }
    }
}

void handleRead(int fd)
{
    Clientcontext *ctx = clients[fd];
    char buf[4096];
    bool isError = false;

    // 1. 循环读取：清空内核 Socket 接收缓冲区
    while (true)
    {
        int n = recv(fd, buf, sizeof(buf), 0);
        if (n > 0)
        {
            // 读到了数据，加入应用层缓冲区
            ctx->buffer.insert(ctx->buffer.end(), buf, buf + n);
        }
        else if (n == 0)
        {
            // 客户端关闭连接
            isError = true;
            break;
        }
        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 读完了
                break;
            }
            else if(errno == EINTR)
            {
                // 被信号中断，继续读
                continue;
            }
            else
            {
                // 其他错误
                isError = true;
                break;
            }
        }
    }

    if (isError)
    {
        // 清理资源
        close(fd);
        delete ctx;
        clients.erase(fd);
        std::cout << "Client disconnected: " << fd << "(total:" << clients.size() << ")" << std::endl;
        return;
    }

    // 2. 处理应用层协议
    while (ctx->buffer.size() >= sizeof(PacketHeader))
    {
        PacketHeader *header = (PacketHeader *)ctx->buffer.data();
        if (header->length > 10 * 1024 * 1024)
        { // 限制单包最大 10MB
            // 认为是恶意攻击，直接断开连接
            close(fd);
            return;
        }
        uint32_t total_len = sizeof(PacketHeader) + header->length;
        // 检查是否有完整包体
        if (ctx->buffer.size() < total_len)
            break;
        const char *bodyptr = ctx->buffer.data() + sizeof(PacketHeader);
        // 处理完整的应用层数据包
        if(header->length>0){
            bodyptr=ctx->buffer.data()+sizeof(PacketHeader);
        }
        processPacket(ctx, header, bodyptr);
        ctx->buffer.erase(ctx->buffer.begin(), ctx->buffer.begin() + total_len);
    }
}
int setSocketNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1)
        return -1;

    flag |= O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flag) == -1)
        return -1;
    return 0;
}

void handle_for_sigpipe()
{
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = SIG_IGN;
    sa.sa_flags = 0;
    if (sigaction(SIGPIPE, &sa, NULL))
        return;
}
void fileCleanerTask()
{
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::hours(CLEAN_INTERVAL_HOURS));
        std::cout << "[Cleaner] Start scanning for expired files..." << std::endl;
        std::cout << "[Cleaner] Start scanning for expired files..." << std::endl;

        DIR *dir = opendir(FILE_DIR);
        if (!dir)
        {
            std::cerr << "[Cleaner] Error opening dir." << std::endl;
            continue;
        }

        struct dirent *entry;
        time_t now = time(nullptr);
        while ((entry = readdir(dir)) != nullptr)
        {
            // 跳过 . 和 ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            std::string path = std::string(FILE_DIR) + entry->d_name;
            struct stat st;

            if (stat(path.c_str(), &st) == 0)
            {
                // 计算时间差
                double diff = difftime(now, st.st_mtime);
                if (diff > EXPIRE_SECONDS)
                {
                    // 删除文件
                    if (unlink(path.c_str()) == 0)
                    {
                        std::cout << "[Cleaner] Removed expired file: " << entry->d_name << std::endl;
                    }
                    else
                    {
                        std::cerr << "[Cleaner] Failed to remove: " << entry->d_name << std::endl;
                    }
                }
            }
        }
        closedir(dir);
        std::cout << "[Cleaner] Scan finished." << std::endl;
    }
}

int main()
{
    handle_for_sigpipe();

    // 【新增】启动清理线程，并分离它让其后台运行
    std::thread cleaner(fileCleanerTask);
    cleaner.detach();

    int listenfd,clnt_sock;
    struct sockaddr_in serv_addr;

    listenfd = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    int optval =1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        printf("setsockopt error %d %s\n", errno, strerror(errno));
        close(listenfd);
        return 0;
    }

    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        printf("bind error %d %s\n", errno, strerror(errno));
        close(listenfd);
        return 0;
    }
    if (listen(listenfd, LISTENQ) == -1)
    {
        printf("listen error %d %s\n", errno, strerror(errno));
        close(listenfd);
        return 0;
    }
    if (setSocketNonBlocking(listenfd) < 0)
    {
        perror("set socket non block failed");
        return 1;
    }

    // epoll复用
    epoll_fd = epoll_create(128);
    if (epoll_fd == -1)
    {
        printf("epoll_create error %d %s\n", errno, strerror(errno));
        return -1;
    }
    int event_cnt = 0;
    epoll_event event;
    epoll_event* all_events = new epoll_event[MAX_EVENTS];
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = listenfd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listenfd, &event);
    std::cout << "Server is listening on port " << PORT <<"..."<< std::endl;

    while(true)
    {
        event_cnt = epoll_wait(epoll_fd, all_events, 100, -1);
        printf("epoll_wait return event count: %d\n", event_cnt);
        if(event_cnt == -1){
            printf("epoll_wait error %d %s\n", errno, strerror(errno));
            break;
        }
        if(event_cnt == 0) continue;
        for(int i=0;i<event_cnt;i++){
            if(all_events[i].data.fd == listenfd){
                struct sockaddr_in clnt_addr;
                memset(&clnt_addr,0,sizeof(struct sockaddr_in));
                socklen_t clnt_size = sizeof(clnt_addr);
                while (true)
                {
                    clnt_sock = accept(listenfd, (struct sockaddr *)&clnt_addr, &clnt_size);
                    if (clnt_sock == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            // 所有连接处理完毕
                            break;
                        }
                        else
                        {
                            perror("accept error");
                            break;
                        }
                    }

                    setSocketNonBlocking(clnt_sock);
                    event.events = EPOLLIN | EPOLLET;
                    event.data.fd = clnt_sock;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clnt_sock, &event);
                    clients[clnt_sock] = new Clientcontext(clnt_sock);
                    std::cout << "New client: " << clnt_sock << " (Total: " << clients.size() << ")" << std::endl;
                }
            }
            else {
                if(all_events[i].events & EPOLLIN){
                    int sockfd = all_events[i].data.fd;
                    // 处理客户端数据读取
                    handleRead(sockfd);
                }
                if (all_events[i].events & EPOLLOUT)
                {
                    int sockfd = all_events[i].data.fd;
                    // 只有在发送文件时才会触发这里
                    handleWrite(sockfd);
                }
            }
        }
    }

    close(listenfd);
    close(epoll_fd);

    return 0;
}
