#include "ipc.h"
#include "tcp.h"
#include <map>
void IPC::recv_fd_loop()
{
    const char *socket_path = "/tmp/socket";
    unlink(socket_path);
    sockaddr_un un;
    bzero(&un, sizeof(sockaddr_un));
    int sock_fd;
    strcpy(un.sun_path, socket_path);
    un.sun_family = AF_UNIX;
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    std::map<int, std::thread> threads;
    if (sock_fd < 0)
    {
        std::cerr << "create socket error" << std::endl;
        exit(-1);
    }
    if (bind(sock_fd, (sockaddr *)&un, sizeof(sockaddr)) < 0)
    {
        std::cerr << "bind error " << std::endl;
        exit(-1);
    }
    if (listen(sock_fd, 20) < 0)
    {
        std::cerr << "listen error" << std::endl;
        exit(-1);
    }
 
    while (true)
    {
        int fd = accept(sock_fd, NULL, NULL);
        threads.insert({fd, std::thread(create_thread, fd)});
    }
    close(sock_fd);
    unlink(socket_path);
    return;
}
void IPC::create_thread(int client_fd)
{
    int max_len = 2000;
    char *buf = new char[max_len];
    int re;
    int pid = 0;
    printf("-------------------------start %d\n", client_fd);
    while (re = (read(client_fd, buf, max_len)) > 0)
    {

        MSG *msg = (MSG *)(buf);
        switch (msg->msg_type)
        {
        case SOCK:

            process_create_socket(client_fd, msg->pid, (MSG_SOCKET *)msg->data);
            break;
        case CONNECT:
            process_connect(client_fd, msg->pid, (MSG_CONNECT *)msg->data);
            break;
        case READ:
            process_read(client_fd, msg->pid, (MSG_READ *)msg->data);
            break;
        case WRITE:
            process_write(client_fd, msg->pid, (MSG_WRITE *)msg->data);
            break;
        case CLOSE:
            process_close(client_fd, msg->pid, (MSG_CLOSE *)msg->data);
            break;
        case BIND:
            process_bind(client_fd, msg->pid, (MSG_BIND *)msg->data);
            break;
        case ACCEPT:

            break;
        case LISTEN:
            break;
        case SENDTO:
            process_sendto(client_fd, msg->pid, (MSG_SENDTO *)msg->data);
            break;
        case RECVFROM:
            process_recvfrom(client_fd, msg->pid, (MSG_RECVFROM *)msg->data);
            break;
        default:
            break;
        }
    }
    delete buf;
    if (pid)
    {
        sockets_manger::clear_pid(pid);
    }
    printf("--------------------------exit\n");
    return;
}
void IPC::process_read(int client_fd, int pid, MSG_READ *msg)
{

    class socket *skt = sockets_manger::search_socket(pid, msg->sockfd);

    if (!skt||skt->sock==nullptr)
    {
      
        int re = -1;
        write(client_fd, &re, sizeof(int));
        return;
    }
  
    if (skt->sock->protocol == IP_TCP)
    { 
        sock* sock=skt->sock;
        std::function<int()> task = [&]() -> int
        {
            
            MSG_RETURN *re = (MSG_RETURN *)malloc(sizeof(int) + msg->len);
            memset(re, 0, sizeof(int) + msg->len);
            int rt = sock->read_((char *)re + sizeof(int), msg->len);

            if (rt == 0 || rt == -1)
            {
                re->re=rt;
                write(client_fd, re, sizeof(int));
                sock->read_task = nullptr;
            }
            else if (rt == -2)
            {
             sock->read_task = task;
            }
            else
            {
                printf("write %d\n",rt);
                re->re = rt;
                write(client_fd, (char *)re, rt + sizeof(int));
                 sock->read_task = nullptr;
            }
           
             free(re);
            return 0;
        };
        sock->read_task = std::bind(task);
        sock->read_task();
        return;
    }
    // udp
     
    std::function<int()> task = [=]()
    {
        MSG_RETURN *re = (MSG_RETURN *)malloc(sizeof(int) + msg->len);
        memset(re, 0, sizeof(int) + msg->len);
        int len = skt->sock->read_((char *)(re) + sizeof(int), msg->len);
        re->re = len;
        write(client_fd, re, sizeof(int) + len);
         skt->sock->read_task = nullptr;
        delete re;
        return 0;
    };
    if (skt->sock->recv_queue.empty())
    {

        skt->sock->read_task = (task);
        return;
    }

    task();
}
void IPC::process_write(int client_fd, int pid, MSG_WRITE *msg)
{
    MSG_RETURN re;

    class socket *skt = sockets_manger::search_socket(pid, msg->sockfd);
    if (!skt||skt->sock==nullptr)
    {

        MSG_RETURN re;
        re.re = -1;
        write(client_fd, &re, sizeof(MSG_RETURN));
        return;
    }
    re.re = skt->sock->write_((char *)msg->buf, msg->len);
    write(client_fd, &re, sizeof(MSG_RETURN));
    return;
}

