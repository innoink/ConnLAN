#ifndef MSG_SENDER_HXX
#define MSG_SENDER_HXX

#include <QThread>
#include <QAtomicInt>
#include <QString>
#include <QSet>
#include "cl_global.hxx"

class msg_sender : public QThread
{
    Q_OBJECT
public:
    explicit msg_sender();
    virtual ~msg_sender();

    void start_server();
    void stop_server();

    void send_msg(QString ip, QString msg);
private:
    void init_socket();
    void run();

signals:
    void msg_accepted();
    void msg_denied();

public slots:
private:
    QAtomicInt stop;
    int server_fd;
    struct pkt_t pkt;
    struct sockaddr_in server_addr;
    int fs_fd;

    uint32_t msg_no;
    QSet<uint32_t> msg_pending;
};

#endif // MSG_SENDER_HXX
