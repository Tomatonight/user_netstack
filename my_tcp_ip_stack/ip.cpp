#include "ip.h"
#include "sock.h"
#include"udp.h"
#include"tcp.h"
int ip_hdr::ip_recv(skbuff *skb)
{

    ip_hdr *ip = ip_hdr::ip_header(skb);
    if (ip->version != IPV4 || ip->ttl == 0 || ip->ihl < 5)
    {
        delete skb;
        return -1;
    }
    uint16_t sum = utils::checksum((char *)ip, IP_HDR_LEN, 0);
    if (sum != 0)
    {
        printf("drop a packet\n");
        delete skb;
        return -1;
    }
    ip->swap();
    if (!netdev::search_netdev(ip->daddr))
    {
        delete skb;
        return -1;
    }
    switch (ip->proto)
    {
    case ICMPV4:
    {
        delete skb;
        break;
    }
    case IP_TCP:
    {
        tcp_hdr::tcp_recv(skb);
        break;
    }
    case IP_UDP:
    {
       udp_hdr::udp_recv(skb);
        break;
    }
    default:
        delete skb;
        break;
    }
    return 0;
}
int ip_hdr::ip_send(sock *sk, skbuff *skb)
{
  
    route_entry *entry;
    entry = route_entry_head::search_netdev(ntohl(sk->daddr));
    uint32_t jump_next_ip = entry->gateway_ip;
    if (jump_next_ip == 0)
    {
        jump_next_ip = ntohl(sk->daddr);
    }
    if (!entry)
    {
        in_addr addr = {.s_addr = sk->daddr};
        std::cerr << "cant find route ip: " << inet_ntoa(addr) << std::endl;
    
        if(skb->protocol!=IP_TCP)delete skb;
      
        return -1;
    }
    //   in_addr addr = {.s_addr = jump_next_ip};
    //  std::cerr << "dest ip: " << inet_ntoa(addr) << std::endl;
    skb->expand_s(IP_HDR_LEN);
    ip_hdr *ip = (ip_hdr *)(skb->data_start);
    skb->dev = entry->dev;
    skb->route = entry;
    ip->version = IPV4;
    ip->proto = skb->protocol;
    ip->flags = 0;
    ip->frag_offset = 0;
    ip->len = 4;
    ip->ihl = 5;
    ip->ttl = 64;
    ip->len = skb->data_end - skb->data_start;
    ip->saddr = skb->dev->netdev_ip;
    ip->daddr = ntohl(sk->daddr);
    in_addr addr = {.s_addr = htonl(ip->daddr)};
    ip->csum = 0;
    ip->swap();
    ip->csum = utils::checksum((char *)ip, ip->ihl * 4, 0);
    ip->tos = 0;
    arp_entry *arp_entry=nullptr;
   
    for (int i = 0; i < 4; i++)
    {
        arp_entry = arp_entry_head::search(jump_next_ip);
        if (!arp_entry && i < 3)
        { 
            arp_hdr::arp_request(jump_next_ip);
            if(skb->protocol==IP_TCP)return 0;
            sleep(1);
        }
    }
    
    if (!arp_entry)
    {
   
        if(skb->protocol!=IP_TCP){
            delete skb;
            };
      
        in_addr addr = {.s_addr = htonl(jump_next_ip)};
        std::cerr << "cant find " << inet_ntoa(addr) << " mac addr" << std::endl;
    
        return -1;
    }
 
    netdev::netdev_send(arp_entry->mac, ETH_IPV4, skb);
    
    return 0;
}