#include "ipc.h"
#include <memory.h>
#include <sys/socket.h>
#include <sys/un.h>
IPC *IPC::ipc = nullptr;
void IPC::init()
{
    static char *socket_path = "/tmp/socket";
    ipc = new IPC;
    sockaddr_un un;
    memset(&un, 0, sizeof(sockaddr_un));
    un.sun_family = AF_UNIX;
    strcpy(un.sun_path, socket_path);
    ipc->client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    std::cout << "init start " << std::endl;
    if (ipc->client_fd < 0)
    {
        std::cerr << "create unix socket err" << std::endl;
        return;
    }
    if (connect(ipc->client_fd, (sockaddr *)&un, sizeof(sockaddr)) < 0)
    {
        std::cerr << "connect with server err" << strerror(errno) << std::endl;
        return;
    }
    ipc->pid = getpid();
    sleep(1);
    std::cout << "init success " << std::endl;

    return;
}
int IPC::process_socket(MSG_SOCKET *msg)
{

    MSG *m = (MSG *)malloc(sizeof(MSG) + sizeof(MSG_SOCKET));
    m->msg_type = SOCKET;
    m->pid = ipc->pid;
    memcpy(m->data, msg, sizeof(MSG_SOCKET));

    int len = write(ipc->client_fd, m, sizeof(MSG) + sizeof(MSG_SOCKET));

    delete m;
    if (len < 0)
    {
        std::cerr << "socket write err" << std::endl;
    }

    MSG_RETURN re;
    len = read(ipc->client_fd, &re, sizeof(int));
    if (len < 0)
    {
        std::cerr << "socket read err" << std::endl;
    }
    return re.re;
}
int IPC::process_read(char *buf, MSG_READ *msg)
{
    MSG *m = (MSG *)malloc(sizeof(MSG) + sizeof(MSG_READ));
    m->msg_type = READ;
    m->pid = ipc->pid;
    memcpy(m->data, msg, sizeof(MSG_READ));
    int len = write(ipc->client_fd, m, sizeof(MSG) + sizeof(MSG_READ));
    if (len < 0)
    {
        std::cerr << "raed err" << std::endl;
    }
    MSG_RETURN re;
    read(ipc->client_fd, &re, sizeof(int));
    printf("read %d\n",re.re);
    if (re.re > 0)
        read(ipc->client_fd, buf, re.re);
        delete m;
    return re.re;
}
int IPC::process_write(char *buf, MSG_WRITE *msg)
{
    iovec iov[2];
    MSG *m = (MSG *)malloc(sizeof(MSG) + sizeof(MSG_WRITE));
    m->msg_type = WRITE;
    m->pid = ipc->pid;
    memcpy(m->data, msg, sizeof(MSG_WRITE));
    iov[0].iov_base = (char *)m;
    iov[0].iov_len = sizeof(MSG) + sizeof(MSG_WRITE);
    iov[1].iov_base = buf;
    iov[1].iov_len = msg->len;
    int len = writev(ipc->client_fd, iov, 2);
    delete m;
    if (len < 0)
    {
        std::cerr << "write err" << std::endl;
    }
    MSG_RETURN re;
    len = read(ipc->client_fd, &re, sizeof(int));
    if (len < 0)
    {
        std::cerr << "write err" << std::endl;
    }
    return re.re;
}
int IPC::process_bind(MSG_BIND *msg)
{
    MSG *m = (MSG *)malloc(sizeof(MSG) + sizeof(MSG_BIND));
    m->msg_type = BIND;
    m->pid = ipc->pid;
    memcpy(m->data, msg, sizeof(MSG_BIND));
    int len = write(ipc->client_fd, m, sizeof(MSG) + sizeof(MSG_BIND));
    delete m;
    if (len < 0)
    {
        std::cerr << "bind write err" << std::endl;
    }
    MSG_RETURN re;
    len = read(ipc->client_fd, &re, sizeof(int));
    if (len < 0)
    {
        std::cerr << "bind read err" << std::endl;
    }
    return re.re;
}
int IPC::process_connect(MSG_CONNECT *msg)
{
    MSG *m = (MSG *)malloc(sizeof(MSG) + sizeof(MSG_CONNECT));
    m->pid = ipc->pid;
    m->msg_type = CONNECT;
    memcpy(m->data, msg, sizeof(MSG_CONNECT));
    int len = write(ipc->client_fd, m, sizeof(MSG) + sizeof(MSG_CONNECT));
    delete m;
    if (len < 0)
    {
        std::cerr << "connect write err" << std::endl;
    }
    MSG_RETURN re;
    len = read(ipc->client_fd, &re, sizeof(int));
    if (len < 0)
    {
        std::cerr << "connect read err" << std::endl;
    }
    return re.re;
}
int IPC::process_close(MSG_CLOSE *msg)
{
    MSG *m = (MSG *)malloc(sizeof(MSG) + sizeof(MSG_CLOSE));
    m->pid = ipc->pid;
    m->msg_type = CLOSE;
    memcpy(m->data, msg, sizeof(MSG_CLOSE));

    int len = write(ipc->client_fd, m, sizeof(MSG) + sizeof(MSG_CLOSE));

    delete m;
    if (len < 0)
    {
        std::cerr << "close write err" << std::endl;
    }
    return 0;
}
int IPC::process_sendto(char *buf, MSG_SENDTO *msg)
{
    iovec iov[2];
    MSG *m = (MSG *)malloc(sizeof(MSG) + sizeof(MSG_SENDTO));
    m->pid = ipc->pid;
    m->msg_type = SENDTO;
    memcpy(m->data, msg, sizeof(MSG_SENDTO));
    iov[0].iov_base = m;
    iov[0].iov_len = sizeof(MSG) + sizeof(MSG_SENDTO);
    iov[1].iov_base = buf;
    iov[1].iov_len = msg->len;
    int len = writev(ipc->client_fd, iov, 2);
    if (len < 0)
    {
        std::cerr << "sendto write err" << std::endl;
    }
    MSG_RETURN re;
    len = read(ipc->client_fd, &re, sizeof(int));
    if (len < 0)
    {
        std::cerr << "sendto read err" << std::endl;
    }
    return len;
}
int IPC::process_recvfrom(char *buf, MSG_RECVFROM *msg)
{
    MSG *m = (MSG *)malloc(sizeof(MSG) + sizeof(MSG_RECVFROM));
    m->pid = ipc->pid;
    m->msg_type = RECVFROM;
    memcpy(m->data, msg, sizeof(MSG_RECVFROM));
    MSG_RETURN re;
    int len;
    if (msg->contain_addr)
    {
    }
    else
    {
    }
}