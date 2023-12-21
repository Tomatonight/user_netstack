#include "udp.h"

udp_sock_manage *udp_sock_manage::udp_manage = nullptr;
sock_udp *udp_sock_manage::search_udp_sock(int port)
{
    std::unique_lock<std::mutex> l(udp_manage->udp_mtx);
    std::map<int, class sock_udp *>::iterator it = udp_manage->used_ports.find(port);
    if (it == udp_manage->used_ports.end())
        return NULL;
    return it->second;
}
int udp_hdr::udp_recv(skbuff *skb)
{

    udp_hdr *udp = udp_hdr::udp_header(skb);
    ip_hdr *ip = ip_hdr::ip_header(skb);
    class sock_udp *sk;
    uint16_t sum = utils::checksum_tran(htonl(ip->saddr), htonl(ip->daddr), (IP_UDP), (char *)udp, ntohs(udp->len));
    skb->data_len = ntohs(udp->len) - UDP_HDR_LEN;

    if (sum != 0)
    {
        printf("udp checksum sum: %d \n", sum);
        delete skb;
        return -1;
    }
 
    sk = udp_sock_manage::search_udp_sock(ntohs(udp->dport));
    if (!sk)
    {
        printf("sock not find\n");
        delete skb;
        return -1;
    }
 std::unique_lock<std::mutex> l(sk->delete_mtx);
    if (sk->state == UDP_CONNECT && sk->sport != udp->sport)
    {
        printf("connect drop\n");
        delete skb;
        return -1;
    }
    sk->recv_queue.add_skbuff(skb);
    if(sk->read_task)
    {
        sk->read_task();
        sk->read_task=nullptr;
    }
 
    return 0;
}
int sock_udp::connect_(sockaddr_in *addr)
{
    if (state == UDP_CONNECT)
        recv_queue.clear();
    daddr = addr->sin_addr.s_addr;
    dport = addr->sin_port;
    state = UDP_CONNECT;
    return 0;
}
int sock_udp::read_(char *buf, int len)
{

    if (state != UDP_CONNECT)
    {
        return -1;
    }
    if (recv_queue.empty())
    {
        printf("error recv_queue empty\n"); // 理论上极小可能为空
        return -1;
    }
    skbuff *skb = recv_queue.pop_skbuff();
    char *data = skb->data_start + ETH_HDR_LEN + IP_HDR_LEN + UDP_HDR_LEN;
    size_t data_size = skb->data_len;
    data_size = data_size > len ? len : data_size;
    memcpy(buf, data, data_size);
    delete skb;
    return data_size;
}
int sock_udp::write_(char *buf, int len)
{
    printf("write buf:%s\n", buf);
    if (state != UDP_CONNECT)
        return -1;
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + UDP_HDR_LEN + len, 0);
    skb->expand_s(len);
    skb->protocol = IP_UDP;
    memcpy(skb->data_start, buf, len);
    skb->expand_s(UDP_HDR_LEN);
    udp_hdr *udp = (udp_hdr *)(skb->data_start);
    udp->csum = 0;
    udp->sport = sport;
    udp->dport = dport;
    udp->len = htons(len + UDP_HDR_LEN);
    udp->csum = utils::checksum_tran(saddr, daddr, IP_UDP, skb->data_start, skb->data_end - skb->data_start);
    return ip_hdr::ip_send((sock *)this, skb);
}
int sock_udp::bind_(sockaddr_in *addr)
{

    if (sport != 0)
    {
        return -1;
    }
    if (addr->sin_addr.s_addr != htonl(netdev::netdev_eth->netdev_ip))
    {
        return -1;
    }
    if (udp_sock_manage::bind_udp_port(ntohs(addr->sin_port), this) < 0)
    {
        return -1;
    }

    saddr = addr->sin_addr.s_addr;
    sport = addr->sin_port;
    return 0;
}
int sock_udp::sendto_(char *buf, int len, sockaddr_in *addr)
{
    if (state == UDP_CONNECT)
    {
        return -1;
    }
    if (sport == 0)
    {
        udp_sock_manage::generate_udp_port_and_bind(this);
        saddr = ntohl(netdev::netdev_eth->netdev_ip);
    }
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + UDP_HDR_LEN + len, 0);
    skb->expand_s(len);
    memcpy(skb->data_start, buf, len);
    skb->expand_s(UDP_HDR_LEN);
    udp_hdr *udp = (udp_hdr *)(skb->data_start);
    udp->csum = 0;
    udp->sport = addr->sin_port;
    skb->protocol=IP_UDP;
    udp->dport = addr->sin_port;
    udp->len = htons(len + UDP_HDR_LEN);
    udp->csum = utils::checksum_tran(saddr, daddr, IP_UDP, skb->data_start, skb->data_end - skb->data_start);
    return ip_hdr::ip_send(this, skb);
}
int sock_udp::recvfrom_(char *buf, int len, sockaddr_in *addr)
{
    if (state == UDP_CONNECT)
    {
        return -1;
    }
    if (recv_queue.empty())
    {
        printf("error recv_queue empty\n"); // 理论上极小可能为空
        return -1;
    }
    skbuff *skb = recv_queue.pop_skbuff();
    udp_hdr *udp = udp_hdr::udp_header(skb);
    ip_hdr *ip = ip_hdr::ip_header(skb);
    addr->sin_family = AF_INET;
    addr->sin_port = ntohs(udp->sport);
    addr->sin_addr.s_addr = ntohl(ip->saddr);
    char *data = skb->data_start + ETH_HDR_LEN + IP_HDR_LEN + UDP_HDR_LEN;
    size_t data_size = skb->data_len;
    data_size = data_size > len ? len : data_size;
    memcpy(buf, data, data_size);
    delete skb;
    return data_size;
}
sock_udp::sock_udp(class socket *skt_)
{
    protocol = IP_UDP;
    skt = skt_;
    state = UDP_UNCONNECT;
    sport = 0;
    saddr = 0;
}
sock_udp::~sock_udp()
{
    std::unique_lock<std::mutex> l(delete_mtx);
    if (sport != 0)
    {
        udp_sock_manage::erase_udp_port(htons(sport));
    }
}
void sock_udp::close_()
{
    delete this;
}