#pragma once
#include <unistd.h>
#include <thread>
#include <iostream>
#include "sysheader.h"
#include "socket.h"

enum MSG_TYPE
{
    SOCK = 0,
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
    char *data[];
} __attribute__((packed));
struct MSG_CONNECT
{
    int sockfd;
    sockaddr_in addr;
} __attribute__((packed));
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
    static void recv_fd_loop();

private:
    static void create_thread(int client_fd);
    static void process_read(int client_fd, int pid, MSG_READ *msg);
    static void process_write(int client_fd, int pid, MSG_WRITE *msg);
    static void process_bind(int client_fd, int pid, MSG_BIND *msg);
    static void process_connect(int client_fd, int pid, MSG_CONNECT *msg);
    static void process_create_socket(int client_fd, int pid, MSG_SOCKET *msg);
    static void process_close(int client_fd, int pid, MSG_CLOSE *msg);
    static void process_sendto(int client_fd, int pid, MSG_SENDTO *msg);
    static void process_recvfrom(int client_fd, int pid, MSG_RECVFROM *msg);
    static void process_listen(int client_fd, int pid, MSG_LISTEN *msg);
    static void process_accept(int client_fd, int pid, MSG_ACCEPT *msg);
};
