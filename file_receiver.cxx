#include "file_receiver.hxx"
#include <QDebug>
#include <cstdio>
#include <cstdlib>

write_task::write_task(QString file_name, QByteArray *ba)
{
    this->file_name = file_name;
    this->ba = ba;
}

void write_task::run()
{
    QFile file(file_name);
    file.open(QIODevice::WriteOnly);
    file.write(*ba);
    file.close();
    delete ba;
}

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

void file_receiver::init_socket()
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
    server_addr.sin_port = htons(FILE_R_PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        qDebug() << "bind() faild!";
        goto error;
    }
    return;
error:
    return;
}

void file_receiver::run()
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
#ifdef _WIN32
    closesocket(server_fd);
    WSACleanup();
#else
    close(server_fd);
#endif
}

void file_receiver::proc_file_new(QString ip, pkt_t *pkt)
{
    pkt->data.file_new.file_no = ntohl(pkt->data.file_new.file_no);
    pkt->data.file_new.file_size = ntohl(pkt->data.file_new.file_size);
    uint32_t blocks = pkt->data.file_new.file_size / DATA_SIZE;
    blocks += pkt->data.file_new.file_size % DATA_SIZE == 0 ? 0 : 1;
    file_pending.insert(ip, pkt->data.file_new.file_no);
    file_blocks.insert(QPair<QString, uint32_t>(ip, pkt->data.file_new.file_no), new QBitArray(blocks, false));
    file_data.insert(QPair<QString, uint32_t>(ip, pkt->data.file_new.file_no), new QByteArray(pkt->data.file_new.file_size, 0));
    emit file_new(ip, QString(pkt->data.file_new.file_name), pkt->data.file_new.file_no, pkt->data.file_new.file_size);
}

void file_receiver::proc_file_data(QString ip, pkt_t *pkt)
{
    pkt->data.file_data.file_no = ntohl(pkt->data.file_data.file_no);
    uint32_t block = ntohl(pkt->data.file_data.block_no);
    if (!file_pending.contains(ip, pkt->data.file_data.file_no)) {
        return;
    }
    file_blocks.value(QPair<QString, uint32_t>(ip, pkt->data.file_data.file_no))->setBit(block, true);
    QByteArray *ba = file_data.value(QPair<QString, uint32_t>(ip, pkt->data.file_data.file_no));
    uint32_t data_ptr = 0;
    data_ptr = block * DATA_SIZE;
    uint32_t file_size = ba->size();
    uint32_t size = (block == file_blocks.value(QPair<QString, uint32_t>(ip, pkt->data.file_data.file_no))->size() - 1 ? file_size % DATA_SIZE : DATA_SIZE);
    memcpy(ba->data(), pkt->data.file_data.data, size);
    for (int i = 0; i < file_blocks.value(QPair<QString, uint32_t>(ip, pkt->data.file_data.file_no))->size(); i++) {
        if (file_blocks.value(QPair<QString, uint32_t>(ip, pkt->data.file_data.file_no))->at(i) == false) {
            return;
        }
    }
    write_task *wt = new write_task(file_name.value(QPair<QString, uint32_t>(ip, pkt->data.file_data.file_no)), ba);
    QThreadPool::globalInstance()->start(wt);
    send_file_ack2(ip, pkt->data.file_data.file_no, true);
}

void file_receiver::send_file_ack1(QString ip, uint32_t file_no, bool is_ok)
{
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
    client_addr.sin_port = htons(FILE_S_PORT);
    client_addr.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());

    pkt.type = htonl(FILE_ACK1);
    pkt.data.file_ack1.file_no = htonl(file_no);
    pkt.data.file_ack1.is_ok   = htonl(is_ok ? 1 : 0);
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
}

void file_receiver::send_file_ack2(QString ip, uint32_t file_no, bool is_ok)
{
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
    client_addr.sin_port = htons(FILE_S_PORT);
    client_addr.sin_addr.s_addr = inet_addr(ip.toStdString().c_str());

    pkt.type = htonl(FILE_ACK2);
    pkt.data.file_ack1.file_no = htonl(file_no);
    pkt.data.file_ack1.is_ok   = htonl(is_ok ? 1 : 0);
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
}

void file_receiver::set_save_file_name(QString ip, uint32_t fno, QString name)
{
    file_name.insert(QPair<QString, uint32_t>(ip, fno), name);

}
