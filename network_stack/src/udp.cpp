#include "udp.h"
#include "check.h"
#include "netdev.h"
#include <thread>
#include <mutex>
#include <arpa/inet.h>
udp_manage::udp_manage() : bitmap_udp_port(MAX_UDP_PORT_NUM, BASE_UDP_PORT)
{
}
void udp_manage::remove_sock(sock_udp *sock)
{
    std::unique_lock<std::mutex> l(mtx);
    bitmap_udp_port.free_value(sock->sport);
    udp_map.erase(sock->sport);
}
sock_udp *udp_manage::search_udp_sock(uint32_t s_ip, uint32_t d_ip, uint16_t s_port, uint16_t d_port)
{
    std::unique_lock<std::mutex> l(mtx);
    if (udp_map.find(d_port) == udp_map.end())
    {
        return nullptr;
    }
    sock_udp *sock = udp_map[d_port];
    if (sock->connect_state == UDP_CONNECT && (sock->dport != s_port || sock->saddr != d_ip || sock->daddr != s_ip))
    {
        return nullptr;
    }
    if (sock->saddr != d_ip)
    {
        return nullptr;
    }
    return sock;
}
udp_manage *udp_manage::udp_manager = nullptr;
void udp_manage::init()
{
    if (!udp_manager)
        udp_manager = new udp_manage;
}
void udp_manage::add_sock(sock_udp *sock)
{
    std::unique_lock<std::mutex> l(mtx);
    int port = sock->sport;
    if (udp_map.find(port) != udp_map.end())
    {
        printf("add udp sock error\n");
        return;
    }
    bitmap_udp_port.set_value(port, true);
    udp_map[port] = sock;
}
udp_manage *udp_manage::get()
{
    return udp_manager;
}

void udp_recv(skbuff *skb)
{
 //   printf("udp recv\n");
    udp_hdr *udp = (udp_hdr *)(skb->data_start + ETH_HDR_LEN + IP_HDR_LEN);
    ip_hdr *ip = (ip_hdr *)(skb->data_start + ETH_HDR_LEN);
    sock_udp *sock = nullptr;
    uint16_t sum = checksum_tran(htonl(ip->saddr), htonl(ip->daddr), IP_UDP, (char *)udp, ntohs(udp->len));
    if (sum != 0)
    {
        printf("drop a udp pkt because checksum\n");
        delete skb;
        return;
    }
    udp->sport = ntohs(udp->sport);
    udp->dport = ntohs(udp->dport);
    udp->len = ntohs(udp->len);
    skb->payload = (char *)udp + UDP_HDR_LEN;
    skb->payload_len = udp->len - UDP_HDR_LEN;
    // in_addr addrs = {.s_addr = ip->saddr}, addrd = {.s_addr = ip->daddr};
    sock = udp_manage::get()->search_udp_sock(ip->saddr, ip->daddr, udp->sport, udp->dport);
    {
        if (!sock)
        {
            printf("drop a udp pkt because fault address\n");
            delete skb;
            return;
        }
        sock->recv_list.add_skbuff(skb);
    }
    sock->recv_list.cv.notify_one();
}
int sock_udp::connect_(sockaddr_in *addr)
{
    
    if (connect_state == UDP_CONNECT)
        return -1;
    daddr = ntohl(addr->sin_addr.s_addr);
    dport = ntohs(addr->sin_port);
    connect_state = UDP_CONNECT;
    return 0;
}
int sock_udp::write_(char *buf, int len)
{
    
    if (connect_state != UDP_CONNECT)
    {
        printf("udp unconnect\n");
        return -1;
    }
    if (bind_state == UDP_UNBIND)
    {
        sockaddr_in addr;
        memset(&addr, 0, sizeof(sockaddr_in));
        bind_(&addr);
        bind_state = UDP_BIND;
    }
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + UDP_HDR_LEN + len);
    skb->data_start = skb->data_end - len;
    skb->payload = skb->data_start;
    memcpy(skb->data_start, buf, len);
    skb->data_start -= UDP_HDR_LEN;
    udp_hdr *udp = (udp_hdr *)(skb->data_start);
    udp->len = htons(UDP_HDR_LEN + len);
    udp->sport = htons(sport);
    udp->dport = htons(dport);
    udp->csum = 0;
    udp->csum = checksum_tran(htonl(saddr), htonl(daddr), IP_UDP, (char *)udp, UDP_HDR_LEN + len);
    int re = ip_send(this, skb);
    delete skb;
    return re;
}
void sock_udp::close_()
{
    
    if (bind_state == UDP_BIND)
        udp_manage::get()->remove_sock(this);
    delete this;
}
int sock_udp::read_(char *buf, int len)
{
    
    while (recv_list.list_empty())
    {
        std::unique_lock<std::mutex> l(recv_list.mtx);
        recv_list.cv.wait_for(l,std::chrono::milliseconds(10));
    }
    skbuff *skb = recv_list.remove_first_skbuff();
    udp_hdr *udp = (udp_hdr *)(skb->data_start + ETH_HDR_LEN + IP_HDR_LEN);
    ip_hdr *ip = (ip_hdr *)(skb->data_start + ETH_HDR_LEN);
    int data_len = skb->payload_len;
    int read_len = data_len > len ? len : data_len;
    memcpy(buf, skb->payload, read_len);
    delete skb;
    return read_len;
}
int sock_udp::bind_(sockaddr_in *addr)
{
    
    if (bind_state == UDP_BIND)
        return -1;
    if (addr->sin_addr.s_addr == 0)
    {
        addr->sin_addr.s_addr = htonl(netdev::get()->netdev_ip);
    }
    if (addr->sin_port == 0)
    {
        addr->sin_port = htons(udp_manage::get()->bitmap_udp_port.alloc_value());
    }
    uint32_t ip = ntohl(addr->sin_addr.s_addr);
    uint16_t port = ntohs(addr->sin_port);
    if (ip != netdev::get()->netdev_ip)
    {
        printf("bind ip error\n");
        return -1;
    }
    saddr = ip;
    sport = port;
    udp_manage::get()->add_sock(this);
    bind_state = UDP_BIND;
    return 0;
}
int sock_udp::sendto_(char *buf, int len, sockaddr_in *addr)
{
    
    if (connect_state == UDP_CONNECT)
    {
        return -1;
    }
    daddr = htonl(addr->sin_addr.s_addr);
    dport = htons(addr->sin_port);
    return write_(buf, len);
}
int sock_udp::recvfrom_(char *buf, int len, sockaddr_in *addr)
{
    
    if (connect_state == UDP_CONNECT)
    {
        return -1;
    }
    while (recv_list.list_empty())
    {
        std::unique_lock<std::mutex> l(recv_list.mtx);
        recv_list.cv.wait_for(l,std::chrono::milliseconds(50));
    }
    skbuff *skb = recv_list.remove_first_skbuff();
    udp_hdr *udp = (udp_hdr *)(skb->data_start + ETH_HDR_LEN + IP_HDR_LEN);
    ip_hdr *ip = (ip_hdr *)(skb->data_start + ETH_HDR_LEN);
    addr->sin_family = AF_INET;
    addr->sin_port = htons(udp->sport);
    addr->sin_addr.s_addr = htonl(ip->saddr);
    int data_len = skb->payload_len;
    int read_len = data_len > len ? len : data_len;
    memcpy(buf, skb->payload, read_len);
    delete skb;
    return read_len;
}