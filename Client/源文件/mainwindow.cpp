#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

#include <QCloseEvent>
#include <QScrollArea>
#include <QFileDialog>
#include <QTimer>
#include <QInputDialog>
#include <QFileInfo>
#include <QStandardPaths>
#include <QScrollBar>

#define MAX_SIZE 8192

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // m_client = new Client("192.168.88.132",9090,this);
    m_client = new Client(this);
    autoConnectToServer();

    //初始化一些基本内容
    init();
    initButton();

    connect(m_client, &Client::sig_error, this, [=](QString errMsg){
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("发送失败");
        msgBox.setText(errMsg);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.addButton(tr("确定"), QMessageBox::AcceptRole);
        msgBox.exec();
        ui->input_Edit->setStyleSheet("border: 1px solid red;");
        QTimer::singleShot(3000, [=](){
            ui->input_Edit->setStyleSheet("");
        });
    });


    // connect(m_client, &Client::sig_recvData, this, &MainWindow::onRecvServerMessage);
    // 连接收到文本消息的槽函数
    connect(m_client,&Client::sigMessageReceived,this,&MainWindow::AddMessageToChat);
    connect(m_client, &Client::sigFileNotifyReceived, this, &MainWindow::AddFileNotifyItem);

    // 连接下载进度和完成信号
    connect(m_client, &Client::sigDownloadProgress, this, &MainWindow::DownloadProgress);
    connect(m_client, &Client::sigDownloadFinished, this, &MainWindow::DownloadFinished);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::init()
{
    // 设置用户头像
    QLabel *imgLabel = ui->img_label;
    QPixmap pixmap(":/images/Profile_picture.png");
    if(pixmap.isNull()){
        qDebug()<<"Profile picture is wrong!";
        imgLabel->setText("Profile picture error!");
        return;
    }
    // 设置比例
    pixmap = pixmap.scaled(imgLabel->size(),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    imgLabel->setPixmap(pixmap);

    isOpen = true;

    ui->name_label->setText("胡子鱼");
    ui->person_signature_label->setText("人要自由自在，一切置身事外");

    m_name = ui->name_label->text() ;
    qDebug()<<"m_name = "<<m_name;

    // 开启自动换行
    ui->person_signature_label->setWordWrap(true);
    // 设置尺寸策略（水平可扩展，垂直自适应）
    ui->person_signature_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    // 限制最大宽度（避免文本过长时过度拉伸，保持界面整洁）
    ui->person_signature_label->setMaximumWidth(200);
    ui->person_signature_label->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // 初始化toolbutton
    QToolButton *toolbtn = ui->sendfile_Button;
    toolbtn->setIcon(QIcon(":/images/sendfile.png"));
    toolbtn->setIconSize(QSize(34,34));
    toolbtn->setFixedSize(34,34);
    toolbtn->setCursor(Qt::PointingHandCursor);

    toolbtn = ui->saying_Button;
    toolbtn->setIcon(QIcon(":/images/saying.png"));
    toolbtn->setIconSize(QSize(34,34));
    toolbtn->setFixedSize(34,34);
    toolbtn->setCursor(Qt::PointingHandCursor);

    toolbtn = ui->remote_Button;
    toolbtn->setIcon(QIcon(":/images/remoteControle.png"));
    toolbtn->setIconSize(QSize(34,34));
    toolbtn->setFixedSize(34,34);
    toolbtn->setCursor(Qt::PointingHandCursor);

    toolbtn = ui->design_Button;
    toolbtn->setIcon(QIcon(":/images/design.png"));
    toolbtn->setIconSize(QSize(34,34));
    toolbtn->setFixedSize(34,34);
    toolbtn->setCursor(Qt::PointingHandCursor);

    toolbtn = ui->addmessage_Button;
    toolbtn->setIcon(QIcon(":/images/addMessage.png"));
    toolbtn->setIconSize(QSize(34,34));
    toolbtn->setFixedSize(34,34);
    toolbtn->setCursor(Qt::PointingHandCursor);

    toolbtn = ui->rename_Button;
    toolbtn->setIcon(QIcon(":/images/rename.png"));
    toolbtn->setIconSize(QSize(26,26));
    toolbtn->setFixedSize(26,26);
    toolbtn->setCursor(Qt::PointingHandCursor);


    QScrollArea *chatScrollArea = ui->scrollArea;
    chatScrollArea->setWidgetResizable(true); // 内容Widget自动适应高度
    chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // 隐藏水平滚动条

    // chatScrollArea->setWidget(ui->chat_widget);

    QVBoxLayout *chatLayout = ui->chat_VLayout;
    chatLayout->setSpacing(10); // 聊天项之间的间距
    chatLayout->setContentsMargins(10, 10, 10, 10); // 容器内边距
    // chatLayout->addStretch(1); // 底部留白，新消息从上方追加
    chatLayout->setAlignment(Qt::AlignTop);
    chatLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);

}

