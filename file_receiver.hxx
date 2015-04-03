#ifndef FILE_RECEIVER_HXX
#define FILE_RECEIVER_HXX

#include <QAtomicInt>
#include <QThread>
#include <QMultiHash>
#include "cl_global.hxx"

class file_receiver : public QThread
{
    Q_OBJECT
public:
    file_receiver();
    ~file_receiver();

    void start_server();
    void stop_server();
private:
    void run();

    void proc_file_new(QString ip, struct pkt_t *pkt);
    void proc_file_data(QString ip, struct pkt_t *pkt);

signals:
    void file_new(QString ip, QString name, uint32_t size);
    void recv_completed();

public slots:
    void send_file_ack1();
    void send_file_ack2();
private:
    QAtomicInt stop;
    int server_fd;
    struct sockaddr_in server_addr;
    struct pkt_t pkt;

    QMultiHash<QString, uint32_t> file_pending;
};
#endif // FILE_RECEIVER_HXX
