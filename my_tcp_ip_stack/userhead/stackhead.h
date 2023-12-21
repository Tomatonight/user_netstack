#pragma once
#include "ipc.h"
#include <memory.h>
int us_init()
{
    IPC::init();
}
int us_socket(int domain, int type, int protocol)
{
    MSG_SOCKET msg = {.domain = domain, .type = type, .protocol = protocol};
    return IPC::process_socket(&msg);
}
void us_close(int sockfd)
{
    MSG_CLOSE msg = {.sockfd = sockfd};
    IPC::process_close(&msg);
    return;
}
int us_listen(int sockfd, int nb)
{
    // MSG_LISTEN msg={.sockfd=sockfd,.backoff=nb};
}
int us_write(int fd, char *buf, size_t len)
{

    MSG_WRITE msg = {.sockfd = fd, .len = len};
    return IPC::process_write(buf, &msg);
}
int us_read(int fd, char *buf, size_t len)
{

    MSG_READ msg = {.sockfd = fd, .len = len};
    return IPC::process_read(buf, &msg);
}
/*
int us_recvfrom(int fd,char* buf,uint16_t len,sockaddr_in addr)
{
MSG_RECVFROM msg={.sockfd=fd,.len=len,.addr=addr};

}
*/
int us_connect(int sockfd, sockaddr_in *addr)
{
    MSG_CONNECT msg = {.sockfd = sockfd};
    msg.sockfd = sockfd;
    memcpy(&msg.addr, addr, sizeof(sockaddr_in));
    return IPC::process_connect(&msg);
}
int us_bind(int sockfd, sockaddr_in *addr)
{
    MSG_BIND msg = {.sockfd = sockfd};
    msg.sockfd = sockfd;
    memcpy(&msg.addr, addr, sizeof(sockaddr_in));
    return IPC::process_bind(&msg);
}
int us_sendto(int sockfd, char *buf, size_t len, sockaddr_in *addr)
{

    MSG_SENDTO msg = {.sockfd = sockfd, .len = len};
    memcpy(&msg.addr, addr, len);
    return IPC::process_sendto(buf,&msg);
}
int us_recvfrom(int sockfd, char *buf, size_t len, sockaddr_in *addr)
{
    MSG_RECVFROM msg={.sockfd=sockfd,.len=len};
    if(!addr)msg.contain_addr=false;
    else msg.contain_addr=true;
    return IPC::process_recvfrom(buf,&msg);
}