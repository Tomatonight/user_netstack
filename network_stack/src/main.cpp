#include "interface.h"
#include <stdio.h>
#include<unistd.h>
#include<time.h>
#include <iostream>
char *my_eth_name = "ens33";
char my_mac_addr[6] = {0x00, 0x0c, 0x29, 0x14, 0x15, 0x16};
char *eth_ip = "192.168.72.123";
char *gateway_ip = "192.168.72.2";
void udp_test1()
{
    init();
    int fd = socket_(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0)
    {
        printf("socket error\n");
    }
    printf("socket success\n");
    sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("192.168.72.144"); // 192.168.72.129
    addr.sin_port = htons(1234);
    if (connect_(fd, &addr) < 0)
    {
        printf("connect error\n");
    }
    printf("connect success \n");
    for (int i = 0; i < 20; i++)
    {

        write_(fd, "asd", 3);
        char c = i + '0';
        std::string s = std::to_string(i);
        write_(fd, s.data(), s.size());
        write_(fd, "\n", 1);
    }
    printf("write success \n");
    close_(fd);
    // exit();
    return;
};
void udp_test2()
{
    init();
    int fd = socket_(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd < 0)
    {
        printf("socket error\n");
    }
    printf("socket success\n");
    sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("192.168.72.123"); // 192.168.72.129
    addr.sin_port = htons(1234);
    if(bind_(fd,&addr)<0)
    {
        printf("bind err\n");
    }
    while (true)
    {
        char buffer[1024]={0};
        sockaddr_in recv_addr;
        in_addr addr;
        int re=recvfrom_(fd,buffer,1024,&recv_addr);
        addr.s_addr=recv_addr.sin_addr.s_addr;
        printf("recvfrom %s %d re:%d %s\n",inet_ntoa(addr),ntohs(recv_addr.sin_port),
        re,buffer);
    }
    close_(fd);
    return;
}
void tcp_test1()
{
    init();
    int fd=socket_(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(fd<0)
    {
        printf("create socket err\n");
    }
    sockaddr_in daddr;
    daddr.sin_addr.s_addr=inet_addr("192.168.72.141");
    daddr.sin_family=AF_INET;
    daddr.sin_port=htons(1234);
    printf("start connect\n");
    if(connect_(fd,&daddr)<0)
    {
        printf("connect err\n");
    }
    for(int i=0;i<10;i++)
    {
        int re=write_(fd,"test\n",5);
        printf("write %d \n",re);
        if(re<=0)
        {
            break;
        }
    }
    close_(fd);
    sleep(100);
    return;
}
void tcp_test2()
{
    init();
    int fd=socket_(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(fd<0)
    {
        printf("create socket err\n");
    }
    sockaddr_in daddr;
    daddr.sin_addr.s_addr=inet_addr("192.168.72.141");
    daddr.sin_family=AF_INET;
    daddr.sin_port=htons(1234);
    printf("start connect\n");
    if(connect_(fd,&daddr)<0)
    {
        printf("connect err\n");
    }
    while(true)
    {
        char buffer[100]={0};
        int re=read_(fd,buffer,100);
        printf("read re: %d %s\n",re,buffer);
        if(re<=0)
        {
            break;
        }
    }
    close_(fd);
    sleep(100);
    return;
}
int main()
{
    tcp_test2();
    return 0;
}