void MainWindow::initButton()
{
    // 发送消息按钮
    ui->send_Button->setShortcut(Qt::CTRL + Qt::Key_Return);
    ui->send_Button->setCursor(Qt::PointingHandCursor);
    ui->shift_Button->setCursor(Qt::PointingHandCursor);

    QToolButton *toolbtn = ui->font_Button;
    toolbtn->setIcon(QIcon(":/images/font.png"));
    toolbtn->setIconSize(QSize(28,28));
    toolbtn->setFixedSize(28,28);
    toolbtn->setCursor(Qt::PointingHandCursor);

    toolbtn = ui->face_Button;
    toolbtn->setIcon(QIcon(":/images/face.png"));
    toolbtn->setIconSize(QSize(28,28));
    toolbtn->setFixedSize(28,28);
    toolbtn->setCursor(Qt::PointingHandCursor);

    toolbtn = ui->picture_Button;
    toolbtn->setIcon(QIcon(":/images/picture.png"));
    toolbtn->setIconSize(QSize(28,28));
    toolbtn->setFixedSize(28,28);
    toolbtn->setCursor(Qt::PointingHandCursor);

    toolbtn = ui->message_Button;
    toolbtn->setIcon(QIcon(":/images/message.png"));
    toolbtn->setIconSize(QSize(28,28));
    toolbtn->setFixedSize(28,28);
    toolbtn->setCursor(Qt::PointingHandCursor);

    toolbtn = ui->reconnect_Button;
    toolbtn->setIcon(QIcon(":/images/reConnect.png"));
    toolbtn->setIconSize(QSize(28,28));
    toolbtn->setFixedSize(28,28);
    toolbtn->setCursor(Qt::PointingHandCursor);
}

void MainWindow::autoConnectToServer()
{
    m_client->connectToServer(m_client->getIp(),m_client->getPort());

}

void MainWindow::chatMessageDisplay(QString senderName, QString timeStr, QByteArray contentBytes)
{
    QString headerText = QString("%1 %2").arg(senderName).arg(timeStr);

    QWidget *msgWidget = new QWidget(this);

    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    msgWidget->setSizePolicy(sizePolicy);

    msgWidget->setStyleSheet("background-color: transparent;");

    QGridLayout *gridLayout = new QGridLayout(msgWidget);
    gridLayout->setContentsMargins(10, 5, 10, 5); // 设置一点边距
    gridLayout->setVerticalSpacing(2); // 名字和内容之间的间距紧凑一点

    QLabel *nameTimeLabel = new QLabel(headerText);
    nameTimeLabel->setStyleSheet("color: blue; font-size: 12px; font-weight: bold;");
    nameTimeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel *contentLabel = new QLabel(contentBytes);
    contentLabel->setStyleSheet("color: black; font-size: 14px;");
    contentLabel->setWordWrap(true);
    contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse); // 允许鼠标复制文字
    contentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    gridLayout->addWidget(nameTimeLabel, 0, 0);
    gridLayout->addWidget(contentLabel, 1, 0);

    ui->chat_VLayout->addWidget(msgWidget);

    QTimer::singleShot(10, this, [=](){
        // 获取垂直滚动条
        QScrollBar *vBar = ui->scrollArea->verticalScrollBar();
        // 设置滚动条位置为最大值（即最底部）
        vBar->setValue(vBar->maximum());
    });
}

