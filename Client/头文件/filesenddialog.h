#ifndef FILESENDDIALOG_H
#define FILESENDDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QFileIconProvider>
#include <QScrollArea>
#include <QList>

class FileSendDialog : public QDialog
{
    Q_OBJECT
public:
    explicit FileSendDialog(QWidget *parent = nullptr);
    // 接收多个文件信息（关键修改：参数改为QList<QFileInfo>）
    void setFileDataList(const QList<QFileInfo>& fileInfoList);

private slots:
    void onSendBtnClicked();

private:
    QVBoxLayout *fileListLayout; // 存储所有文件的信息布局
    QPushButton *sendBtn;        // 发送按钮
};

#endif // FILESENDDIALOG_H
