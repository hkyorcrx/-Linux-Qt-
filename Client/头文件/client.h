#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QString>
#include <QFile>

#include "Protocol.h"

class Client: public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject* parent = nullptr);
    Client(QString ip,quint16 port,QObject* parent = nullptr);
    ~Client();

public:
    void connectToServer(const QString& ip,quint16 port);
    void sendData(const QByteArray& data);
    bool isConnected()const;
    QString getIp(){
        return m_ip;
    }
    quint16 getPort(){
        return m_port;
    }
    void setSaveFilePath(const QString &path){
        m_saveFilePath = path;
    }
    QString getSaveFilePath(){
        return m_saveFilePath;
    }

signals:
    void sig_error(QString str);

    // 定义信号：发送者，内容，格式化后的时间字符串
    void sigMessageReceived(QString sender, QString content, QString timeStr);
    void sigFileNotifyReceived(QString fileName,QString uploader,uint64_t fileSize,QString timeStr);
    // 下载进度信号 (当前字节, 总字节)
    void sigDownloadProgress(qint64 current, qint64 total);
    // 下载完成信号
    void sigDownloadFinished(QString fileName);

private slots:
    void onConnected();
    void onReadyRead();   // 接收数据
    void onErrorOccurred(QAbstractSocket::SocketError err);

private:
    QTcpSocket *m_socket;  // 套接字对象
    QString m_ip;
    quint16 m_port;

    QByteArray m_buffer;    // 接收缓冲区

    // 下载相关变量
    QFile m_downloadFile;           // 当前正在写入的本地文件
    QString m_downloadFileName;     // 当前下载的文件名
    uint64_t m_totalBytesToRecv;    // 需要接收的总大小
    uint64_t m_currentBytesRecv;    // 当前已接收大小
    bool m_isDownloading = false;   // 是否处于下载状态
    QString m_saveFilePath;

    QFile m_file;                   // 本地文件对象
    uint64_t m_totalBytes = 0;      // 文件总大小
    uint64_t m_receivedBytes = 0;   // 已接收大小
};

#endif // CLIENT_H
