#pragma once
#include"utils.h"
#include"arp.h"
#include"skbuff.h"
class sock;
#define IPV4   0x04

#define IP_TCP 0x06

#define IP_UDP 0x11

#define ICMPV4 0x01

#define IP_HDR_LEN sizeof(ip_hdr)

class ip_hdr
{
    public:
    uint8_t ihl : 4;					// 4位首部长度
	uint8_t version : 4;				// 4位版本号
	uint8_t tos;						// 8位服务类型
	uint16_t len;						// 16位总长度
	uint16_t id;						// 16位标识
	uint16_t flags : 3;					// 3位标志
	uint16_t frag_offset : 13;			// 13位偏移
	uint8_t ttl;						// 8位生存时间
	uint8_t proto;						// 8位协议
	uint16_t csum;						// 16位首部校验和
	uint32_t saddr;						// 源地址
	uint32_t daddr;						// 目的地址
    static inline ip_hdr* ip_header(skbuff* skb)
    {
        return (ip_hdr*)(skb->data_start+ETH_HDR_LEN);
    }
    int static ip_recv(skbuff* sk);
    int static ip_send(class sock* sk,skbuff *skb);
    inline void swap()
    {
        len=ntohs(len);
        id=ntohs(id);
        saddr=ntohl(saddr);
        daddr=ntohl(daddr);
    }

}__attribute__((packed));

