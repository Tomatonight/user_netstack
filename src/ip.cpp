#include "ip.h"
#include "check.h"
#include "sock.h"
#include "udp.h"
#include"tcp.h"
#include <unistd.h>
#include "route.h"
#include"icmp.h"
void ip_recv(skbuff *skb)
{
 
    ip_hdr *ip = (ip_hdr *)(skb->data_start + ETH_HDR_LEN);
    if (ip->daddr != htonl(netdev::get()->netdev_ip))
    {
        delete skb;
        return;
    }
    if (ip->version != IPV4 || ip->ttl == 0 || ip->ihl < 5)
    {
        delete skb;
        return;
    }
    uint16_t sum = checksum((char *)ip, IP_HDR_LEN, 0);
    if (sum != 0)
    {
        printf("recv fault ip pkt\n");
        delete skb;
        return;
    }
    ip->len = ntohs(ip->len);
    ip->id = ntohs(ip->id);
    ip->saddr = ntohl(ip->saddr);
    ip->daddr = ntohl(ip->daddr);
    switch (ip->proto)
    {
    case ICMPV4:
    {
        icmp_recv(skb);
        break;
    }
    case IP_TCP:
    {
        tcp_recv(skb);
        break;
    }
    case IP_UDP:
    {
        udp_recv(skb);
        break;
    }
    default:
        delete skb;
        break;
    }
}
int ip_send(sock *sk, skbuff *skb)
{
    skb->data_start = skb->data_start - IP_HDR_LEN;
    ip_hdr *ip = (ip_hdr *)(skb->data_start);
    ip->version = IPV4;
    ip->proto = sk->protocol;
    ip->flags = 0;
    ip->frag_offset = 0;
    ip->tos = 0;
    ip->ihl = 5;
    ip->csum = 0;
    ip->ttl = 64;
    ip->len = skb->data_end - skb->data_start;
    ip->saddr = sk->saddr;
    ip->daddr = sk->daddr;
    ip->len = htons(ip->len);
    ip->id = htons(ip->id);
    ip->saddr = htonl(ip->saddr);
    ip->daddr = htonl(ip->daddr);
    ip->csum = checksum((char *)ip, ip->ihl * 4, 0);
    //*********************************** search route
    arp_entry *arp_entry = nullptr;
    route_entry *route = nullptr;
    route = route_entry_control::get()->search_route_entry(sk->daddr);
    if (!route)
    {
        printf("error cant find route\n");
        return -1;
    }
    uint32_t jump_next_ip = route->gateway_ip;
    if (jump_next_ip == 0)
    {
        jump_next_ip = sk->daddr;
    }
    for (int i = 0; i < 5; i++)
    {
        arp_entry = arp_entry_control::get()->search_arp_entrys(jump_next_ip);
        if (!arp_entry )
        {
            arp_request(jump_next_ip);
            sleep(1);
            continue;
        }
        else if(arp_entry)break;
    }
    if (!arp_entry)
    {
        in_addr addr = {.s_addr = htonl(jump_next_ip)};
        printf("search ip:%s mac addr error\n", inet_ntoa(addr));
        return -1;
    }
   //  printf("mac %02x %02x %02x %02x %02x %02x \n",arp_entry->mac[0],arp_entry->mac[1],arp_entry->mac[2]
  //  ,arp_entry->mac[3],arp_entry->mac[4],arp_entry->mac[5]);
    return netdev::get()->netdev_send(arp_entry->mac, ETH_IPV4, skb);
}