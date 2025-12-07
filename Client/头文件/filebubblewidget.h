#ifndef FILEBUBBLEWIDGET_H
#define FILEBUBBLEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDateTime>

class FileBubbleWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FileBubbleWidget(const QString &fileName,uint64_t fileSize,
                            const QString &timeStr,QWidget *parent = nullptr);
    void setProgress(int value){
        m_progressBar->setValue(value);
    }
    void setDownloaded(const QString &filePath);
signals:
    void sigReceive(); //点击接收
    void sigSaveAs(); // 另存为
    void sigCancel(); // 点击取消

private:
    void setupUI();
    QString formatFileSize(uint64_t size); // 辅助函数，把字节转为MB/KB

private:
    QString m_fileName;
    uint64_t m_fileSize;
    QString m_timeStr;

    QLabel *m_iconLabel;
    QLabel *m_infoLabel;
    QProgressBar *m_progressBar;
    QLabel *m_timeLabel;

    QPushButton *m_btnReceive;
    QPushButton *m_btnSaveAs;
    QPushButton *m_btnCancel;

};

#endif // FILEBUBBLEWIDGET_H
