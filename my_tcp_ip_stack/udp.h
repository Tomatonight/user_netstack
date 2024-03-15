#pragma once
#include "sock.h"
#include "ether.h"
#include <unistd.h>
#include <map>
#include "ip.h"
#define UDP_UNCONNECT 0x1
#define UDP_CONNECT 0x2
#define UDP_HDR_LEN sizeof(udp_hdr)
class sock_udp : public sock
{
public:
    sock_udp(class socket *skt_); //: sock(IP_UDP,skt_)
    ~sock_udp();
    std::mutex delete_mtx;

private:
    int connect_(sockaddr_in *addr);
    int write_(char *buf, int len);
    int read_(char *buf, int len);
    int bind_(sockaddr_in *);
    void close_();
    int sendto_(char *buf, int len, sockaddr_in *addr);
    int recvfrom_(char *buf, int len, sockaddr_in *addr);
};
class udp_sock_manage
{

public:
    udp_sock_manage()
    {
    }
    ~udp_sock_manage()
    {
    }
    static inline void init()
    {
        udp_manage = new udp_sock_manage();
    }
    static sock_udp *search_udp_sock(int port);
    std::map<int, class sock_udp *> used_ports;
    std::mutex udp_mtx;
    static udp_sock_manage *udp_manage;
    static int generate_udp_port_and_bind(class sock_udp *skt)
    {
        std::unique_lock<std::mutex> l(udp_manage->udp_mtx);
        while (true)
        {
            int port = random() % 50000 + 2000;
            if (udp_manage->used_ports.find(port) == udp_manage->used_ports.end())
            {
                skt->sport = port;
                udp_manage->used_ports[port] = skt;
                return port;
            }
        }
    }
    static int bind_udp_port(int port, class sock_udp *skt)
    {
        std::unique_lock<std::mutex> l(udp_manage->udp_mtx);
        if (udp_manage->used_ports.find(port) != udp_manage->used_ports.end())
            return -1;
        udp_manage->used_ports[port] = skt;
        return 0;
    }
    static void erase_udp_port(int port)
    {
        std::unique_lock<std::mutex> l(udp_manage->udp_mtx);
        udp_manage->used_ports.erase(port);
    }
};
class udp_hdr
{
public:
    uint16_t sport; /* 源端口				*/
    uint16_t dport; /* 目的端口			*/
    uint16_t len;   /* 长度,包括首部和数据 */
    uint16_t csum;  /* 检验和				*/
    uint8_t data[];
    static inline udp_hdr *udp_header(skbuff *skb)
    {
        return (udp_hdr *)(skb->data_start + ETH_HDR_LEN + IP_HDR_LEN);
    }
    void inline swap()
    {
        sport = ntohs(sport);
        dport = ntohs(dport);
        len = ntohs(len);
    }
    static int udp_recv(skbuff *skb);
} __attribute__((packed));