void MainWindow::sendDownloadRequest(QString fileName)
{
    if(!m_client || !m_client->isConnected() || fileName.isEmpty()){
        qWarning() << "下载请求失败：客户端未初始化或断开连接或文件名为空";
        return;
    }
    QByteArray nameBytes = fileName.toUtf8();
    PacketHeader header;
    header.type = MSG_DOWNLOAD_REQ;
    header.length = nameBytes.size();

    QByteArray packet;
    packet.append((char*)&header,sizeof(PacketHeader));
    packet.append(nameBytes);

    m_client->sendData(packet);
    qDebug()<<"[Request]请求下载文件："<<fileName;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"确认退出","你确定要关闭窗口嘛？",
                                  QMessageBox::Yes | QMessageBox::No);

    if(reply == QMessageBox::Yes){
        event->accept();
    }else{
        event->ignore();
    }
}

void MainWindow::AddMessageToChat(QString sender, QString content,QString timeStr)
{
    QString headerText = QString("%1 %2").arg(sender).arg(timeStr);

    QWidget *msgWidget = new QWidget(this);

    msgWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    msgWidget->setStyleSheet("background-color: transparent;");

    QGridLayout *gridLayout = new QGridLayout(msgWidget);
    gridLayout->setContentsMargins(10, 5, 10, 5); // 设置一点边距
    gridLayout->setVerticalSpacing(2); // 名字和内容之间的间距紧凑一点

    QLabel *nameTimeLabel = new QLabel(headerText);
    nameTimeLabel->setStyleSheet("color: black; font-size: 12px; font-weight: bold;");
    nameTimeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel *contentLabel = new QLabel(content);
    contentLabel->setStyleSheet("color: black; font-size: 14px;");
    contentLabel->setWordWrap(true);
    contentLabel->setTextInteractionFlags(Qt::TextSelectableByMouse); // 允许鼠标复制文字
    contentLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    gridLayout->addWidget(nameTimeLabel, 0, 0);
    gridLayout->addWidget(contentLabel, 1, 0);

    ui->chat_VLayout->addWidget(msgWidget);

    QTimer::singleShot(50, this, [=](){
        // 拿到垂直滚动条
        QScrollBar *vBar = ui->scrollArea->verticalScrollBar();
        // 滚动到最大值（最底部）
        vBar->setValue(vBar->maximum());
    });
}

void MainWindow::on_send_Button_clicked()
{
    QString sendMessage = ui->input_Edit->toPlainText();
    QString senderName = ui->name_label->text();
    if(sendMessage.isEmpty()){
        QMessageBox::warning(this,"warning","输入框为空，无法发送数据",QMessageBox::Yes);
        return;
    }
    if(senderName.isEmpty()){
        QMessageBox::warning(this,"warning","昵称不能为空",QMessageBox::Yes);
        return;
    }
    if(m_client && m_client->isConnected()){
        TextBody tb;
        tb.timestamp = QDateTime::currentSecsSinceEpoch(); // 获取当前时间戳

        memset(tb.sender, 0, sizeof(tb.sender));
        strncpy(tb.sender, senderName.toStdString().c_str(), sizeof(tb.sender) - 1);

        QByteArray contentBytes = sendMessage.toUtf8();

        QByteArray packetBody;
        packetBody.append((char*)&tb, sizeof(TextBody));
        packetBody.append(contentBytes);

        PacketHeader header;
        header.type = MSG_TEXT;
        header.length = packetBody.size();

        // QByteArray headerData((char*)&header,sizeof(PacketHeader));

        QByteArray finalPacket;
        finalPacket.append((char*)&header,sizeof(PacketHeader));
        finalPacket.append(packetBody);

        m_client->sendData(finalPacket);

        // 消息只转发给了其他人
        QString timeStr = QDateTime::fromSecsSinceEpoch(tb.timestamp).toString("HH:mm:ss");

        // 显示到聊天框
        // 显示顺序，名称 时间
        // 换行：输入内容
        chatMessageDisplay(senderName,timeStr,contentBytes);
        // 清空输入框
        ui->input_Edit->clear();
    }
    else{
        QMessageBox::warning(this, "错误", "未连接到服务器，发送失败", QMessageBox::Yes);
        qDebug() << "客户端对象未初始化或未连接";
    }
}


