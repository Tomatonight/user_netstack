#pragma once
#include<list>
#include<mutex>
#include<shared_mutex>
#include<thread>
#include "netdev.h"
struct route_entry
{
    
    route_entry(u_int32_t dest_ip_, u_int32_t gateway_ip_, u_int32_t mask_, class netdev *dev_);
    u_int32_t dest_ip;
    u_int32_t gateway_ip;
    u_int32_t mask;
    class netdev* dev;
};
class route_entry_control
{
public:
static void route_entrys_init();
static route_entry_control* get();
void add_entry(route_entry*);
route_entry* search_route_entry(uint32_t d_ip);
private:
static route_entry_control* route_entrys;
std::list<route_entry*> route_list;
std::shared_mutex shared_mtx;
};


