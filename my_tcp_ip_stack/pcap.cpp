#include "pcap.h"
const char *pcap::my_eth_name = "ens33";
const char pcap::my_mac_addr[6] = {0x00, 0x0c, 0x29, 0x6, 0x6, 0x6};
pcap_t *pcap::pcap_dev = nullptr;
uint32_t pcap::eth_device_send(char *buffer, uint32_t length)
{
    if (pcap_sendpacket(pcap_dev, (const u_char *)buffer, length) == -1)
    {
        fprintf(stderr, "pcap send: send packet failed!:%s\n", pcap_geterr(pcap_dev));
        fprintf(stderr, "pcap send: pcaket size %d\n", length);
        return 0;
    }
 
    return 0;
}
uint32_t pcap::eth_device_read(char *buffer, uint32_t length)
{
    int err;
    struct pcap_pkthdr *pkthdr;
    const uint8_t *pkt_data;
    err = pcap_next_ex(pcap_dev, &pkthdr, &pkt_data);

    if (err == 0)
    {
        return 0;
    }
    else if (err == 1)
    {
        if (pkthdr->len > 1500)
            return -1;
        memcpy(buffer, pkt_data, pkthdr->len);
        return pkthdr->len;
    }

    fprintf(stderr, "pcap_read: reading packet failed!:%s", pcap_geterr(pcap_dev));
    return 0;
}
void pcap::eth_open_dev()
{
    char error_buf[PCAP_ERRBUF_SIZE] = {0};
    /*
       pcap_if_t* pcapif_list ;
       int count=0;
       if(pcap_findalldevs(&pcapif_list,error_buf)<0)
       {
           fprintf(stderr,"find dev error\n");
           pcap_freealldevs(pcapif_list);
           return -1;
       }
       for(pcap_if_t* eth_if=pcapif_list;eth_if! ;eth_if=eth_if->next)
       {
           if(!strcmp(eth_name,eth_if->name))
           {

           }
       }

       */

    pcap_dev = pcap_open_live(my_eth_name, 65535, 1,512, error_buf);
    if (!pcap_dev)
    {
        fprintf(stderr, "open %s error\n", my_eth_name);
        return;
    }
    /*
    if (pcap_setnonblock(pcap, 1, error_buf) != 0)
    {
        fprintf(stderr, "set noblock %s error\n", eth_name);
        return ;
    }*/
    
        if (pcap_setdirection(pcap_dev, PCAP_D_IN) != 0)
        {
            fprintf(stderr, "set direction error\n");
            return;
        }
    
    bpf_u_int32 mask=0;
    bpf_u_int32 net=0;
    char filter[255] = {0};
    struct bpf_program fp;
    memset(&fp,0,sizeof(fp));
    
    if (pcap_lookupnet(my_eth_name,&net, &mask, error_buf))
    {
        fprintf(stderr, "get net and mask error\n");
        return;
    }
//(ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether broadcast) and (not ether src %02x:%02x:%02x:%02x:%02x:%02x)
    sprintf(filter,
            "(ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether broadcast) and (not ether src %02x:%02x:%02x:%02x:%02x:%02x)",
            my_mac_addr[0], my_mac_addr[1], my_mac_addr[2], my_mac_addr[3], my_mac_addr[4], my_mac_addr[5],
            my_mac_addr[0], my_mac_addr[1], my_mac_addr[2], my_mac_addr[3], my_mac_addr[4], my_mac_addr[5]);
   
    
    if (pcap_compile(pcap_dev, &fp, filter, 0,0) < 0)
    {
        fprintf(stderr, "set compile error %s\n", pcap_geterr(pcap_dev));
        return;
    }

    if (pcap_setfilter(pcap_dev, &fp) < 0)
    {
        fprintf(stderr, "setfilter error\n");
        return;
    }

    return;
}
void pcap::eth_device_close()
{
    pcap_close(pcap_dev);
}
