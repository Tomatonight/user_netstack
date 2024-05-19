#pragma once
#include <iostream>
#include<thread>
#include "ether.h"
#include "skbuff.h"
#include "pcap.h"
class netdev
{
public:
    static void netdev_init();
    static netdev *get();
    char dev_name[10];
    char mac[6];
    u_int16_t mtu;
    u_int32_t netdev_ip;
    int netdev_send(char *dest_mac, uint16_t ether_type, skbuff *skb);
    void netdev_recv_loop();
    std::thread recv_thread;

private:
    static netdev *netdev_eth;
};
