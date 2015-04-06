#ifndef FILE_SENDER_HXX
#define FILE_SENDER_HXX

#include <QObject>
#include <QThread>
#include <QString>
#include <QAtomicInt>
#include <QMap>
#include "cl_global.hxx"

class fs_worker : public QObject
{
    Q_OBJECT
public:
    fs_worker();
    ~fs_worker();
public slots:
    void do_send_data(QString ip, QString file_path, uint32_t file_no);
signals:
    void send_finished();
};

class file_sender : public QThread
{
    Q_OBJECT
public:
    explicit file_sender();
    ~file_sender();

    void start_server();
    void stop_server();

    void send_file(QString ip, QString file_path);
private:
    void run();
    void init_socket();

    void proc_file_ack1(QString ip, struct pkt_t *pkt);
    void proc_file_ack2(QString ip, struct pkt_t *pkt);

    void send_file_data(QString ip, QString path, uint32_t file_no);

signals:
    void file_accepted(QString ip);
    void file_denied(QString ip);
    void file_trans_done(QString ip);
    void file_send_finished(QString ip);
    //
    void operate_worker(QString ip, QString file_path, uint32_t file_no);

public slots:

private:
    QAtomicInt stop;
    int server_fd;
    struct pkt_t pkt;
    struct sockaddr_in server_addr;

    uint32_t file_no;
    QMap<uint32_t, QString> file_pending;
};

#endif // FILE_SENDER_HXX
