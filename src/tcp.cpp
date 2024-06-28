#include "tcp.h"
#include "check.h"
tcp_manage *tcp_manage::tcp_manager = nullptr;
void tcp_manage::init()
{
    tcp_manager = new tcp_manage();
}
tcp_manage *tcp_manage::get()
{
    return tcp_manager;
}
tcp_manage::tcp_manage() : tcp_bitmap(TCP_MAX_PORT_NUM, TCP_BASE_PORT)
{
}
void tcp_manage::add_sock(tcp_sock *sock)
{
    std::unique_lock<std::mutex> l(mtx);
    if (tcp_map.find({sock->saddr, sock->sport}) != tcp_map.end())
    {
        printf("add tcp sock error\n");
        return;
    }
    tcp_bitmap.set_value(sock->sport, true);
    tcp_map[{sock->saddr, sock->sport}] = sock;
}
void tcp_manage::remove_sock(tcp_sock *sock)
{
    std::unique_lock<std::mutex> l(mtx);
    tcp_bitmap.free_value(sock->sport);
    tcp_map.erase({sock->saddr, sock->sport});
}
tcp_sock *tcp_manage::search_tcp_sock(uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport)
{
    if (tcp_map.find({daddr, dport}) == tcp_map.end())
    {
        return nullptr;
    }
    tcp_sock *sk = tcp_map[{daddr, dport}];
    if (sk->daddr == saddr && sk->dport == sport)
        return sk;
    return nullptr;
}
void tcp_recv(skbuff *skb)
{
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start + ETH_HDR_LEN + IP_HDR_LEN);
    ip_hdr *ip = (ip_hdr *)(skb->data_start + ETH_HDR_LEN);
    uint16_t sum = checksum_tran(htonl(ip->saddr), htonl(ip->daddr), IP_TCP, (char *)tcp, ip->len - IP_HDR_LEN);
    if (sum != 0)
    {
        printf("drop a tcp skbuff because checksum\n");
        delete skb;
        return;
    }
    tcp_sock *sock = nullptr;
    tcp->sport = ntohs(tcp->sport);
    tcp->dport = ntohs(tcp->dport);
    tcp->seq = ntohl(tcp->seq);
    tcp->ack_seq = ntohl(tcp->ack_seq);
    tcp->win = ntohs(tcp->win);
    tcp->urp = ntohs(tcp->urp);
    uint32_t saddr = ip->saddr, daddr = ip->daddr;
    uint16_t sport = tcp->sport, dport = tcp->dport;
    sock = tcp_manage::get()->search_tcp_sock(saddr, daddr, sport, dport);
    if (!sock)
    {
        printf("drop because cant find sock\n");
        delete skb;
        return;
    }
    skb->seq = tcp->seq;
    skb->payload = (char *)tcp + tcp->hl * 4;
    skb->payload_len = ip->len - IP_HDR_LEN - tcp->hl * 4;
    skb->tcp_hdr = tcp;
    sock->occupy_mtx.lock();
    sock->handle_recv(skb, tcp, ip);
    if (tcp_manage::get()->search_tcp_sock(saddr, daddr, sport, dport))
    {
        sock->occupy_mtx.unlock();
    }
};
