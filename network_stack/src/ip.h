#pragma once
#include "arp.h"
class sock;
#define IPV4   0x04
#define IP_TCP 0x06
#define IP_UDP 0x11
#define ICMPV4 0x01
#define IP_HDR_LEN sizeof(ip_hdr)
struct ip_hdr
{
    uint8_t ihl : 4;           // 4位首部长度
    uint8_t version : 4;       // 4位版本号
    uint8_t tos;               // 8位服务类型
    uint16_t len;              // 16位总长度
    uint16_t id;               // 16位标识
    uint16_t flags : 3;        // 3位标志
    uint16_t frag_offset : 13; // 13位偏移
    uint8_t ttl;               // 8位生存时间
    uint8_t proto;             // 8位协议
    uint16_t csum;             // 16位首部校验和
    uint32_t saddr;            // 源地址
    uint32_t daddr;            // 目的地址
}__attribute__((packed));
void ip_recv(skbuff* skb);
int ip_send(sock* sk,skbuff* skb);

