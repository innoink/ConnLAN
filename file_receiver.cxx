#include "file_receiver.hxx"
#include <QDebug>

file_receiver::file_receiver()
{
    stop.store(1);
}

file_receiver::~file_receiver()
{
    stop_server();
}

void file_receiver::start_server()
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
    server_addr.sin_port = htons(MSG_R_PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        qDebug() << "bind() faild!";
        goto error;
    }
    //start thread
    stop.store(false);
    start();
    return;
error:
    return;
}

void file_receiver::stop_server()
{
    if (stop.load()) {
        return;
    }
    stop.store(1);
}

void file_receiver::run()
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
        //select()
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
                            &pkt,
                            sizeof(pkt),
                            0,
                            (struct sockaddr*)&client_addr,
                            &client_addr_len);
        if (recv_len != sizeof(pkt)) {
            qDebug() << "unknown data received.";
            continue;
        }
        //get ip
        QString ip(inet_ntoa(client_addr.sin_addr));
        //check packet type
        pkt.type = ntohl(pkt.type);
        switch (pkt.type) {
            case FILE_NEW:
                proc_file_new(ip, &pkt);
                break;
            case FILE_DATA:
                proc_file_data(ip, &pkt);
                break;
            default:
                //error
                break;
        }
    }
    //close fd
    close(server_fd);
}

void file_receiver::proc_file_new(QString ip, pkt_t *pkt)
{
    file_pending.insert(ip, ntohl(pkt->data.file_new.file_no));
    emit file_new(ip, QString(pkt->data.file_new.file_name), pkt->data.file_new.file_size);
}

void file_receiver::proc_file_data(QString ip, pkt_t *pkt)
{

}

void file_receiver::send_file_ack1()
{

}

void file_receiver::send_file_ack2()
{
}
