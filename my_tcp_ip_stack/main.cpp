#include "route.h"
#include "pcap.h"
#include "arp.h"
#include "skbuff.h"
#include "netdev.h"
#include "sock.h"
#include "socket.h"
#include "ip.h"
#include "ipc.h"
#include "udp.h"
#include "tcp.h"
#include <iostream>
#include <thread>
#include <netinet/in.h>
int main()
{

  pcap::init();
  netdev::netdev_init();
  route_entry_head::routes_init();
  arp_entry_head::init();
  sockets_manger::init();
  udp_sock_manage::init();
  tcp_manage::init();
  std::thread t1([](){timer_ctl::init();});
    std::thread t2([](){
    IPC::recv_fd_loop();
  });
  netdev::netdev_recv_loop();

  printf("exit\n");
  return 0;
}