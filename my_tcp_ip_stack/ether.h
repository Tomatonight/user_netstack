#pragma once
#include<sys/types.h>
#include<arpa/inet.h>
#include"skbuff.h"
#define ETH_HDR_LEN sizeof(eth_hdr)
#define ETH_IPV4 0x0800
#define ETH_IPv6 0x86dd
#define ETH_ARP 0x0806
class eth_hdr
{
    public:
	u_int8_t dmac[6];	
	u_int8_t smac[6];	
	u_int16_t ethertype; 
	u_int8_t payload[];
    void inline swap()
    {
        ethertype=ntohs(ethertype);
    }

    static inline eth_hdr* eth_header(skbuff* skb)
    {
        return (eth_hdr*)(skb->data_start);
    }
} __attribute__((packed));

