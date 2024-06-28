#include "interface.h"
#include "socket.h"
#include "udp.h"
#include "tcp.h"
#include "timer.h"
#include "netdev.h"
#include "route.h"
#include <thread>

void init()
{
    printf("init start\n");
    pcap::pcap_init();
    netdev::netdev_init();
    route_entry_control::route_entrys_init();
    udp_manage::init();
    tcp_manage::init();
    timer::init();
    arp_entry_control::init();
    socket_manage::init();
    netdev::get()->recv_thread = std::move(std::thread([]()
                                                       { netdev::get()->netdev_recv_loop(); }));
    printf("init done\n");
}
int socket_(int domain, int type, int protocol)
{
    if (domain != AF_INET || (type != SOCK_STREAM && type != SOCK_DGRAM))
        return -1;
    switch (protocol)
    {
    case IPPROTO_TCP:
    {
        int new_fd = socket_manage::get()->alloc_socket(IPPROTO_TCP);
        return new_fd;
    }
    case IPPROTO_UDP:
    {

        int new_fd = socket_manage::get()->alloc_socket(IPPROTO_UDP);
        return new_fd;
    }
    default:
        return -1;
    }
    return 0;
}
int connect_(int fd, sockaddr_in *addr)
{

    class socket *skt = socket_manage::get()->get_socket(fd);
    if (!skt||!skt->sk)
    {
        return -1;
    }
    return skt->sk->connect_(addr);
}
int write_(int fd, char *buf, int len)
{
    class socket *skt = socket_manage::get()->get_socket(fd);
    if (!skt||!skt->sk)
    {
        return -1;
    }
    return skt->sk->write_(buf, len);
}
void close_(int fd)
{
    class socket *skt = socket_manage::get()->get_socket(fd);
    if (!skt||!skt->sk)
    {
        return;
    }
    skt->sk->close_();
    socket_manage::get()->remove_socket(skt);
}
int read_(int fd, char *buf, int len)
{
    class socket *skt = socket_manage::get()->get_socket(fd);
    if (!skt||!skt->sk)
    {
        return -1;
    }
    return skt->sk->read_(buf, len);
}
int bind_(int fd, sockaddr_in *addr)
{
    class socket *skt = socket_manage::get()->get_socket(fd);
    if (!skt||!skt->sk)
    {
        return -1;
    }
    return skt->sk->bind_(addr);
}
int sendto_(int fd, char *buf, int len, sockaddr_in *addr)
{
    class socket *skt = socket_manage::get()->get_socket(fd);
    if (!skt||!skt->sk)
    {
        return -1;
    }
    return skt->sk->sendto_(buf, len, addr);
}
int recvfrom_(int fd, char *buf, int len, sockaddr_in *addr)
{
    class socket *skt = socket_manage::get()->get_socket(fd);
    if (!skt||!skt->sk)
    {
        return -1;
    }
    return skt->sk->recvfrom_(buf, len, addr);
}
int listen_(int fd)
{
    class socket *skt = socket_manage::get()->get_socket(fd);
    if (!skt||!skt->sk)
    {
        return -1;
    }
}
int accpet_(int fd)
{
    class socket *skt = socket_manage::get()->get_socket(fd);
    if (!skt||!skt->sk)
    {
        return -1;
    }
}
void exit()
{
    timer::get()->timer_thread.detach();
    netdev::get()->recv_thread.detach();
}