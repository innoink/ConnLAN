#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include "file_sender.hxx"

fs_worker::fs_worker()
{
}

fs_worker::~fs_worker()
{
}

void fs_worker::do_send_data(QString ip, QString file_path, uint32_t file_no)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Can't open file:" << file_path;
        return;
    }
    uint32_t block_amount, block_no;
    block_amount = file.size() / DATA_SIZE;
    block_amount += file.size() % DATA_SIZE == 0 ? 0 : 1;
    //init socket
#ifdef _WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2), &wsadata) == SOCKET_ERROR) {
        qDebug() << "WSAStartup() failed!";
        return ;
    }
#endif
    int send_fd;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    struct pkt_t pkt;
    if ((send_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        qDebug() << "socket() failed!";
        return;
    }
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(FILE_R_PORT);
    client_addr.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());

    for (block_no = 0; block_no < block_amount; block_no++) {
        pkt.type = htonl(FILE_DATA);
        pkt.data.file_data.file_no = htonl(file_no);
        pkt.data.file_data.block_no = htonl(block_no);
        file.read(pkt.data.file_data.data, DATA_SIZE);
        sendto(send_fd,
               (const char *)&pkt,
               sizeof(pkt),
               0,
               (struct sockaddr*)&client_addr,
               client_addr_len);
    }

#ifdef _WIN32
    closesocket(send_fd);
    WSACleanup();
#else
    close(send_fd);
#endif
    file.close();
    emit send_finished();
}

file_sender::file_sender()
{
    stop.store(1);
    file_no = 0;
}

file_sender::~file_sender()
{
    stop_server();
}
void file_sender::start_server()
{
    //check if running
    if (!stop.load()) {
        return;
    }
    //start thread
    stop.store(0);
    start();
}

void file_sender::stop_server()
{
    if (stop.load()) {
        return;
    }
    stop.store(1);
}

void file_sender::init_socket()
{
#ifdef _WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2), &wsadata) == SOCKET_ERROR) {
        qDebug() << "WSAStartup() failed!";
        return ;
    }
#endif
    //init server fd
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        qDebug() << "socket() failed!";
        goto error;
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(FILE_S_PORT);
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        qDebug() << "bind() faild!";
        goto error;
    }
    return;
error:
    return;
}

void file_sender::run()
{
    init_socket();
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int recv_len;
    fd_set fd_read;
    struct timeval tv;
    while (true) {
        //thread control
        if (stop.load()) {
            break;
        }
        //select
        FD_ZERO(&fd_read);
        FD_SET(server_fd, &fd_read);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int ret = select(server_fd + 1,
                         &fd_read,
                         NULL,
                         NULL,
                         &tv);
        if (ret < 0) {
            qDebug() << "select() failed!";
            break;
        }
        if (ret == 0) {
            qDebug() << "select() timed out!";
            continue;
        }
        if (!FD_ISSET(server_fd, &fd_read)) {
            qDebug() << "server_fd not readable!";
            continue;
        }
        //receive packet
        recv_len = recvfrom(server_fd,
                             (char *)&pkt,
                             sizeof(pkt),
                             0,
                             (struct sockaddr*)&client_addr,
                             &client_addr_len);
        if (recv_len != sizeof(pkt)) {
            qDebug() << 'unknown data received.';
            continue;
        }
        pkt.type = ntohl(pkt.type);
        switch (pkt.type) {
            case FILE_ACK1:
                proc_file_ack1(inet_ntoa(client_addr.sin_addr), &pkt);
                break;
            case FILE_ACK2:
                proc_file_ack2(inet_ntoa(client_addr.sin_addr), &pkt);
                break;
            default:
                //error
                break;
        }
    }
    //close fd
#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
}

void file_sender::send_file(QString ip, QString file_path)
{
    int send_fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    struct pkt_t pkt;

    if ((send_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        qDebug() << "socket() failed!";
        return;
    }
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(FILE_R_PORT);
    client_addr.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());

    file_path.replace(QString("\\"), QString("/"));
    QFileInfo file_info(file_path);
    pkt.type = htonl(FILE_NEW);
    strcpy(pkt.data.file_new.file_name, file_info.fileName().toStdString().c_str());
    pkt.data.file_new.file_size = htonl((uint32_t)file_info.size());
    pkt.data.file_new.file_no = file_no;

    sendto(send_fd,
           (const char *)&pkt,
           sizeof(pkt),
           0,
           (struct sockaddr*)&client_addr,
           client_addr_len);
#ifdef _WIN32
    closesocket(send_fd);
#else
    close(send_fd);
#endif
    file_pending.insert(file_no, file_path);
    file_no++;
}


void file_sender::proc_file_ack1(QString ip, struct pkt_t *pkt)
{
    pkt->data.file_ack1.file_no = ntohl(pkt->data.file_ack1.file_no);
    if (!file_pending.keys().contains(pkt->data.file_ack1.file_no)) {
        emit file_denied(ip);
        return;
    }
    emit file_accepted(ip);
    send_file_data(ip, file_pending.value(pkt->data.file_ack1.file_no));
    file_pending.remove(pkt->data.file_ack1.file_no);
}

void file_sender::proc_file_ack2(QString ip, struct pkt_t *pkt)
{
    pkt->data.file_ack2.file_no = ntohl(pkt->data.file_ack2.file_no);
    emit file_trans_done(ip);
}

void file_sender::send_file_data(QString ip, QString path)
{
    QThread *worker_thread = new QThread(this);
    fs_worker *worker = new fs_worker;
    worker->moveToThread(worker_thread);
    connect(worker_thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &file_sender::operate_worker, worker, &fs_worker::do_send_data);
    connect(worker, &fs_worker::send_finished,
            [this, ip](){
                emit file_send_finished(ip);
            });
    connect(worker, &fs_worker::send_finished, worker_thread, &QThread::quit);
    worker_thread->start();
}
