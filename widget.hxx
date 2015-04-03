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

#include "msg_receiver.hxx"
#include "msg_sender.hxx"

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

    msg_receiver *mr;
    msg_sender   *ms;
};

#endif // WIDGET_HXX
