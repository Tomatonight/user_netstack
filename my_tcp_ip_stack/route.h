#pragma once
#include <arpa/inet.h>
#include <mutex>
#include<list>
#include <shared_mutex>
#include "netdev.h"

class route_entry
{
public:
    route_entry(u_int32_t dest_ip_, u_int32_t gateway_ip_, u_int32_t mask_, class netdev *dev_);
    ~route_entry(){};
    u_int32_t dest_ip;
    u_int32_t gateway_ip;
    u_int32_t mask;
    class netdev *dev;
};
class route_entry_head
{
public:
    static void routes_init();
    ~route_entry_head();
    static route_entry *search_netdev(u_int32_t dest_ip);
    static route_entry_head *routes;

private:
    std::list<route_entry*> lists;
    static void inline add_entry(route_entry *);
    route_entry_head();

    std::shared_mutex shared_mtx;
};
