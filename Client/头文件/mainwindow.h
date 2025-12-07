#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "client.h"
#include "client.h"
#include "Protocol.h"
#include "filesenddialog.h"
#include "FileBubbleWidget.h"

#include <QMainWindow>
#include <QPixmap>
#include <QLabel>
#include <QMessageBox>
#include <QToolButton>
#include <QIcon>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void init();
    void initButton();
    void autoConnectToServer();
    void chatMessageDisplay(QString senderName,QString timeStr,QByteArray contentBytes);
    void sendDownloadRequest(QString fileName);

protected:
    void closeEvent(QCloseEvent* event)override;

private slots:
    void on_send_Button_clicked();

    void on_sendfile_Button_clicked();

    void on_reconnect_Button_clicked();

    void on_shift_Button_clicked();

    void on_picture_Button_clicked();

    void on_rename_Button_clicked();
    void AddMessageToChat(QString sender, QString content, QString timeStr);
    void AddFileNotifyItem(QString fileName,QString uploader,uint64_t size,QString timeStr);

    // 文件下载进度条
    void DownloadProgress(qint64 current,qint64 total);
    void DownloadFinished(QString fileName);

private:
    Ui::MainWindow *ui;
    Client *m_client;

    bool isOpen;
    QString m_name;
    FileBubbleWidget* m_currentDownloadWidget = nullptr;

};
#endif // MAINWINDOW_H
