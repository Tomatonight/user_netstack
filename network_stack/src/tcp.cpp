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
    if (tcp_map.find(sock->sport) != tcp_map.end())
    {
        printf("add tcp sock error\n");
        return;
    }
    tcp_bitmap.set_value(sock->sport,true);
    tcp_map[sock->sport] = sock;
}
void tcp_manage::remove_sock(tcp_sock *sock)
{
    std::unique_lock<std::mutex> l(mtx);
    tcp_bitmap.free_value(sock->sport);
    tcp_map.erase(sock->sport);
}
tcp_sock *tcp_manage::search_tcp_sock(uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport)
{
    if (tcp_map.find(dport) == tcp_map.end())
    {
        return nullptr;
    }
    tcp_sock *sock = tcp_map[dport];
    if (sock->saddr == daddr && sock->daddr == saddr && sock->dport == sport)
        return sock;
    else
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
    sock = tcp_manage::get()->search_tcp_sock(ip->saddr, ip->daddr, tcp->sport, tcp->dport);
    if (!sock)
    {
        printf("drop because cant find sock\n");
        delete skb;
        return;
    }
    skb->seq = tcp->seq;
    skb->payload = (char *)tcp + tcp->hl * 4;
    skb->payload_len = ip->len - IP_HDR_LEN - tcp->hl * 4;
    skb->tcp_hdr=tcp;
    std::unique_lock<std::mutex> l(sock->occupy_mtx);
    sock->handle_recv_(skb, tcp, ip);
};
void tcp_sock::adjust_unordered_list(skbuff *skb)
{
    std::list<skbuff *>::iterator i = unordered_list.skb_list.begin();
    for (; i != unordered_list.skb_list.end(); i++)
    {
        if (skb->seq > (*i)->seq)
            continue;
        unordered_list.skb_list.insert(i, skb);
    }
    if (i == unordered_list.skb_list.end())
    {
        unordered_list.skb_list.insert(i, skb);
    }
    int sum=0;
    for (skbuff *skb : unordered_list.skb_list)
    {
        if (skb->seq == expect_seq)
        {
            expect_seq += skb->payload_len;
            recv_list.add_skbuff(skb);
            sum++;
        }
        else
        break;
    }
    while(sum-->0)
    {
        unordered_list.remove_first_skbuff();
    }

}
