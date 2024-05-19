#pragma once
#include<stdlib.h>
#include<stdio.h>
#include<memory.h>
#include<pcap.h>
class pcap
{
public:
~pcap();
static void pcap_init();
static int eth_device_send(char *buffer, u_int32_t length);
static int eth_device_read(char** buffer);
private:
pcap();
pcap_t *pcap_dev=nullptr;
void eth_open_dev();
void eth_close_dev();
static class pcap* individual_pcap;
};