#include "netdev.h"
#include"arp.h"
#include"ip.h"
extern char *my_eth_name;
extern char my_mac_addr[6];
extern char* eth_ip;
netdev* netdev::netdev_eth=nullptr;
void netdev::netdev_init()
{
    netdev_eth = new netdev();
    netdev_eth->mtu = 1500;
    netdev_eth->netdev_ip=ntohl(inet_addr(eth_ip));
    memcpy(netdev_eth->mac, my_mac_addr, 6);
    memcpy(netdev_eth->dev_name, my_eth_name, strlen(my_eth_name));
}
netdev *netdev::get()
{
    return netdev_eth;
}
int netdev::netdev_send(char *dest_mac, uint16_t ether_type, skbuff *skb)
{
    skb->data_start -= ETH_HDR_LEN;
    eth_hdr *ether = (eth_hdr *)skb->data_start;
    ether->ethertype = ether_type;
    memcpy(ether->dmac, dest_mac, 6);
    memcpy(ether->smac, netdev_eth->mac, 6);
    ether->ethertype = htons(ether->ethertype);
    return pcap::eth_device_send(skb->data_start, skb->data_end - skb->data_start);
}
void netdev::netdev_recv_loop()
{
    while (true)
    {

        char* buffer;
        int len = pcap::eth_device_read(&buffer);
        if(len<=0)continue;
        skbuff *skb = new skbuff(1500);
        memcpy(skb->start,buffer,len);
        skb->data_start = skb->start;
        skb->data_end = skb->data_start + len;
        eth_hdr *ether = (eth_hdr *)skb->data_start;
        ether->ethertype=ntohs(ether->ethertype);
        switch (ether->ethertype)
        {
        case ETH_ARP:
            arp_recv(skb);
            break;
        case ETH_IPV4:
            ip_recv(skb);
            break;
        default:
            printf("drop\n");
            delete skb;
        }
    }
}