void IPC::process_bind(int client_fd, int pid, MSG_BIND *msg)
{
   
    MSG_RETURN re;
    class socket *skt = sockets_manger::search_socket(pid, msg->sockfd);
    if (!skt||skt->sock==nullptr)
    {
        re.re = -1;

        write(client_fd, &re, sizeof(MSG_RETURN));
        return;
    }
    re.re = skt->sock->bind_(&msg->addr);

    write(client_fd, &re, sizeof(MSG_RETURN));
    return;
}
void IPC::process_connect(int client_fd, int pid, MSG_CONNECT *msg)
{

    MSG_RETURN re;
    class socket *skt = sockets_manger::search_socket(pid, msg->sockfd);
    if (!skt||skt->sock==nullptr)
    {
        re.re = -1;
        write(client_fd, &re, sizeof(MSG_RETURN));
        return;
    }
    if (skt->sock->protocol == IP_TCP)
    {
        
        std::function<int()> task = [=]() -> int
        {
        
            MSG_RETURN r;
            if (skt->sock->state == TCP_ESTABLISHED)
            {
             
                r.re = 0;
                write(client_fd, &r, sizeof(MSG_RETURN));
            }
            else
            {
                r.re = -1;
                write(client_fd, &r, sizeof(MSG_RETURN));
                
            }
            skt->sock->read_task=nullptr;
            return 0;
        };
        skt->sock->read_task=task;
        skt->sock->connect_(&msg->addr);
        return;
    }
    // udp
    re.re = skt->sock->connect_(&msg->addr);
    write(client_fd, &re, sizeof(MSG_RETURN));

    return;
}
void IPC::process_create_socket(int client_fd, int pid, MSG_SOCKET *msg)
{
    MSG_RETURN re;
    if (msg->domain != AF_INET || (msg->protocol != IP_UDP && msg->protocol != IP_TCP && msg->protocol != 0) || msg->type != SOCK_STREAM && msg->type != SOCK_DGRAM)
    {
        re.re = -1;
        write(client_fd, &re, sizeof(MSG_RETURN));
        return;
    }
    re.re = sockets_manger::create_socket(pid, msg->type);
    write(client_fd, &re, sizeof(MSG_RETURN));

    return;
}
void IPC::process_close(int client_fd, int pid, MSG_CLOSE *msg)
{
 
    MSG_RETURN re;
    class socket *sk = sockets_manger::search_socket(pid, msg->sockfd);
    if(!sk)return;
    sockets_manger::remove_socket(sk);
    if(sk->sock==nullptr)
    {
        delete sk;
        return;
    }
    sk->sock->skt=nullptr;
    sk->sock->close_();
    sk->sock=nullptr;
    delete sk;
}
void IPC::process_sendto(int client_fd, int pid, MSG_SENDTO *msg)
{
    MSG_RETURN re;
    class socket *skt = sockets_manger::search_socket(pid, msg->sockfd);
    if (!skt||skt->sock==nullptr)
    {
        re.re = -1;
        write(client_fd, &re, sizeof(MSG_RETURN));
        return;
    }
    re.re = skt->sock->sendto_((char *)(msg->buf), msg->len, &msg->addr);
    write(client_fd, &re, sizeof(MSG_RETURN));
    return;
}
void IPC::process_recvfrom(int client_fd, int pid, MSG_RECVFROM *msg)
{

    class socket *skt = sockets_manger::search_socket(pid, msg->sockfd);
    if (!skt||skt->sock==nullptr)
    {
        MSG_RETURN re;
        re.re = -1;
        write(client_fd, &re, sizeof(MSG_RETURN));
        return;
    }
    std::function<int()> task = [=]()
    {
        MSG_RETURN *re = (MSG_RETURN *)malloc(sizeof(int) + sizeof(sockaddr) + msg->len);
        sockaddr_in addr;
        re->re = skt->sock->recvfrom_((char *)(re) + sizeof(sockaddr), msg->len, &addr);
        write(client_fd, &re, re->re + sizeof(int) + sizeof(sockaddr) + re->re);
        delete re;
        return 0;
    };
    if (skt->sock->recv_queue.empty())
    {

        skt->sock->read_task = (task);
        return;
    }
    task();
}
void process_listen(int client_fd, int pid, MSG_LISTEN *msg)
{
    class socket *skt = sockets_manger::search_socket(pid, msg->sockfd);
    MSG_RETURN re;
    if (!skt||skt->sock==nullptr)
    {

        re.re = -1;
        write(client_fd, &re, sizeof(MSG_RETURN));
        return;
    }
    re.re = skt->sock->listen_(msg->backoff);
    write(client_fd, &re, sizeof(MSG_RETURN));
    return;
}
void process_accept(int client_fd, int pid, MSG_ACCEPT *msg)
{
}