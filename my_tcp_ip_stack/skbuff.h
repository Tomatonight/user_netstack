#pragma once
#include <mutex>
#include "sysheader.h"
#include "route.h"
#include "netdev.h"
#include <list>
class skbuff
{
public:
    skbuff(int size, bool recv_or_send) // true recv,false send
    {
        head = new char[size];
        memset(head, 0, size);
        if (recv_or_send)
        {
            data_start = head;
            data_end = head;
            end = head + size;
        }
        else
        {
            data_start = head + size;
            data_end = data_start;
            end = data_end;
        }
    }
    ~skbuff()
    {
        if (head)
           delete[] head;
    }
    class route_entry *route;
    uint16_t protocol;
    class netdev *dev;
    uint16_t data_len=0;
    uint32_t seq;
    char* tcp_start;
    char *head;
    char *end;
    char *data_start;
    char *data_end;

    void inline expand_s(uint16_t len)
    {
        data_start -= len;
    }
    void inline expand_r(uint16_t len)
    {
        data_end += len;
    }
};

class skbuff_head
{
public:
    skbuff_head()
    {
    }
    ~skbuff_head()
    {
        clear();
    }
    inline void add_skbuff(skbuff *skb)
    {
        std::unique_lock<std::mutex> l(mtx);
        lists.push_back(skb);
    }
    inline skbuff *pop_skbuff()
    {
        std::unique_lock<std::mutex> l(mtx);
        if (lists.empty())
            return nullptr;
        skbuff *re = lists.front();
   
        lists.pop_front();
        return re;
    }
    inline skbuff *reserve_skbuff()
    {
        std::unique_lock<std::mutex> l(mtx);
        if (lists.empty())
            return nullptr;
        return lists.front();
    }
    inline bool empty()
    {
        std::unique_lock<std::mutex> l(mtx);
        return lists.empty();
    }
    void clear()
    {
         
        std::unique_lock<std::mutex> l(mtx);
        for (skbuff *sk : lists)
        {
            if (sk)
                delete sk;
        }
        lists.clear();
    }
    void insert_skbuff_depend_seq(skbuff *skb)
    {
        std::unique_lock<std::mutex> l(mtx);
        std::list<skbuff *>::iterator i = lists.begin(), j;
        for (; i != lists.end();)
        {
            j = i++;
            if (i == lists.end() || (*i)->seq > skb->seq && (*j)->seq < skb->seq)
            {
                lists.insert(i, skb);
            }
        }
    }
friend class tcp_sock;
private:
    std::list<skbuff *> lists;
    std::mutex mtx;
};
#define FOR_EACH_SKBUFF(head, skb)\
std::unique_lock<std::mutex> l(head->mtx);\
for(std::list<skbuff *>::iterator i = head->lists.begin();i!=head->lists.end();i++)


class tcp_sock;
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