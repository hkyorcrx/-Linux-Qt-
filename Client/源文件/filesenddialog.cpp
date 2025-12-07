#include "FileSendDialog.h"

FileSendDialog::FileSendDialog(QWidget *parent) : QDialog(parent)
{
    // 1. 初始化滚动区域（文件过多时可滚动）
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true); // 自适应大小
    QWidget *scrollWidget = new QWidget(scrollArea);
    fileListLayout = new QVBoxLayout(scrollWidget);
    fileListLayout->setSpacing(10); // 文件之间的间距
    scrollArea->setWidget(scrollWidget);

    // 2. 发送按钮（初始文本占位，后续动态设置）
    sendBtn = new QPushButton(this);
    sendBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #007AFF;
            color: white;
            border-radius: 5px;
            padding: 10px;
            font-size: 14px;
        }
        QPushButton:hover {
            background-color: #0066CC;
        }
    )");

    // 3. 主布局（滚动区域+按钮）
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(scrollArea);
    mainLayout->addWidget(sendBtn);

    // 4. 对话框基础设置
    this->setFixedSize(320, 300); // 增大高度适配多文件
    this->setWindowTitle("发送给我的手机");

    // 5. 信号槽连接
    connect(sendBtn, &QPushButton::clicked, this, &FileSendDialog::onSendBtnClicked);
}

// 关键修改：接收多文件信息并展示
void FileSendDialog::setFileDataList(const QList<QFileInfo>& fileInfoList)
{
    // 1. 动态设置按钮文本（发送(文件个数)）
    int fileCount = fileInfoList.size();
    sendBtn->setText(QString("发送(%1)").arg(fileCount));

    // 2. 遍历所有文件，添加到界面
    QFileIconProvider iconProvider;
    for (const QFileInfo& fileInfo : fileInfoList) {
        // 单个文件的布局（图标+名称+大小）
        QHBoxLayout *fileItemLayout = new QHBoxLayout;

        // 文件图标
        QIcon fileIcon = iconProvider.icon(fileInfo);
        QLabel *iconLabel = new QLabel;
        iconLabel->setPixmap(fileIcon.pixmap(24, 24)); // 缩小图标适配多文件

        // 文件信息（名称+大小）
        qint64 fileSizeByte = fileInfo.size();
        QString sizeStr;
        if (fileSizeByte < 1024) {
            sizeStr = QString("%1 B").arg(fileSizeByte);
        } else if (fileSizeByte < 1024*1024) {
            sizeStr = QString("%1 KB").arg(fileSizeByte/1024.0, 0, 'f', 1);
        } else {
            sizeStr = QString("%1 MB").arg(fileSizeByte/(1024.0*1024.0), 0, 'f', 1);
        }
        QLabel *infoLabel = new QLabel(QString("%1<br/>%2").arg(fileInfo.fileName()).arg(sizeStr));
        infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

        // 添加到单个文件布局
        fileItemLayout->addWidget(iconLabel);
        fileItemLayout->addWidget(infoLabel);
        fileItemLayout->addStretch(); // 右对齐填充

        // 添加到文件列表布局
        fileListLayout->addLayout(fileItemLayout);
    }
}

void FileSendDialog::onSendBtnClicked()
{
    this->accept();
}
