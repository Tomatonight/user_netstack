#pragma once
#include "sock.h"
#include "bitmap.h"
#include <map>
#define FD_BASE 3
#define MAX_FD_NUM 160
class socket
{
public:
    socket();
    int fd=0;
    sock* sk=nullptr;
};
class socket_manage
{
public:
    static void init();
    static socket_manage *get();
    int alloc_socket(int protocol);
    class socket* get_socket(int fd);
    void remove_socket(class socket* skt);
private:
    socket_manage();   
    bitmap fd_bitmap;
    std::map<int, class socket *> socket_map;
    static socket_manage *socket_manager;
};