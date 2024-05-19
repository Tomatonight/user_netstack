#pragma once
#include<arpa/inet.h>
void init();
int socket_(int domain,int type,int protocol);
int connect_(int fd,sockaddr_in *addr);
int write_(int fd,char *buf, int len);
void close_(int fd);
int read_(int fd,char *buf, int len);
int bind_(int fd,sockaddr_in *);
int sendto_(int fd,char *buf, int len, sockaddr_in *addr);
int recvfrom_(int fd,char *buf, int len, sockaddr_in *addr);
int listen_(int fd);
int accpet_(int fd);
void exit();