#include "socket.h"
#include "udp.h"
#include "tcp.h"
socket_manage *socket_manage::socket_manager = nullptr;
socket_manage::socket_manage() : fd_bitmap(MAX_FD_NUM, FD_BASE)
{
}
void socket_manage::init()
{
    if (!socket_manager)
    {
        socket_manager = new socket_manage;
    }
}
socket_manage *socket_manage::get()
{
    return socket_manager;
}
socket::socket()
{
}
int socket_manage::alloc_socket(int protocol)
{
    int new_fd = fd_bitmap.alloc_value();
    class socket *new_socket = new class socket();
    if (protocol == IP_UDP)
    {
        new_socket->sk = (new sock_udp);
        new_socket->sk->protocol = IP_UDP;
    }
    else
    {
        new_socket->sk = (new tcp_sock);
        new_socket->sk->protocol = IP_TCP;
    }
    new_socket->fd=new_fd;
    socket_map[new_fd] = new_socket;
    return new_fd;
}
class socket *socket_manage::get_socket(int fd)
{
    if (socket_map.find(fd) == socket_map.end())
    {
        return nullptr;
    }
    return socket_map[fd];
}
void socket_manage::remove_socket(class socket *sock)
{
    fd_bitmap.free_value(sock->fd);
    socket_map.erase(sock->fd);
    delete sock;
}