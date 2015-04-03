#include "widget.hxx"
#include <QApplication>

int main(int argc, char *argv[])
{
#ifdef WIN32
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
        qDebug() << "WSAStartup() failed!";
        return -1;
    }
#endif
    QApplication a(argc, argv);
    Widget w;
    w.show();

    return a.exec();
}
