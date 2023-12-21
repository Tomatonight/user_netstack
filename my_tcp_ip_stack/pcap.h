#pragma once
#include <pcap.h>
#include <memory.h>
class pcap
{
public:

    static void init()
    {
        eth_open_dev();
    }
    static void exit()
    {
         eth_device_close();
    }
    static uint32_t eth_device_send(char *buffer, uint32_t length);
    static uint32_t eth_device_read(char *buffer, uint32_t length);
    static const char* my_eth_name;
    static const char my_mac_addr[6];
private:
    static void eth_open_dev();
    static void eth_device_close();
    static pcap_t *pcap_dev;
     
};