void MainWindow::on_sendfile_Button_clicked()
{
    if(!m_client || !m_client->isConnected()){
        QMessageBox::warning(this,"错误","未连接到服务器，无法发送文件");
        return;
    }
    QStringList filePaths = QFileDialog::getOpenFileNames(this,"选择文件");
    if(!filePaths.isEmpty()){
        QList<QFileInfo> fileInfoList;
        for(const QString& path:filePaths){
            fileInfoList.append(QFileInfo(path));
        }
        // 弹出自定义对话框
        FileSendDialog sendDialog(this);
        sendDialog.setFileDataList(fileInfoList);

        // 美化一下发送对话框
        // todo

        // 点击发送后处理多文件发送
        if (sendDialog.exec() == QDialog::Accepted) {
            QString uploaderName = ui->name_label->text();
            for(const QFileInfo &info:fileInfoList){
                QString path = info.absoluteFilePath();
                QFile file(path);
                if(!file.open(QIODevice::ReadOnly))
                {
                    qDebug()<<"无法打开文件"<<path;
                    continue;
                }
                uint64_t size = file.size();
                QString Name = info.fileName();

                FileMeta meta;
                meta.fileSize = size; // 必须保证这个值没有被后面的操作覆盖

                memset(meta.fileName, 0, sizeof(meta.fileName));
                QByteArray nameBytes = Name.toUtf8();
                // 使用 qMin 确保不越界，且使用 memcpy 支持中文
                memcpy(meta.fileName, nameBytes.data(), qMin((int)sizeof(meta.fileName)-1, nameBytes.size()));

                memset(meta.uploader, 0, sizeof(meta.uploader));
                QByteArray uploaderBytes = uploaderName.toUtf8();
                memcpy(meta.uploader, uploaderBytes.data(), qMin((int)sizeof(meta.uploader)-1, uploaderBytes.size()));
                QByteArray metaBody((char*)&meta,sizeof(FileMeta));

                PacketHeader metaheader;
                metaheader.type = MSG_FILE_META;
                metaheader.length = metaBody.size();

                QByteArray metaPacket;
                metaPacket.append((char*)&metaheader,sizeof(PacketHeader));
                metaPacket.append(metaBody);

                m_client->sendData(metaPacket);

                // 循环分块发送文件内容 (MSG_FILE_DATA)
                char buf[MAX_SIZE]; // 8kb
                qint64 sentBytes = 0;
                while(!file.atEnd()){
                    qint64 len = file.read(buf,sizeof(buf));
                    if(len>0){
                        PacketHeader dataHeader;
                        dataHeader.type = MSG_FILE_DATA;
                        dataHeader.length = (uint32_t)len;
                        QByteArray dataPacket;
                        dataPacket.resize(sizeof(PacketHeader)+len);

                        memcpy(dataPacket.data(),&dataHeader,sizeof(PacketHeader));
                        memcpy(dataPacket.data()+sizeof(PacketHeader),buf,len);

                        m_client->sendData(dataPacket);
                        sentBytes+=len;

                        // 防止界面在发送大文件的时候卡死
                        QCoreApplication::processEvents();
                    }
                }
                file.close();
                qDebug()<<"文件发送完毕："<<Name;
            }
            QMessageBox::information(this,"提示","所有文件已加入到发送队列");
        }
    }
}


void MainWindow::on_reconnect_Button_clicked()
{
    autoConnectToServer();
}


void MainWindow::on_shift_Button_clicked()
{
    if(isOpen){
        ui->shift_Button->setText("打开");
        isOpen=false;
    }else{
        ui->shift_Button->setText("关闭");
        isOpen=true;
    }
}


void MainWindow::on_picture_Button_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this,"选择文件","D://", tr("Images (*.png *.xpm *.jpg)"));

}


