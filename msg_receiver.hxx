#ifndef MSG_RECEIVER_HXX
#define MSG_RECEIVER_HXX

#include <QThread>
#include <QAtomicInt>
#include <QString>
#include "cl_global.hxx"

class msg_receiver : public QThread
{
    Q_OBJECT
public:
    explicit msg_receiver();
    virtual ~msg_receiver();

    void start_server();
    void stop_server();
private:
    void run();
    void init_socket();

signals:
    void msg_received(QString ip, QString msg);

public slots:

private:
    QAtomicInt stop;
    int server_fd;
    struct sockaddr_in server_addr;
    struct pkt_t pkt;
};

#endif // MSG_RECEIVER_HXX
