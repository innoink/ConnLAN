#include <QDebug>

#include "msg_receiver.hxx"

msg_receiver::msg_receiver()
{
    stop.store(1);
}

msg_receiver::~msg_receiver()
{
    stop_server();
}

void msg_receiver::start_server()
{
    //check if running
    if (!stop.load()) {
        return;
    }
    //start thread
    stop.store(false);
    start();
    return;
error:
    return;
}

void msg_receiver::stop_server()
{
    if (stop.load()) {
        return;
    }
    stop.store(1);
}

void msg_receiver::init_socket()
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
    server_addr.sin_port = htons(MSG_R_PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        qDebug() << "bind() faild!";
        goto error;
    }
    return;
error:
    return;
}

void msg_receiver::run()
{
    init_socket();
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
                            (char *)&pkt,
                            sizeof(pkt),
                            0,
                            (struct sockaddr*)&client_addr,
                            &client_addr_len);
        if (recv_len != sizeof(pkt)) {
            qDebug() << "unknown data received.";
            continue;
        }
        //check packet type
        pkt.type = ntohl(pkt.type);
        if (pkt.type != MSG_NEW) {
            qDebug() << "unknown head type.";
            continue;
        }
        //get message
        QString msg(pkt.data.msg_new.msg_data);
        //get client ip
        QString ip(inet_ntoa(client_addr.sin_addr));
        //emit new message
        emit msg_received(ip, msg);
        //send MSG_ACK
        struct pkt_t pkt_ack;
        pkt_ack.type = htonl(MSG_ACK);
        pkt_ack.data.msg_ack.msg_no = pkt.data.msg_new.msg_no;
        pkt_ack.data.msg_ack.is_ok  = htonl(1);
        client_addr.sin_port = htons(MSG_S_PORT);
        send_len = sendto(server_fd,
                          (const char *)&pkt_ack,
                          sizeof(pkt_ack),
                          0,
                          (struct sockaddr*)&client_addr,
                          client_addr_len);
        if (send_len != sizeof(pkt_ack)) {
            qDebug() << "send msg ack failed!";
            continue;
        }
    }
    //close fd
#ifdef _WIN32
    closesocket(server_fd);
#else
    close(server_fd);
#endif
}
