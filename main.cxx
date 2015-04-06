#include "widget.hxx"
#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<uint32_t>("uint32_t");
#ifdef _WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2,2), &wsadata) == SOCKET_ERROR) {
        qDebug() << "WSAStartup() failed!";
        return -1;
    }
#endif
    QApplication a(argc, argv);
    Widget w;
    w.show();
#ifdef _WIN32
    WSACleanup();
#endif
    return a.exec();
}
