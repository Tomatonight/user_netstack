#include"tcp.h"
tcp_manage *tcp_manage::tcp_manger = nullptr;

void tcp_manage::add_sock(tcp_sock *sock)
{
    std::unique_lock<std::shared_mutex> l(tcp_manger->shared_mtx);
    
    std::map<std::tuple<uint32_t, uint32_t, uint16_t, uint16_t>, tcp_sock *>::iterator i = tcp_manger->tcp_maps.find(
    {sock->saddr,sock->daddr,sock->sport,sock->dport}
    );
  
    if (i != tcp_manger->tcp_maps.end())
        return;
    tcp_manger->tcp_maps[{sock->saddr,sock->daddr,sock->sport,sock->dport}]=sock;
}
void tcp_manage::remove_sock(tcp_sock *sock)
{
  
    std::unique_lock<std::shared_mutex> l(tcp_manger->shared_mtx);
    int port = sock->sport;
    std::map<std::tuple<uint32_t, uint32_t, uint16_t, uint16_t>,  tcp_sock *>::iterator i = 
    tcp_manger->tcp_maps.find({sock->saddr,sock->daddr,sock->sport,sock->dport});
  
    if (i == tcp_manger->tcp_maps.end() || i->second != sock)
        return;
    tcp_manger->tcp_maps.erase(i);
}

tcp_sock *tcp_manage::search_sock(uint32_t sip, uint32_t dip,
                                  uint16_t sport, uint16_t dport)
{
    std::shared_lock<std::shared_mutex> l(tcp_manger->shared_mtx);
    std::map<std::tuple<uint32_t, uint32_t, uint16_t, uint16_t>, tcp_sock *>::iterator i 
    = tcp_manger->tcp_maps.find({sip,dip,sport,dport});
    
    if (i != tcp_manger->tcp_maps.end())
    {
        return i->second;
    }
    return nullptr;
}
bool tcp_manage::exist_port( uint16_t port)
{
    std::shared_lock<std::shared_mutex> l(tcp_manger->shared_mtx);
    for(std::map<std::tuple<uint32_t, uint32_t, uint16_t, uint16_t>, tcp_sock *>::iterator
    i=tcp_manger->tcp_maps.begin();i!=tcp_manger->tcp_maps.end();i++)
    {
        tcp_sock* sock=i->second;
        if(sock->sport==port)
        return true;
    }
    return false;
}
int tcp_manage::generate_port()
{
    int port=gernerate_random();
    while (exist_port(port))
    {
         port=gernerate_random();
    }
return port;
}