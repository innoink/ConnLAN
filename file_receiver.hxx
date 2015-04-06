#ifndef FILE_RECEIVER_HXX
#define FILE_RECEIVER_HXX

#include <QAtomicInt>
#include <QThread>
#include <QMultiHash>
#include <QThreadPool>
#include <QRunnable>
#include <QHash>
#include <QFile>
#include <QPair>
#include <QBitArray>
#include "cl_global.hxx"

class write_task : public QRunnable
{
public:
    write_task(QString file_name, QByteArray *ba);
    void run();
private:
    QString file_name;
    QByteArray *ba;
};

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
    void init_socket();

    void proc_file_new(QString ip, struct pkt_t *pkt);
    void proc_file_data(QString ip, struct pkt_t *pkt);

signals:
    void file_new(QString ip, QString name, uint32_t fno, uint32_t size);
    void recv_completed();

public slots:
    void send_file_ack1(QString ip, uint32_t file_no, bool is_ok);
    void send_file_ack2(QString ip, uint32_t file_no, bool is_ok);

    void set_save_file_name(QString ip, uint32_t fno, QString name);
private:
    QAtomicInt stop;
    int server_fd;
    struct sockaddr_in server_addr;
    struct pkt_t pkt;

    QMultiHash<QString, uint32_t> file_pending;
    QHash<QPair<QString, uint32_t>, QBitArray*> file_blocks;
    QHash<QPair<QString, uint32_t>, QByteArray*> file_data;
    QHash<QPair<QString, uint32_t>, QString> file_name;
};
#endif // FILE_RECEIVER_HXX
