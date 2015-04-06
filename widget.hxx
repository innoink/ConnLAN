#ifndef WIDGET_HXX
#define WIDGET_HXX

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QNetworkInterface>
#include <QFileDialog>
#include <QFile>
#include <QDialog>

#include "msg_receiver.hxx"
#include "msg_sender.hxx"
#include "file_receiver.hxx"
#include "file_sender.hxx"

class file_recv_dialog : public QDialog
{
    Q_OBJECT
public:
    file_recv_dialog(QWidget *parent);
    void set_info(QString ip, QString fname, uint32_t fno, uint32_t size);
signals:
    void save_path(QString ip, uint32_t fno, QString path);
    void file_rejected(QString ip, uint32_t fno);
private:
    QString ip;
    uint32_t fno;
    QLabel *lb_info;
    QPushButton *pb_acc, *pb_rej;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = 0);
    ~Widget();

    void log_send_msg(QString ip, QString msg);
    void log_recv_msg(QString ip, QString msg);

private:
    QString get_host_ip();
private:
    //left
    QLabel *lb_host_info;
    QLabel *lb_file_size;
    QLineEdit *le_file_path;
    QLineEdit *le_ip;
    QTextEdit *te_msg;
    QPushButton *pb_select_file;
    QPushButton *pb_send_msg;
    QPushButton *pb_send_file;
    //right
    QTextEdit *te_log;

    file_recv_dialog *frd;

    msg_receiver *mr;
    msg_sender   *ms;
    file_receiver *fr;
    file_sender *fs;
};

#endif // WIDGET_HXX
