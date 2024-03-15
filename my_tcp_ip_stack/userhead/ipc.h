#pragma once
#include <unistd.h>
#include <arpa/inet.h>
#include<iostream>
#include<sys/uio.h>
enum MSG_TYPE
{
    SOCKET =0,
    CONNECT,
    READ,
    WRITE,
    CLOSE,
    BIND,
    ACCEPT,
    LISTEN,
    SENDTO,
    RECVFROM,
};

struct MSG
{
    MSG_TYPE msg_type;
    pid_t pid;
    char data[];
} __attribute__((packed));
struct MSG_RETURN
{
    int re;

}__attribute__((packed));
struct MSG_CONNECT
{
    int sockfd;
    sockaddr_in addr;
}__attribute__((packed));
struct MSG_SOCKET
{
    int domain;
    int type;
    int protocol;
} __attribute__((packed));
struct MSG_ACCEPT
{
    int sockfd;
    int contain_addr; /* 是否需要包含地址信息 */
    struct sockaddr_in addr;
} __attribute__((packed));
struct MSG_RECVFROM
{
    int sockfd;
    size_t len;
    int contain_addr; /* 是否包含了地址信息 */
    uint8_t buf[];
} __attribute__((packed));
struct MSG_BIND
{
    int sockfd;
    struct sockaddr_in addr;
} __attribute__((packed));
struct MSG_SENDTO
{
    int sockfd;
    size_t len;
    struct sockaddr_in addr;
    uint8_t buf[];
} __attribute__((packed));
struct MSG_WRITE
{
    int sockfd;
    size_t len;
    uint8_t buf[];
} __attribute__((packed));
struct MSG_READ
{
    int sockfd;
    size_t len;
    uint8_t buf[];
} __attribute__((packed));
struct MSG_CLOSE
{
    int sockfd;
} __attribute__((packed));
struct MSG_LISTEN
{
    int sockfd;
    int backoff;
} __attribute__((packed));
class IPC
{
public:
    IPC()
    {
    }
    ~IPC()
    {
    }
    static void init();
    static int process_socket(MSG_SOCKET *msg);
    static int process_read(char* buf,MSG_READ *msg);
    static int process_write(char* buf,MSG_WRITE *msg);
    static int process_bind( MSG_BIND *msg);
    static int process_connect(MSG_CONNECT *msg);
    static int process_close(MSG_CLOSE *msg);
    static int process_sendto(char* buf,MSG_SENDTO *msg);
    static int process_recvfrom(char* buf,MSG_RECVFROM *msg);
private:
    static IPC *ipc;
    int pid;
    int client_fd = 0;
};
