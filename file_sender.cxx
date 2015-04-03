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

void fs_worker::send_data(QString ip, QString file_path)
{


}

file_sender::file_sender()
{
    stop.store(0);
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
    //start thread
    stop.store(0);
    start();
    return;
error:
    return;
}

void file_sender::stop_server()
{
    if (stop.load()) {
        return;
    }
    stop.store(1);
}

void file_sender::run()
{
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    int recv_len;
    int send_len;
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
                //handle FILE_ACK1
                break;
            case FILE_ACK2:
                //handle FILE_ACK2
                break;
            default:
                //error
                break;
        }
    }
    //close fd
#ifdef _WIN32
    closesocket(server_fd);
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
    client_addr.sin_port = htons(MSG_R_PORT);
    client_addr.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());

    file_path.replace(QString("\\"), QString("/"));
    QFile file(file_path);
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
    file_pending.insert(file_no);
    file_no++;
}
