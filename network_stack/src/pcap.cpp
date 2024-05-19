#include "pcap.h"
extern char *my_eth_name;
extern char my_mac_addr[6];
pcap* pcap::individual_pcap=nullptr;
int pcap::eth_device_send(char *buffer, u_int32_t length)
{
    int re=pcap_sendpacket(individual_pcap->pcap_dev, (const u_char *)buffer, length);
    if (re == -1)
    {
        fprintf(stderr, "pcap send: send packet failed!:%s\n", pcap_geterr(individual_pcap->pcap_dev));
        fprintf(stderr, "pcap send: pcaket size %d\n", length);
        return 0;
    }
 
    return re;
}
int pcap::eth_device_read(char** buffer)
{
    int err;
    struct pcap_pkthdr *pkthdr;
    const uint8_t *pkt_data;
    err = pcap_next_ex(individual_pcap->pcap_dev, &pkthdr, &pkt_data);
    *buffer=(char*)pkt_data;
    if (err == 0)
    {
        return 0;
    }
    else if (err == 1)
    {
        if (pkthdr->len > 1500)
            return 0;
        return pkthdr->len;
    }
    printf("pcap_read: reading packet failed!:%s", pcap_geterr(individual_pcap->pcap_dev));
    return -1;
}
void pcap::eth_open_dev()
{
    char error_buf[PCAP_ERRBUF_SIZE] = {0};
    pcap_dev = pcap_open_live(my_eth_name, 65535, 1, 1000, error_buf);

    if (!pcap_dev)
    {

        printf("open %s error\n", my_eth_name);
        return;
    }
 
    if (pcap_setdirection(pcap_dev, PCAP_D_IN) != 0)
    {
        printf("set direction error\n");
        return;
    }

    bpf_u_int32 mask = 0;
    bpf_u_int32 net = 0;
    char filter[255] = {0};
    struct bpf_program fp;
    memset(&fp, 0, sizeof(fp));

    if (pcap_lookupnet(my_eth_name, &net, &mask, error_buf))
    {
        printf("get net and mask error\n");
        return;
    }
    //(ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether broadcast) and (not ether src %02x:%02x:%02x:%02x:%02x:%02x)
    sprintf(filter,
            "(ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether broadcast) and (not ether src %02x:%02x:%02x:%02x:%02x:%02x)",
            my_mac_addr[0], my_mac_addr[1], my_mac_addr[2], my_mac_addr[3], my_mac_addr[4], my_mac_addr[5],
            my_mac_addr[0], my_mac_addr[1], my_mac_addr[2], my_mac_addr[3], my_mac_addr[4], my_mac_addr[5]);

    if (pcap_compile(pcap_dev, &fp, filter, 0, 0) < 0)
    {
        printf("set compile error %s\n", pcap_geterr(pcap_dev));
        return;
    }

    if (pcap_setfilter(pcap_dev, &fp) < 0)
    {
        printf("setfilter error\n");
    }
}
void pcap::eth_close_dev()
{
    pcap_close(pcap_dev);
}
pcap::~pcap()
{
    eth_close_dev();
}
pcap::pcap()
{
    eth_open_dev();
};
void pcap::pcap_init()
{
    if (!individual_pcap)
    {
        individual_pcap = new pcap();
    }
}