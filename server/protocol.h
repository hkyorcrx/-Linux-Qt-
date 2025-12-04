#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <cstdint>
#include <cstring>

// 消息类型
enum MsgType
{
    MSG_TEXT = 1,           // 文本消息 (带名字、时间)
    MSG_FILE_META = 2,      // 上传文件元信息
    MSG_FILE_DATA = 3,      // 文件数据块
    MSG_FILE_NOTIFY = 4,    // 群文件通知
    MSG_DOWNLOAD_REQ = 5,   // 请求下载
    MSG_DOWNLOAD_START = 6, // 准备开始下载
    MSG_CANCEL_DOWNLOAD = 7// 取消下载
    
};

// 基础包头 
struct PacketHeader
{
    uint32_t type;
    uint32_t length;
};

// 文本消息结构
struct TextBody
{
    int64_t timestamp; // 时间戳
    char sender[32];   // 发送者名字
    // 后面紧跟实际文本内容...
};

// 文件元信息
struct FileMeta
{
    uint64_t fileSize;
    char fileName[128];
    char uploader[32]; 
};

#endif // PROTOCOL_H
