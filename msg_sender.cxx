#include <QDebug>
#include "msg_sender.hxx"

msg_sender::msg_sender()
{
    stop.store(1);
    msg_no = 0;
}

msg_sender::~msg_sender()
{
    stop_server();
}

void msg_sender::start_server()
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
    server_addr.sin_port = htons(MSG_S_PORT);

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

void msg_sender::stop_server()
{
    if (stop.load()) {
        return;
    }
    stop.store(1);
}

void msg_sender::run()
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
            qDebug() << 'unknown data received.';
            continue;
        }
        //check packet type
        pkt.type = ntohl(pkt.type);
        if (pkt.type != MSG_ACK) {
            qDebug() << "unknown head type.";
            continue;
        }
        //check result
        if(!ntohl(pkt.data.msg_ack.is_ok)) {
            emit msg_denied();
            continue;
        }
        //check msg no
        if (!msg_pending.contains(ntohl(pkt.data.msg_ack.msg_no))) {
            qDebug() << "invalid msg_no";
            continue;
        }
        msg_pending.remove(ntohl(pkt.data.msg_ack.msg_no));
        //finally, message accepted
        emit msg_accepted();
    }
    //close fd
    close(server_fd);
}

void msg_sender::send_msg(QString ip, QString msg)
{
    int send_fd;
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);
    struct pkt_t pkt;

    if ((send_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        qDebug() << "socket() failed!";
        return;
    }
    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(MSG_R_PORT);
    client_addr.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());

    pkt.type = htonl(MSG_NEW);
    pkt.data.msg_new.msg_no = htonl(msg_no);
    memcpy(pkt.data.msg_new.msg_data, msg.toStdString().c_str(), msg.length() + 1);

    sendto(send_fd,
           &pkt,
           sizeof(pkt),
           0,
           (struct sockaddr*)&client_addr,
           client_addr_len);
    close(send_fd);
    msg_pending.insert(msg_no);
    msg_no++;
}
