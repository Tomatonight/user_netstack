#include "socket.h"
#include"unistd.h"
sockets_manger *sockets_manger::sockets = nullptr;
void sockets_manger::clear_pid(int pid)
{
    std::unique_lock<std::shared_mutex> l(sockets->shared_mtx);
    std::map<int, class socket *> &m = sockets->sockets_map[pid];
    for (std::map<int, class socket *>::iterator it = m.begin(); it != m.end(); it++)
    {
        sockets_manger::remove_socket(it->second);
        if(it->second->sock)
        it->second->sock->close_();
        delete it->second;
    }
    m.clear();
   
}
void sockets_manger::init()
{
    sockets = new sockets_manger();
}
void sockets_manger::add_socket(class socket *sock)
{
    std::unique_lock<std::shared_mutex> l(sockets->shared_mtx);
    std::map<int, class socket *> &m = sockets->sockets_map[sock->pid];
    m[sock->fd] = sock;
}

void sockets_manger::remove_socket(class socket *sock)
{
    printf("a\n");
    int fd = sock->fd;
    int pid = sock->pid;
    std::unique_lock<std::shared_mutex> l(sockets->shared_mtx);
    std::map<int, class socket *> &m = sockets->sockets_map[sock->pid];
    m.erase(sock->fd);
   
    
}

class socket *sockets_manger ::search_socket(int pid, int fd)
{
    std::shared_lock<std::shared_mutex> l(sockets->shared_mtx);
    std::map<int, class socket *> &m = sockets->sockets_map[pid];
    std::map<int, class socket *>::iterator it = m.find(fd);
    if (it == m.end())
        return NULL;
    return it->second;
}
int sockets_manger::create_socket(int pid, int type)
{
    class socket *sk = new class socket(pid, type);
    sk->fd = gernerate_fd(pid);
    add_socket(sk);
    return sk->fd;
}

int sockets_manger::gernerate_fd(int pid)
{
    std::shared_lock<std::shared_mutex> l(sockets->shared_mtx);
    std::map<int, class socket *> &m = sockets->sockets_map[pid];
    while (true)
    {
        int fd = random() % 60000 + 1000;
        if (m.find(fd) == m.end())
        {
            m[fd] = NULL;
            return fd;
        }
    }
}