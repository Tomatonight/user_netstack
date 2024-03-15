#include "stackhead.h"
#include "ipc.cpp"
int main()
{
  IPC::init();

  int tcp_fd = us_socket(AF_INET, SOCK_STREAM, 0);
  printf("tcpfd:%d\n", tcp_fd);
  sockaddr_in a;
  a.sin_addr.s_addr = inet_addr("192.168.72.144");
  a.sin_port = htons(1234);
  a.sin_family = AF_INET;
  int re = us_connect(tcp_fd, &a);
  printf("connect %d\n", re);
  char buf[100] = {0};
  for (int i = 0; i < 1; i++)
  {
    memset(buf, 0, 100);
    int re = us_read(tcp_fd, buf, 100);
    printf("len: %d %s\n", re, buf);
    if (re == 0)
    {
      us_close(tcp_fd);
      break;
    }
  }
  us_close(tcp_fd);
  printf("read end\n");

  printf("exit\n");
}
