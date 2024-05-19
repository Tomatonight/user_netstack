#include "route.h"
extern char* gateway_ip;
route_entry::route_entry(u_int32_t dest_ip_, u_int32_t gateway_ip_, u_int32_t mask_, netdev *dev_)
{
    dest_ip = dest_ip_;
    gateway_ip = gateway_ip_;
    mask = mask_;
    dev = dev_;
}
route_entry_control* route_entry_control::route_entrys=nullptr;
void route_entry_control::route_entrys_init()
{
    if (!route_entrys)
        route_entrys = new route_entry_control;
    route_entry *entry_1 = new route_entry(ntohl(inet_addr("192.168.72.0")),
                                           0, 0xFFFFFF00, netdev::get());
    route_entry *entry_default = new route_entry(ntohl(inet_addr("0.0.0.0")),
                                                 ntohl(inet_addr(gateway_ip)), 0X00000000, netdev::get());
    route_entrys->add_entry(entry_1);
    route_entrys->add_entry(entry_default);
}
route_entry_control *route_entry_control::get()
{
    return route_entrys;
}
void route_entry_control::add_entry(route_entry *route_entry)
{
    route_list.push_back(route_entry);
}
route_entry* route_entry_control::search_route_entry(uint32_t d_ip)
{
    for(route_entry* item:route_list)
    {   
        if((item->dest_ip&item->mask)==(d_ip&item->mask))
        {
           return item;
        }
    }
    printf("error no route entry\n");
    return NULL;
}