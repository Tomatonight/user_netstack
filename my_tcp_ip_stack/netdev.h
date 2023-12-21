#pragma once
#include "sys/types.h"
#include "string.h"
#include "skbuff.h"
#include "ether.h"
#include <arpa/inet.h>
#include <iostream>
class skbuff;
class netdev
{
public:
    netdev(){};
    ~netdev(){};

    char dev_name[10];
    char mac[6];
    u_int16_t mtu;
    u_int32_t netdev_ip;
    static netdev *netdev_loop;
    static netdev *netdev_eth;
    static void netdev_init();
    static netdev *search_netdev(uint32_t ip);
    static int netdev_send(char *dest_mac, uint16_t ether_type, skbuff *skb);
    static int netdev_recv_loop();

private:
    netdev(const char *name_, const char *addr, u_int16_t mtu_, u_int32_t netdev_ip_);
};
