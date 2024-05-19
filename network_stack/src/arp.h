#pragma once
#include <stdlib.h>
#include<iostream>
#include<list>
#include<shared_mutex>
#include<mutex>
#include "ether.h"
#include"skbuff.h"
#define ARP_ETHERNET 0x0001
#define ARP_IPV4 0x0800

#define ARP_REQUEST 0x0001
#define ARP_REPLY 0x0002

#define ARP_HDR_LEN sizeof(arp_hdr)
#define ARP_DATA_LEN sizeof(arp_data)
struct arp_hdr
{
    u_int16_t hwtype;
    u_int16_t protype;
    u_int8_t hwsize;
    u_int8_t prosize;
    u_int16_t opcode;
} __attribute__((packed));
struct arp_data
{
    char smac[6];
    uint32_t sip;
    char dmac[6];
    uint32_t dip;
}__attribute__((packed));
struct arp_entry
{
    uint32_t ip;
    char mac[6];
};
class arp_entry_control
{
    public:
    static void init();
    void add_entry(uint32_t ip,char* mac);
    arp_entry* search_arp_entrys(uint32_t ip);
    void updata_arp_entrys(uint32_t ip,char* mac);
    static inline arp_entry_control* get();
    private:
    static arp_entry_control* arp_entrys;
    std::list<arp_entry*> arp_list;
    std::shared_mutex mtx;
};
void arp_recv(skbuff* skb);
void arp_reply(skbuff *skb,arp_hdr *arp,arp_data* data);
void arp_request(uint32_t dest_ip);
void update_arp_entry(skbuff*);