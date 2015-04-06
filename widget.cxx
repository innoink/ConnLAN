#include "widget.hxx"

file_recv_dialog::file_recv_dialog(QWidget *parent):QDialog(parent)
{
    lb_info = new QLabel;
    pb_acc = new QPushButton(tr("Accept"));
    pb_rej = new QPushButton(tr("Reject"));
    QVBoxLayout *vl = new QVBoxLayout;
    QHBoxLayout *hl = new QHBoxLayout;
    hl->addWidget(pb_acc);
    hl->addWidget(pb_rej);
    vl->addWidget(lb_info);
    vl->addLayout(hl);
    setLayout(vl);
    connect(pb_acc, &QPushButton::clicked,
            [this]()
            {
                QString path;
                path = QFileDialog::getSaveFileName(0,
                                                    tr("Save to"),
                                                    "./");
                emit save_path(ip, fno, path);
                QDialog::accept();

            });
    connect(pb_rej, &QPushButton::clicked,
            [this]()
            {
                emit file_rejected(ip, fno);
                QDialog::reject();
            });
}

void file_recv_dialog::set_info(QString ip, QString fname, uint32_t fno, uint32_t size)
{
    lb_info->setText(tr("File %1 from %2 size %3").arg(fname).arg(ip).arg(size));
    this->ip = ip;
    this->fno = fno;
}

Widget::Widget(QWidget *parent)
    : QWidget(parent)
{
    //left
    QVBoxLayout *vl_left;
    QHBoxLayout *hl;
    QVBoxLayout *vl1, *vl2;
    lb_host_info = new QLabel;
    lb_file_size = new QLabel;
    le_file_path = new QLineEdit;
    le_ip        = new QLineEdit;
    pb_select_file = new QPushButton(tr("Select"));
    pb_send_file = new QPushButton(tr("Send File"));
    te_msg       = new QTextEdit;
    pb_send_msg  = new QPushButton(tr("Send Message"));

    vl_left = new QVBoxLayout;
    hl = new QHBoxLayout;

    lb_host_info->setText(tr("Host IP:").append(get_host_ip()));
    le_file_path->setReadOnly(true);
    vl_left->addWidget(lb_host_info);
    hl->addWidget(new QLabel(tr("Destination IP:")));
    hl->addWidget(le_ip);
    vl_left->addLayout(hl);
    hl = new QHBoxLayout;
    vl1 = new QVBoxLayout;
    hl->addWidget(new QLabel(tr("File Path:")));
    hl->addWidget(le_file_path);
    hl->addWidget(pb_select_file);
    vl1->addLayout(hl);
    hl = new QHBoxLayout;
    hl->addWidget(new QLabel(tr("File Size:")));
    hl->addWidget(lb_file_size);
    vl1->addLayout(hl);
    vl1->addWidget(pb_send_file);
    vl2 = new QVBoxLayout;
    vl2->addWidget(te_msg);
    vl2->addWidget(pb_send_msg);
    hl = new QHBoxLayout;
    hl->addLayout(vl1);
    hl->addLayout(vl2);
    vl_left->addLayout(hl);
    //right
    te_log = new QTextEdit;
    QHBoxLayout *hl_main = new QHBoxLayout;
    QVBoxLayout *vl_right = new QVBoxLayout;
    vl_right->addWidget(new QLabel(tr("LOG:")));
    vl_right->addWidget(te_log);
    hl_main->addLayout(vl_left);
    hl_main->addLayout(vl_right);

    setLayout(hl_main);

    connect(pb_select_file, &QPushButton::clicked,
            [this]()
            {
                QString path;
                path = QFileDialog::getOpenFileName(0,
                                                    tr("Select File:"),
                                                    tr("./"),
                                                    tr("All File(*.*)"));
                le_file_path->setText(path);
                lb_file_size->setText(QString("%1").arg(QFile(path).size()));
            });
    //
    mr = new msg_receiver;
    ms = new msg_sender;
    fr = new file_receiver;
    fs = new file_sender;

    frd = new file_recv_dialog(this);

    connect(pb_send_msg, &QPushButton::clicked,
            [this]()
            {
                ms->send_msg(le_ip->text(), te_msg->toPlainText());
            });
    connect(pb_send_msg, &QPushButton::clicked,
            [this]()
            {
                log_send_msg(le_ip->text(), te_msg->toPlainText());
            });
    connect(mr, &msg_receiver::msg_received, this, &Widget::log_recv_msg);
    connect(ms, &msg_sender::msg_accepted, this, &Widget::log_acc_msg);
    connect(pb_send_file, &QPushButton::clicked,
            [this]()
            {
                fs->send_file(le_ip->text(), le_file_path->text());
                te_log->append(tr("File to %1:\nPath: %2").arg(le_ip->text()).arg(le_file_path->text()));
            });
    connect(fs, &file_sender::file_accepted, this, &Widget::log_file_accepted);
    connect(fs, &file_sender::file_denied, this, &Widget::log_file_denied);
    connect(fs, &file_sender::file_send_finished, this, &Widget::log_file_send_finished);
    connect(fs, &file_sender::file_trans_done,
            [this](QString ip)
            {
                te_log->append(tr("File successfully transmitted to %1").arg(ip));
            });
    connect(fr, &file_receiver::file_new, this, &Widget::on_file_new);
    connect(frd, &file_recv_dialog::save_path,
            [this](QString ip, uint32_t fno, QString path)
            {
                fr->set_save_file_name(ip, fno, path);
                fr->send_file_ack1(ip, fno, true);
            });
    connect(frd, &file_recv_dialog::file_rejected,
            [this](QString ip, uint32_t fno)
            {
                fr->send_file_ack1(ip, fno, false);
            });


    mr->start_server();
    ms->start_server();
    fr->start_server();
    fs->start_server();
}

Widget::~Widget()
{
    mr->stop_server();
    ms->stop_server();
}

QString Widget::get_host_ip()
{
    foreach (QHostAddress address, QNetworkInterface::allAddresses()) {
        if (address.toString().contains("127.0.")) {
            continue;
        }
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            return address.toString();
        }
    }
       return QString();
}

void Widget::log_send_msg(QString ip, QString msg)
{
    te_log->append(QString(tr("Message To:%1\n%2")).arg(ip).arg(msg));
}

void Widget::log_recv_msg(QString ip, QString msg)
{
    te_log->append(QString(tr("Message From:%1\n%2")).arg(ip).arg(msg));
}

void Widget::log_acc_msg()
{
    te_log->append(tr("message accepted"));
}

void Widget::log_file_accepted(QString ip)
{
    te_log->append(tr("File accepted by %1").arg(ip));
}

void Widget::log_file_denied(QString ip)
{
    te_log->append(tr("File denied by %1").arg(ip));
}

void Widget::log_file_send_finished()
{
    te_log->append(tr("File send finished."));
}

void Widget::on_file_new(QString ip, QString name, uint32_t fno, uint32_t size)
{
    frd->set_info(ip, name, fno, size);
    frd->show();
}
