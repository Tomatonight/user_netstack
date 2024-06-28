#include "icmp.h"
#include"sock.h"
#include"check.h"
void icmp_recv(skbuff *skb)
{
    ip_hdr *ip = (ip_hdr *)(skb->data_start + ETH_HDR_LEN);
    icmp_hdr *icmp = (icmp_hdr *)(skb->data_start + ETH_HDR_LEN + ip->ihl * 4);
    // checksum
    switch (icmp->type)
    {
    case ICMP_ECHO:
        icmp_reply(skb);
        break;
    case ICMP_REPLY:
    default:
        break;
    }
    delete skb;
}
void icmp_reply(skbuff *skb)
{
    ip_hdr *ip = (ip_hdr *)(skb->data_start + ETH_HDR_LEN);
    icmp_hdr *icmp = (icmp_hdr *)(skb->data_start + ETH_HDR_LEN + ip->ihl * 4);
    icmp->type=ICMP_REPLY;
    icmp->code=0;
    icmp->csum=0;
    icmp->csum=checksum((char*)icmp,sizeof(icmp_hdr),0);
    skb->data_start=(char*)icmp;
    skb->data_end=(char*)(icmp+1);
    class sock sk;
    sk.saddr=ip->daddr;
    sk.daddr=ip->saddr;
    sk.protocol=IPPROTO_ICMP;
    ip_send(&sk,skb);
}
void icmp_udp_close(uint32_t dst_ip)
{
    
}