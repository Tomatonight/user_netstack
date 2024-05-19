#include "arp.h"
#include "route.h"
#include <thread>
#include <memory.h>
#include <netinet/in.h>
arp_entry_control *arp_entry_control::arp_entrys = nullptr;
inline arp_entry_control *arp_entry_control::get()
{
    return arp_entrys;
}
void arp_entry_control::add_entry(uint32_t ip, char *mac)
{
    std::unique_lock<std::shared_mutex> l(mtx);
    arp_entry *new_entry = new arp_entry();
    new_entry->ip = ip;
    memcpy(new_entry->mac, mac, 6);
    arp_list.push_back(new_entry);
}
arp_entry *arp_entry_control::search_arp_entrys(uint32_t ip)
{

    std::shared_lock<std::shared_mutex> l(mtx);
    for (arp_entry *item : arp_list)
    {
        if (item->ip == ip)
        {
            return item;
        }
    }
    return nullptr;
}
void arp_entry_control::updata_arp_entrys(uint32_t ip, char *mac)
{
    std::unique_lock<std::shared_mutex> l(mtx);
    for (arp_entry *item : arp_list)
    {
        if (item->ip == ip)
        {
            memcpy(item->mac, mac, 6);
            return;
        }
    }
    arp_entry *new_entry = new arp_entry();
    new_entry->ip = ip;
    memcpy(new_entry->mac, mac, 6);
    arp_list.push_back(new_entry);
}
void arp_entry_control::init()
{
    arp_entrys = new arp_entry_control();
    arp_entry* local_addr=new arp_entry;
    local_addr->ip=netdev::get()->netdev_ip;
    memcpy(local_addr->mac,netdev::get()->mac,6);
    arp_entrys->arp_list.push_back(local_addr);
}
void arp_recv(skbuff *skb)
{
   
    arp_hdr *arp = (arp_hdr *)(skb->data_start + ETH_HDR_LEN);

    arp->opcode = ntohs(arp->opcode);
    arp->protype = ntohs(arp->protype);
    arp->hwtype = ntohs(arp->hwtype);

    if (arp->hwtype != ARP_ETHERNET || arp->protype != ARP_IPV4)
    {
        delete skb;
        return;
    }
    update_arp_entry(skb);

    switch (arp->opcode)
    {
    case ARP_REPLY:
        break;
    case ARP_REQUEST:
        arp_data *data = (arp_data *)(skb->data_start + ETH_HDR_LEN + ARP_HDR_LEN);
        arp_reply(skb, arp, data);
        break;
    }
    delete skb;
}
void arp_reply(skbuff *skb, arp_hdr *arp, arp_data *data)
{
    if(ntohl(data->dip)!=netdev::get()->netdev_ip)
        return;
    skbuff *new_skb = new skbuff(ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN);
    new_skb->data_start = new_skb->start + ETH_HDR_LEN;
    new_skb->data_end = new_skb->end;
    arp_hdr *new_arp = (arp_hdr *)new_skb->data_start;
    arp_data *new_data = (arp_data *)(new_skb->data_start + ARP_HDR_LEN);
    new_arp->hwsize = 6;
    new_arp->hwtype = htons(ARP_ETHERNET);
    new_arp->opcode = htons(ARP_REPLY);
    new_arp->prosize = 4;
    new_arp->protype = htons(ARP_IPV4);
    new_data->dip = data->sip;
    new_data->sip = data->dip;
    netdev *dev = netdev::get();
    memcpy(new_data->smac, dev->mac, 6);
    memcpy(new_data->dmac, data->smac, 6);
    arp->hwtype = ntohs(arp->hwtype);
    arp->protype = ntohs(arp->protype);
    arp->opcode = ntohs(arp->opcode);
    netdev::get()->netdev_send(new_data->dmac, ETH_ARP, new_skb);
    delete new_skb;
}
void arp_request(uint32_t dest_ip)
{
    skbuff *skb = new skbuff(ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN);
    skb->data_start = skb->start + ETH_HDR_LEN;
    skb->data_end = skb->end;
    arp_hdr *arp = (arp_hdr *)skb->data_start;
    arp_data *data = (arp_data *)(skb->data_start + ARP_HDR_LEN);
    netdev *dev = netdev::get();
    arp->hwsize = 6;
    arp->hwtype = ARP_ETHERNET;
    arp->opcode = ARP_REQUEST;
    arp->prosize = 4;
    arp->protype = ARP_IPV4;
    arp->hwtype = ntohs(arp->hwtype);
    arp->protype = ntohs(arp->protype);
    arp->opcode = ntohs(arp->opcode);
    data->dip = dest_ip;
    data->sip = dev->netdev_ip;
    data->sip = ntohl(data->sip);
    data->dip = ntohl(data->dip);
    memcpy(data->smac, dev->mac, 6);
    char dest_mac[6] = {0, 0, 0, 0, 0, 0};
    memcpy(data->dmac, dest_mac, 6);
    uint8_t boardcast[6] = {0xFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF};
    netdev::get()->netdev_send((char *)boardcast, ETH_ARP, skb);
    delete skb;
}
void update_arp_entry(skbuff *skb)
{
    arp_data *data = (arp_data *)(skb->data_start + ETH_HDR_LEN + ARP_HDR_LEN);
   // printf("update mac %02x %02x %02x %02x %02x %02x\n",data->smac[0],data->smac[1],data->smac[2]
  //  ,data->smac[3],data->smac[4],data->smac[5]);
    arp_entry_control::get()->updata_arp_entrys(ntohl(data->sip), data->smac);
}