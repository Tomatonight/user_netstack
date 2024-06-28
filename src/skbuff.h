#pragma once
#include<iostream>
#include<memory.h>
#include<list>
#include<condition_variable>
#include<mutex>
struct tcp_hdr;
class skbuff
{
public:
skbuff(uint32_t len);
~skbuff();
uint16_t protocol;
char* start;
char* end;
char* data_start;
char* data_end;
char* payload;
struct tcp_hdr* tcp_hdr;
int payload_len=0;
uint32_t seq;
};
class skbuff_list
{
    public:
    ~skbuff_list();
    void add_skbuff(skbuff* skb);
    skbuff* get_first_skbuff();
    void remove_and_destroy_first_skbuff();
    skbuff* remove_first_skbuff();
    bool list_empty();
    std::condition_variable cv;
    std::mutex mtx;
    std::list<skbuff*> skb_list;
    
};