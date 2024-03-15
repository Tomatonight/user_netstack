#include"tcp.h"
class sock_queue
{
public:
    sock_queue()
    {
    }
    ~sock_queue()
    {
    }
    inline tcp_sock *pop_sock()
    {
        std::unique_lock<std::mutex> l(mtx);
        if (lists.empty())
            return NULL;
        tcp_sock *re = lists.front();
        lists.pop_front();
        return re;
    }
    inline void add_sock(tcp_sock *sock)
    {
        std::unique_lock<std::mutex> l(mtx);
        lists.push_back(sock);
    }

private:
    std::mutex mtx;
    std::list<tcp_sock *> lists;
};