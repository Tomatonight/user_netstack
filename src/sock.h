#pragma once
#include <sys/socket.h>
#include <arpa/inet.h>
#include <memory>
#include "ip.h"
class sock
{
public:
  skbuff_list recv_list;
  int protocol = 0;
  uint16_t sport = 0;
  uint16_t dport = 0;
  uint32_t saddr = 0;
  uint32_t daddr = 0;
  virtual int connect_(sockaddr_in *addr){};
  // std::function<int(sockaddr_in *addr)> connect_;
  virtual int write_(char *buf, int len){};
  virtual void close_(){};
  // std::function<int(char *buf, int len)> write_;
  //  std::function<int(char *buf, int len)> read_;
  virtual int read_(char *buf, int len){};
  virtual int bind_(sockaddr_in *){};
  //  std::function<int(sockaddr *)> bind_;
  virtual int sendto_(char *buf, int len, sockaddr_in *addr){};
  //  std::function<int(char *buf, int len, sockaddr *addr)> sendto_;
  virtual int recvfrom_(char *buf, int len, sockaddr_in *addr){};
  virtual int listen_(int){};
  virtual int accpet_(int){};
};