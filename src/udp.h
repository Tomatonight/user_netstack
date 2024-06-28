#pragma once
#include "ip.h"
#include "sock.h"
#include "bitmap.h"
#include<condition_variable>
#include <map>
#include <memory>
#define BASE_UDP_PORT 1000
#define MAX_UDP_PORT_NUM 50000
#define UDP_UNCONNECT 0x0
#define UDP_CONNECT 0x1
#define UDP_UNBIND 0x0
#define UDP_BIND 0x1
#define UDP_HDR_LEN sizeof(udp_hdr)
void udp_recv(skbuff *skb);
struct udp_hdr
{
    uint16_t sport; /* 源端口				*/
    uint16_t dport; /* 目的端口			*/
    uint16_t len;   /* 长度,包括首部和数据 */
    uint16_t csum;  /* 检验和				*/
    uint8_t data[];
} __attribute__((packed));
class sock_udp : public sock
{
public:
    sock_udp(){};
    ~sock_udp(){};
    bool bind_state = UDP_UNBIND;
    bool connect_state = UDP_UNCONNECT;

private:
    
    int connect_(sockaddr_in *addr);
    int write_(char *buf, int len);
    void close_();
    int read_(char *buf, int len);
    int bind_(sockaddr_in *);
    int sendto_(char *buf, int len, sockaddr_in *addr);
    int recvfrom_(char *buf, int len, sockaddr_in *addr);
    int listen_(int){};
    int accpet_(int){};

};
//typedef std::shared_ptr<sock_udp> SHARED_UDP_SOCK;
class udp_manage
{
public:
    static void init();
    static inline udp_manage *get();
    void remove_sock(sock_udp* sock);
    void add_sock(sock_udp* sock);
    sock_udp* search_udp_sock(uint32_t s_ip, uint32_t d_ip, uint16_t s_port, uint16_t d_port);
    bitmap bitmap_udp_port;
    std::map<std::tuple<uint32_t,uint16_t>, sock_udp*> udp_map;
private:
    udp_manage();
    std::mutex mtx;
    static udp_manage *udp_manager;
};
