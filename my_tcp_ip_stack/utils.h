#pragma once
#include<sys/types.h>
#include<memory.h>
#include<arpa/inet.h>
class pseudo_head//用于传输层checksum
{
    public:
    u_int32_t saddr;
    u_int32_t daddr;
    char zero;
    char proto;
    u_int16_t len;    
};
class utils
{
public:
static u_int16_t checksum(char* buff,int len,int start_sum);
static u_int16_t checksum_tran(u_int32_t saddr,u_int32_t daddr,char proto,char* data,u_int16_t len);
private:
static u_int32_t sum_every_16bits(char* data,int len);
};