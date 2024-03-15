#include "arp.h"
arp_entry_head *arp_entry_head::arp_entrys_head = nullptr;
void arp_hdr::arp_reply(skbuff *skb)
{
    arp_hdr *arp = arp_hdr::arp_header(skb);
    arp_data *data = arp_data::arp_data_header(skb);

    data->swap();

    netdev *aim_dev = netdev::search_netdev(data->dip);
    if (!aim_dev)
    {
        return;
    }
    skbuff *new_skb = new skbuff(ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN, 0);
    new_skb->expand_s(ARP_DATA_LEN);
    arp_data *new_data = (arp_data *)(new_skb->data_start);
    new_skb->expand_s(ARP_HDR_LEN);
    arp_hdr *new_arp = (arp_hdr *)(new_skb->data_start);
    new_arp->hwsize = 6;
    new_arp->hwtype = ARP_ETHERNET;
    new_arp->opcode = ARP_REPLY;
    new_arp->prosize = 4;
    new_arp->protype = ARP_IPV4;
    new_data->dip = data->sip;
    new_data->sip = data->dip;
    memcpy(new_data->smac, aim_dev->mac, 6);
    memcpy(new_data->dmac, data->smac, 6);
    new_skb->dev = aim_dev;
    new_data->swap();
    new_arp->swap();
    netdev::netdev_send(data->smac, ETH_ARP, new_skb);
    return;
}
void arp_hdr::arp_recv(skbuff *skb)
{

    arp_hdr *arp = arp_hdr::arp_header(skb);
    arp->swap();
    if (arp->protype != ARP_IPV4 || arp->hwtype != ARP_ETHERNET)
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
        arp_reply(skb);
        break;
    default:
        break;
    }
    delete skb;

    return;
}
void arp_hdr::arp_request(uint32_t dest_ip)//dest_ip host
{
    in_addr addr={.s_addr=ntohl(dest_ip)};
 
    route_entry *entry;
    entry = route_entry_head::search_netdev(dest_ip);

    if (!entry)
    {
        std::cerr << "find route error" << std::endl;
        return;
    }
    netdev *dev = entry->dev;
    skbuff *skb = new skbuff(ETH_HDR_LEN + ARP_HDR_LEN + ARP_DATA_LEN, 0);
    skb->dev = dev;
    skb->expand_s(ARP_DATA_LEN);
    arp_data *data = (arp_data *)(skb->data_start);
    skb->expand_s(ARP_HDR_LEN);
    arp_hdr *arp = (arp_hdr *)(skb->data_start);
    data->dip = dest_ip;
    data->sip = dev->netdev_ip;
    memcpy(data->smac, dev->mac, 6);
    char dest_mac[6] = {0, 0, 0, 0, 0, 0};
    memcpy(data->dmac, dest_mac, 6);
    data->swap();
    arp->hwsize = 6;
    arp->hwtype = ARP_ETHERNET;
    arp->opcode = ARP_REQUEST;
    arp->prosize = 4;
    arp->protype=ARP_IPV4;
    arp->swap();
    uint8_t boardcast[6] ={0xFF,0XFF,0XFF,0XFF,0XFF,0XFF};
    netdev::netdev_send((char*)boardcast, ETH_ARP, skb);
    return;
}
void arp_hdr::update_arp_entry(skbuff *skb)
{
    // arp_hdr* arp=arp_hdr::arp_header(skb);
    arp_data *arp_data = arp_data::arp_data_header(skb);

    arp_entry *entry = arp_entry_head::arp_entrys_head->search(ntohl(arp_data->sip));

    if (entry)
    {
        memcpy(entry->mac, arp_data->smac, 6);
        return;
    }
    arp_entry_head::arp_entrys_head->add_entry(ntohl(arp_data->sip), arp_data->smac);
}
arp_entry *arp_entry_head ::search(uint32_t ip)//host
{
    std::shared_lock<std::shared_mutex> shared_lock(arp_entrys_head->shared_mtx);
    for (arp_entry *item : arp_entrys_head->lists)
    {
        if (item->ip == ip)
        {
            return item;
        }
    }
    return NULL;
}
void arp_entry_head ::add_entry(uint32_t ip, char *mac)//host
{
    in_addr addr={.s_addr=ntohl(ip)};

    arp_entry *entry = new arp_entry(ip, mac);
    std::unique_lock<std::shared_mutex> shared_lock(arp_entrys_head->shared_mtx);
    lists.push_back(entry);
}
void arp_entry_head::init()
{
    arp_entrys_head = new arp_entry_head();
}
