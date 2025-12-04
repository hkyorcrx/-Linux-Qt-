#include "client.h"

#include <QDebug>
#include <QNetworkProxy>
#include <QDir>
#include <QDataStream>
#include <QString>

Client::Client(QObject *parent)
    : QObject(parent)
{
    // 初始化套接字
    m_socket = new QTcpSocket(this);
    m_ip="192.168.88.132";
    m_port=9090;

    m_socket->setProxy(QNetworkProxy::NoProxy);

    // 绑定信号与内部槽函数
    // 连接服务器成功，会发射connected信号
    connect(m_socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(m_socket,&QTcpSocket::errorOccurred,this,&Client::onErrorOccurred);

}

Client::Client(QString ip, quint16 port, QObject *parent) : QObject(parent)
{
    m_socket = new QTcpSocket(this); // 必须初始化socket！
    m_ip = ip;
    m_port = port;

    m_socket->setProxy(QNetworkProxy::NoProxy);

    // 绑定信号（和默认构造一致）
    connect(m_socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(m_socket,&QTcpSocket::errorOccurred,this,&Client::onErrorOccurred);

}

Client::~Client()
{
    if(m_socket== nullptr){
        return;
    }else{
        m_socket->close();
    }
}



void Client::connectToServer(const QString &ip, quint16 port)
{
    if(isConnected()){
        // disconnectFromServer();
    }
    // 发起连接(异步,不会阻塞界面）
    m_socket->connectToHost(QHostAddress(ip),port);
}

void Client::sendData(const QByteArray &data)
{
    if(m_socket==nullptr){
        qDebug() << "[Client] 发送失败：套接字未初始化";
        emit sig_error("套接字未初始化，发送失败"); // 对外抛错误信号
        return;
    }
    if (!isConnected()) {
        qDebug() << "[Client] 发送失败：未连接到服务器";
        emit sig_error("未连接到服务器，无法发送数据");
        return;
    }
    if(data.isEmpty()){
        qDebug()<<"数据为空，无法发送";
        return;
    }
    qint64 writeLen = m_socket->write(data);
    if(writeLen == -1){
        qDebug()<<"[client]数据发生失败："<<m_socket->errorString();
    }
    else{
        m_socket->flush();
        qDebug()<<"[client]二进制数据发送成功 | 长度："<<writeLen;
    }

}

bool Client::isConnected() const
{
    return m_socket!=nullptr && (m_socket->state() == QAbstractSocket::ConnectedState);
}


void Client::onConnected()
{
    qDebug()<<"服务器连接成功";
}

void Client::onDisconnected()
{

}

void Client::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while(m_buffer.size()>=(int)sizeof(PacketHeader))
    {
        PacketHeader* header = (PacketHeader*)m_buffer.data();
        uint32_t totalLen = sizeof(PacketHeader)+header->length;
        if(m_buffer.size()<(int)totalLen){
            // 数据不够，跳出循环，等待下一次 readyRead 信号
            return;
        }
        // 数据完整，提取包体数据
        QByteArray body = m_buffer.mid(sizeof(PacketHeader),header->length);
        switch(header->type){
        //处理文本消息
        case MSG_TEXT:
            if(body.size()>=(int)sizeof(TextBody)){
                // TextBody* tb=(TextBody*)body.data();
                TextBody* tb;
                memcpy(&tb, body.data(), sizeof(TextBody)); // 避免对齐崩溃

                QString content = QString::fromUtf8(body.mid(sizeof(TextBody)));
                QString sender = QString::fromUtf8(tb->sender);

                QDateTime time = QDateTime::fromSecsSinceEpoch(tb->timestamp);
            }
            break;
        }
    }
}

void Client::onErrorOccurred(QAbstractSocket::SocketError err)
{
    if (err == QAbstractSocket::UnknownSocketError) return;
    QString errMsg;
    switch(err) {
    case QAbstractSocket::ConnectionRefusedError:
        errMsg = "服务器拒绝连接（可能未启动/端口错误）";
        break;
    case QAbstractSocket::HostNotFoundError:
        errMsg = "未找到服务器（IP地址错误）";
        break;
    default:
        errMsg = m_socket->errorString(); // 兜底描述
    }
    qDebug() << "[Client 错误]：" << errMsg;
    emit sig_error(errMsg); // 通知UI层提示用户

}

