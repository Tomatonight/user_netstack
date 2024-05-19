#pragma once
#include<stdlib.h>
#define ETH_HDR_LEN sizeof(eth_hdr)
#define ETH_IPV4 0x0800
#define ETH_IPv6 0x86dd
#define ETH_ARP 0x0806
struct eth_hdr
{
    public:
	u_int8_t dmac[6];	
	u_int8_t smac[6];	
	u_int16_t ethertype; 
} __attribute__((packed));