void MainWindow::on_rename_Button_clicked()
{
    bool isOk; // 记录用户是否点击“确定”
    QString newName = QInputDialog::getText(
        this,                  // 父窗口
        "修改用户名",           // 对话框标题
        "请输入新的用户名：",    // 提示文字
        QLineEdit::Normal,     // 输入框类型（普通文本）
        ui->name_label->text(), // 默认值（当前显示的用户名）
        &isOk                  // 输出参数：用户是否确认
        );

    // 2. 处理输入结果：用户点击“确定”且输入不为空
    if (isOk && !newName.trimmed().isEmpty()) {
        ui->name_label->setText(newName.trimmed());
        qDebug() << "用户名已修改为：" << newName.trimmed();
    }

}

void MainWindow::AddFileNotifyItem(QString fileName, QString uploader, uint64_t size, QString timeStr)
{
    // 1. 创建气泡容器 (为了控制气泡的宽度和位置，通常外面套一个Widget)
    QWidget *container = new QWidget(this);
    QHBoxLayout *containerLayout = new QHBoxLayout(container);
    containerLayout->setContentsMargins(0, 5, 0, 5);

    // 2. 创建我们自定义的文件通知控件
    FileBubbleWidget *fileWidget = new FileBubbleWidget(fileName, size, timeStr, this);
    fileWidget->setMaximumWidth(300); // 限制气泡最大宽度，模仿QQ
    fileWidget->setMinimumWidth(250);

    // 3. 处理文件控件的按钮点击事件
    connect(fileWidget, &FileBubbleWidget::sigReceive, this, [=](){
        qDebug() << "点击了接收: " << fileName;
        // 这里调用 m_client->sendData 发送 MSG_DOWNLOAD_REQ
        QString appDir = QCoreApplication::applicationDirPath();
        QString downloadDir = QDir(appDir).filePath("temp");
        QString savePath = QDir(downloadDir).filePath(fileName);

        // 2. 检查并创建目录（如果目录不存在，下载会失败）
        QDir dir(downloadDir);
        if (!dir.exists()) {
            // 递归创建目录（包括多级子目录）
            bool mkDirOk = dir.mkpath(".");
            if (!mkDirOk) {
                QMessageBox::warning(this, "错误", QString("创建下载目录失败：%1").arg(downloadDir));
                return; // 目录创建失败，终止下载
            }
        }
        m_client->setSaveFilePath(savePath);

        sendDownloadRequest(fileName);

        QMessageBox::information(this, "提示", QString("开始下载，将保存至:\n%1").arg(savePath));
    });

    connect(fileWidget, &FileBubbleWidget::sigSaveAs, this, [=](){
        qDebug() << "点击了另存为";
        // 弹出文件保存对话框，然后请求下载
        QString savepath= QFileDialog::getSaveFileName(this,"另存为",fileName);

        if(!savepath.isEmpty()){
            m_client->setSaveFilePath(savepath);

            sendDownloadRequest(fileName);
        }
    });

    connect(fileWidget, &FileBubbleWidget::sigCancel, this, [=](){
        qDebug() << "点击了取消";
        // 可以在这里把这个 fileWidget 从界面移除
        fileWidget->deleteLater();
        container->deleteLater();
    });

    // 左侧弹簧把气泡挤到左边 (或者不需要弹簧直接 addWidget)
    containerLayout->addWidget(fileWidget);
    containerLayout->addStretch(); // 右边加弹簧，让气泡靠左

    // 5. 添加到聊天列表 (ui->chat_VLayout)
    ui->File_VLayout->setAlignment(Qt::AlignTop);
    ui->File_VLayout->addWidget(container);

    // 自动滚动
    QTimer::singleShot(50, this, [=](){
        ui->scrollArea->verticalScrollBar()->setValue(
            ui->scrollArea->verticalScrollBar()->maximum()
            );
    });
}

void MainWindow::DownloadProgress(qint64 current, qint64 total)
{
    if(m_currentDownloadWidget && total >0){
        int percent = (int)((current*100)/total);
        m_currentDownloadWidget->setProgress(percent);
    }
}

void MainWindow::DownloadFinished(QString fileName)
{
    if(m_currentDownloadWidget){
        QString fullPath = m_client->getSaveFilePath();

        m_currentDownloadWidget->setDownloaded(fullPath);

        QMessageBox::information(this,"提示","文件下载成功!");
        m_currentDownloadWidget = nullptr;
    }
}

