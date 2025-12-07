#include "filebubblewidget.h"
#include <QDesktopServices> // 用于打开系统文件

#include <QDebug>

FileBubbleWidget::FileBubbleWidget(const QString &fileName,uint64_t fileSize,
                                    const QString &timeStr,QWidget *parent)
    :QWidget(parent),m_fileName(fileName),m_fileSize(fileSize),m_timeStr(timeStr)
{
    setupUI();
}

void FileBubbleWidget::setDownloaded(const QString &filePath)
{

    // 1. UI 状态更新
    m_progressBar->setValue(100);
    m_btnReceive->setText("打开");

    // 2. 隐藏不需要的按钮
    m_btnSaveAs->setVisible(false);
    m_btnCancel->setVisible(false);

    // 3. 断开旧的信号连接 (防止点击"打开"时又去触发下载)
    m_btnReceive->disconnect();

    // 4. 连接新的逻辑：点击调用系统默认程序打开文件
    connect(m_btnReceive, &QPushButton::clicked, this, [=](){
        // QUrl::fromLocalFile 会自动处理路径中的特殊字符和分隔符
        bool isOpened = QDesktopServices::openUrl(QUrl::fromLocalFile(filePath));

        if(!isOpened) {
            qDebug() << "无法打开文件:" << filePath;
        }
    });
}

void FileBubbleWidget::setupUI()
{
    // 1. 主布局 (水平：左边图标，右边内容)
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(10);

    // 2. 左侧图标
    m_iconLabel = new QLabel(this);
    m_iconLabel->setPixmap(QPixmap(":/images/Files.png").scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_iconLabel->setFixedSize(50, 50);
    m_iconLabel->setStyleSheet("background-color: #eee; border-radius: 5px;"); // 占位样式

    // 3. 右侧布局 (垂直：信息 -> 进度条 -> 底部栏)
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(2);

    // 3.1 文件名和大小 "新建文件夹(1.08MB)"
    QString infoText = QString("%1(%2)").arg(m_fileName).arg(formatFileSize(m_fileSize));
    m_infoLabel = new QLabel(infoText, this);
    m_infoLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #333;");

    // 3.2 进度条 (细条，灰色)
    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(3); // 细一点
    m_progressBar->setTextVisible(false); // 不显示百分比文字
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0); // 初始为0
    // 样式：背景灰色，进度条蓝色
    m_progressBar->setStyleSheet("QProgressBar { border: none; background: #E0E0E0; border-radius: 1px; }"
                                 "QProgressBar::chunk { background-color: #0078D7; border-radius: 1px; }");

    // 3.3 底部栏 (时间 + 按钮)
    QHBoxLayout *bottomLayout = new QHBoxLayout();

    // 时间
    m_timeLabel = new QLabel(m_timeStr, this);
    m_timeLabel->setStyleSheet("color: #999; font-size: 11px;");

    // 弹簧 (把按钮顶到右边)
    bottomLayout->addWidget(m_timeLabel);
    bottomLayout->addStretch();

    // 按钮样式 (像超链接一样的蓝色文字按钮)
    QString btnStyle = "QPushButton { border: none; color: #0066CC; background: transparent; font-size: 12px; }"
                       "QPushButton:hover { text-decoration: underline; color: #004499; }";

    m_btnReceive = new QPushButton("接收", this);
    m_btnReceive->setCursor(Qt::PointingHandCursor);
    m_btnReceive->setStyleSheet(btnStyle);

    m_btnSaveAs = new QPushButton("另存为", this);
    m_btnSaveAs->setCursor(Qt::PointingHandCursor);
    m_btnSaveAs->setStyleSheet(btnStyle);

    m_btnCancel = new QPushButton("取消", this);
    m_btnCancel->setCursor(Qt::PointingHandCursor);
    m_btnCancel->setStyleSheet(btnStyle);

    bottomLayout->addWidget(m_btnReceive);
    bottomLayout->addWidget(m_btnSaveAs);
    bottomLayout->addWidget(m_btnCancel);

    // 4. 组装布局
    rightLayout->addWidget(m_infoLabel);
    rightLayout->addWidget(m_progressBar);
    rightLayout->addLayout(bottomLayout);

    mainLayout->addWidget(m_iconLabel);
    mainLayout->addLayout(rightLayout);

    // 整体背景 (可选，白色圆角卡片)
    this->setStyleSheet("FileBubbleWidget { background-color: white; border-radius: 5px; }");
    this->setAttribute(Qt::WA_StyledBackground); // 确保自定义Widget背景生效

    // 1. 强制设置固定高度 (根据你的图标大小50px，这里设为80px足够放下文字和按钮)
    this->setFixedHeight(80);

    // 2. 或者：设置垂直方向为"固定"，不让布局管理器拉伸它
    // this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    // 5. 绑定信号
    connect(m_btnReceive, &QPushButton::clicked, this, &FileBubbleWidget::sigReceive);
    connect(m_btnSaveAs, &QPushButton::clicked, this, &FileBubbleWidget::sigSaveAs);
    connect(m_btnCancel, &QPushButton::clicked, this, &FileBubbleWidget::sigCancel);
}

QString FileBubbleWidget::formatFileSize(uint64_t size)
{
    if (size < 1024) return QString("%1 B").arg(size);
    if (size < 1024 * 1024) return QString::asprintf("%.2f KB", size / 1024.0);
    return QString::asprintf("%.2f MB", size / (1024.0 * 1024.0));
}
