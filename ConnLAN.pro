#-------------------------------------------------
#
# Project created by QtCreator 2015-03-25T18:08:12
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ConnLAN
TEMPLATE = app
CONFIG += c++11


SOURCES += main.cxx\
        widget.cxx \
    msg_sender.cxx \
    msg_receiver.cxx \
    file_sender.cxx \
    file_receiver.cxx

HEADERS  += widget.hxx \
    msg_sender.hxx \
    msg_receiver.hxx \
    file_sender.hxx \
    file_receiver.hxx \
    cl_global.hxx

LIBS += -lpthread

win32: {
    LIBS += -lws2_32
}
