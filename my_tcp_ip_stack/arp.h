#pragma once
#include<list>
#include "ether.h"

#include <mutex>
#include <shared_mutex>
#define ARP_ETHERNET 0x0001 // hwtype

#define ARP_IPV4 0x0800 // protype

#define ARP_REQUEST 0x0001 // opcode
#define ARP_REPLY 0x0002

#define ARP_HDR_LEN sizeof(arp_hdr)
#define ARP_DATA_LEN sizeof(arp_data)
class arp_hdr
{

public:
    static void arp_reply(skbuff* skb);
    static void arp_recv(skbuff *skb);
    static void arp_request(uint32_t dest_ip);
    static void update_arp_entry(skbuff *skb);
    inline void swap()
    {
        hwtype = ntohs(hwtype);
        protype = ntohs(protype);
        opcode = ntohs(opcode);
    }

    static inline arp_hdr *arp_header(skbuff *skb)
    {
        return (arp_hdr *)(skb->data_start + ETH_HDR_LEN);
    }

public:
    uint16_t hwtype;
    uint16_t protype;
    uint8_t hwsize;
    uint8_t prosize;
    uint16_t opcode;
} __attribute__((packed));
class arp_data
{
public:
    char smac[6];
    uint32_t sip;
    char dmac[6];
    uint32_t dip;
    static inline arp_data *arp_data_header(skbuff *skb)
    {
        return (arp_data *)(skb->data_start + ETH_HDR_LEN + ARP_HDR_LEN);
    }
    inline void swap()
    {
        sip=ntohl(sip);
        dip=ntohl(dip);
    }

}__attribute__((packed));
class arp_entry
{
public:
    arp_entry(uint32_t ip_, char *mac_) : ip(ip_)
    {
        memcpy(mac, mac_, 6);
    }
    uint32_t ip;
    char mac[6];
};
class arp_entry_head
{
    std::shared_mutex shared_mtx;
    std::list<arp_entry*> lists;
public:
    static void init();
    static arp_entry_head *arp_entrys_head;
    static arp_entry * search(uint32_t ip);
    void add_entry(uint32_t ip, char *mac);
    
};