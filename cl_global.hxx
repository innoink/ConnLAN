#ifndef CL_GLOBAL_HXX
#define CL_GLOBAL_HXX

#ifdef _MSC_VER
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>
#endif

#include <cstdint>

#define MSG_S_PORT 1415
#define MSG_R_PORT 9265
#define FILE_S_PORT 3589
#define FILE_R_PORT 7932

#define MSG_NEW 1
#define MSG_ACK 2
#define FILE_NEW 3
#define FILE_ACK1 4
#define FILE_DATA 5
#define FILE_ACK2 6

#define DATA_SIZE 400
#define FILE_NAME_SIZE 256

struct pkt_t {
    uint32_t type;
    union {
        //for new message -- MSG_NEW
        struct {
            uint32_t msg_no;
            char msg_data[DATA_SIZE];//terminate with '\0'
        } msg_new;
        //for message ack -- MSG_ACK
        struct {
            uint32_t msg_no;
            uint32_t is_ok;
        } msg_ack;
        //for file tras request -- FILE_NEW
        struct {
            char file_name[FILE_NAME_SIZE];//terminate with '\0'
            uint32_t file_size;
            uint32_t file_no;
        } file_new;
        //for file trans request ack -- FILE_ACK1
        struct {
            uint32_t file_no;
            uint32_t is_ok;
        } file_ack1;
        //for file trans data -- FILE_DATA
        struct {
            uint32_t file_no;
            uint32_t frag_no;
            char data[DATA_SIZE];
        } file_data;
        //for file trans data ack -- FILE_ACK2
        struct {
            uint32_t file_no;
            uint32_t is_ok;
        } file_ack2;
    } data;
};

//recvfrom:
//recvfrom以数据报为单位接收数据，放入缓存，如果缓存大小不够，则只存入前面的部分，后面的数据丢失。不可能存在一次读一半下次读另一半的情况。

#endif // CL_GLOBAL_HXX

