#include "netdev.h"
#include "pcap.h"
#include"ip.h"
netdev* netdev::netdev_loop=nullptr;
netdev* netdev::netdev_eth=nullptr;
netdev::netdev(const char *name_, const char *addr, u_int16_t mtu_, u_int32_t netdev_ip_) : mtu(mtu_), netdev_ip(netdev_ip_)
{
    strcpy(dev_name, name_);
    memcpy(mac, addr, 6);
}
void netdev::netdev_init()
{
    char addr[6] = {0};
    netdev_loop = new netdev("lo", addr, 1500, htonl(inet_addr("127.0.0.1")));
    netdev_eth = new netdev(pcap::my_eth_name, pcap::my_mac_addr, 1500, htonl(inet_addr("192.168.72.10")));
}
int netdev::netdev_send( char *dest_mac, uint16_t ether_type, skbuff *skb)
{
    
    netdev* dev=skb->dev;
    skb->expand_s(ETH_HDR_LEN);
    eth_hdr *eth = (eth_hdr *)(skb->data_start);
    memcpy(eth->dmac,dest_mac, 6);
    memcpy(eth->smac, dev->mac, 6);
    eth->ethertype = ether_type;
    eth->swap();
    
    int len = pcap::eth_device_send(skb->data_start, skb->data_end - skb->data_start);
    if(skb->protocol!=IP_TCP)delete skb;
    return len;
}
static int netdev_recv()
{

    skbuff *skb = new skbuff(1600, 1);
    int len = pcap::eth_device_read(skb->head, 1600); // 阻塞
    if (len <= 0)
    {
        delete skb;
        return -1;
    }
    skb->expand_r(len);
    eth_hdr *eth = eth_hdr::eth_header(skb);
    eth->swap();
    switch (eth->ethertype)
    {
    case ETH_ARP:
   
     arp_hdr::arp_recv(skb);
       break;
    case ETH_IPV4:

        ip_hdr::ip_recv(skb);
        break;
    case ETH_IPv6:
    default:
        delete skb;
      
        return -1;
        break;
    }
    return 0;
}
int netdev::netdev_recv_loop()
{
    while (true)
    {
        netdev_recv();
    }
}
netdev *netdev::search_netdev(uint32_t ip)
{
    if (ip == netdev_loop->netdev_ip)
    {
        return netdev_loop;
    }
    if (ip == netdev_eth->netdev_ip)
    {
        return netdev_eth;
    }
    return NULL;
}