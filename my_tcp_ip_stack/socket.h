#pragma once
#include <map>
#include <set>
#include <shared_mutex>
#include <mutex>
#include "ip.h"
#include "udp.h"
#include"tcp.h"
#include "sock.h"
class sockets_manger
{
public:
    static sockets_manger *sockets;
    static void init();
    static int create_socket(int pid, int protocol);
    static void remove_socket(class socket *sock);

    static class socket *search_socket(int pid, int fd);
    static int gernerate_fd(int pid);
    static void clear_pid(int pid);
    std::shared_mutex shared_mtx;
private:
    static void add_socket(class socket *sock);
    std::map<int, std::map<int, class socket *>> sockets_map;

    
};
class socket
{
public:
    socket(int pid_, int type) : pid(pid_)
    {

        fd = sockets_manger::gernerate_fd(pid);
        switch (type)
        {
        case SOCK_DGRAM:
            sock = (new sock_udp(this));
            break;
        case SOCK_STREAM:
            sock=new tcp_sock;
            sock->skt=this;
            break;
        }
    }
    ~socket()
    {
        delete sock;
    };
    class sock *sock;
    int pid;
    int fd;
};
