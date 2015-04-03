#ifndef FILE_SENDER_HXX
#define FILE_SENDER_HXX

#include <QObject>
#include <QThread>
#include <QString>
#include <QAtomicInt>
#include <QSet>
#include "cl_global.hxx"

class fs_worker : public QObject
{
    Q_OBJECT
public:
    fs_worker();
    ~fs_worker();
public slots:
    void send_data(QString ip, QString file_path);
signals:
    void send_finished();
private:
    QString file_path;
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

signals:

public slots:

private:
    QAtomicInt stop;
    int server_fd;
    struct pkt_t pkt;
    struct sockaddr_in server_addr;

    uint32_t file_no;
    QSet<uint32_t> file_pending;
};

#endif // FILE_SENDER_HXX
