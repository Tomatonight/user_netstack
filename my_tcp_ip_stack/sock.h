#pragma once
#include <functional>
#include <mutex>
#include <set>
#include <condition_variable>
#include "read_tasks.h"
#include "ip.h"
#include "skbuff.h"
#include <arpa/inet.h>
class socket;
class sock
{
public:
  sock():read_task(nullptr){};
  virtual ~sock()
  {
  }
  class socket *skt=nullptr;
  skbuff_head recv_queue;
  skbuff_head send_queue;
  std::function<int(void)> read_task;
  std::condition_variable cv;
  int protocol=0;
  int state=0;

  uint16_t sport=0;
  uint16_t dport=0;
  uint32_t saddr=0;
  uint32_t daddr=0;

  //------------------------------------------
  virtual int connect_(sockaddr_in *addr) = 0;
  // std::function<int(sockaddr_in *addr)> connect_;
  virtual int write_(char *buf, int len) = 0;
  virtual void close_() = 0;
  // std::function<int(char *buf, int len)> write_;
  //  std::function<int(char *buf, int len)> read_;
  virtual int read_(char *buf, int len) = 0;
  virtual int bind_(sockaddr_in *) = 0;
  //  std::function<int(sockaddr *)> bind_;
  virtual int sendto_(char *buf, int len, sockaddr_in *addr) {};
  //  std::function<int(char *buf, int len, sockaddr *addr)> sendto_;
  virtual int recvfrom_(char *buf, int len, sockaddr_in *addr) {};
  virtual int listen_(int){};
  virtual int accpet_(int){};
  // std::function<int(char *buf, int len, sockaddr *addr)> recvfrom_;
};
