#include "route.h"
route_entry_head *route_entry_head::routes = nullptr;

route_entry::route_entry(u_int32_t dest_ip_, u_int32_t gateway_ip_, u_int32_t mask_, netdev *dev_) : dest_ip(dest_ip_), gateway_ip(gateway_ip_), mask(mask_), dev(dev_){};
void route_entry_head::routes_init()
{
    routes = new route_entry_head();
    route_entry *entry_0 = new route_entry(ntohl(inet_addr("127.0.0.1")), 0,0XFF000000, netdev::netdev_loop);
    route_entry *entry_1 = new route_entry(ntohl(inet_addr("192.168.72.0")), 0, 0xFFFFFF00, netdev::netdev_eth);
    route_entry *entry_default = new route_entry(ntohl(inet_addr("0.0.0.0")), ntohl(inet_addr("192.168.72.2")), 0X00000000, netdev::netdev_eth);
    add_entry(entry_0);
    add_entry(entry_1);
    add_entry(entry_default);
}
route_entry_head::route_entry_head()
{
}
route_entry_head::~route_entry_head()
{

      std::unique_lock<std::shared_mutex> l(routes->shared_mtx);
    for(route_entry * entry:routes->lists)
    {
        delete entry;
    }
    lists.clear();
    return;
}
route_entry *route_entry_head::search_netdev(u_int32_t dest_ip)
{
    std::shared_lock<std::shared_mutex> l(routes->shared_mtx);
    for(route_entry * entry:routes->lists)
    {    
        if ((entry->dest_ip & entry->mask )== (dest_ip & entry->mask))
        {
            return entry;
        }
    }

    return NULL;

}

void inline route_entry_head::add_entry(route_entry *entry)
{
    std::unique_lock<std::shared_mutex> l(routes->shared_mtx);
    routes->lists.push_back(entry);
}
