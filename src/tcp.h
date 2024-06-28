#pragma once
#include "sock.h"
#include "bitmap.h"
#include <map>
#include "timer.h"
#define TCP_FLAG_FIN 0x1
#define TCP_FLAG_RST 0x2
#define TCP_BASE_PORT 2000
#define TCP_MAX_PORT_NUM 60000
#define RETRAN_CONN_TIME 2000000 // 2s
#define RETRANSMIT_TIME 500000   // 0.5s
#define TIMEWAIT_TIME 5000000    // 5s
#define CONNECT_WAIT_TIME 10     // 10s
#define TCP_BIND 0x1
#define TCP_UNBIND 0x0
#define TCP_CONNECT 0x1
#define TCP_UNCONNECT 0x0
#define TCP_HDR_LEN sizeof(struct tcp_hdr)
struct tcp_hdr
{
    uint16_t sport;   /* 16位源端口号 */
    uint16_t dport;   /* 16位目的端口号 */
    uint32_t seq;     /* 32位序列号 */
    uint32_t ack_seq; /* 32位确认序列号,一般表示下一个期望收到的数据的序列号 */
    uint8_t rsvd : 4;
    uint8_t hl : 4;  /* 4位首部长度 */
    uint8_t fin : 1, /* 发送端完成发送任务 */
        syn : 1,     /* 同步序号用来发起一个连接 */
        rst : 1,     /* 重建连接 */
        psh : 1,     /* 接收方应该尽快将这个报文段交给应用层 */
        ack : 1,     /* 确认序号有效 */
        urg : 1,     /* 紧急指针有效 */
        ece : 1,     // 通知对方网络有阻塞1
        cwr : 1;     // 通知对方已将拥塞窗口缩小
    uint16_t win;    /* 16位窗口大小 */
    uint16_t csum;   /* 16位校验和 */
    uint16_t urp;    /* 16位紧急指针 */
    uint8_t data[];
} __attribute__((packed));
enum tcp_states
{
    TCP_LISTEN = 1,   /* 等待一个连接 */
    TCP_SYN_SENT,     /* 已经发送了一个连接请求,等待对方的回复 */
    TCP_SYN_RECEIVED, /* 等待第三次握手 */
    TCP_ESTABLISHED,  /* 连接建立成功 */
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_CLOSE,
    TCP_CLOSE_WAIT,
    TCP_CLOSING,
    TCP_LAST_ACK,
    TCP_TIME_WAIT,
};
struct tcp_opt_mss
{
    uint8_t kind;
    uint8_t len;
    uint16_t mss;
} __attribute__((packed));
class tcp_sock : public sock
{
public:
    tcp_sock();
    ~tcp_sock();
    bool bind_state = TCP_UNBIND;
  //  bool connect_state = TCP_UNCONNECT;
    uint16_t windows_recv = 40000; //	接收窗口大小
    uint16_t windows_send = 40000; // 发送窗口大小
    uint32_t next_seq = 0;         // 发送数据包的下一个序列号
    uint32_t recv_ack_seq = 0;
    uint32_t expect_seq = 0; // 期待收到的对方的seq
    uint16_t read_max_segment_size = 1460;
    uint16_t send_max_segment_size = 1460;
    int tcp_state = TCP_CLOSE;
    int tcp_flag = 0;
    bool closed = false;

    std::mutex occupy_mtx;
   // std::condition_variable occupy_cv;
    skbuff_list send_list;
    skbuff_list unordered_list;
    time_node *retransmit;
    time_node *time_wait;
    int connect_(sockaddr_in *addr);
    int write_(char *buf, int len);
    void close_();
    int read_(char *buf, int len);
    int bind_(sockaddr_in *);
    int listen_(int) {};
    int accpet_(int) {};
    void send_fin();
    void send_rst();
    void send_ack();
    void send_syn();
    void send_data(char *buf, int len);
    void handle_recv(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip);
    void recv_while_closed(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip);
    void recv_while_listen(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip);
    void recv_while_synsent(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip);
    void clear_send_list();
    bool test_if_could_read(int len);
    void enter_into_timewait();
    void transmit_skb(skbuff *skb, tcp_hdr *tcp, int add_seq, bool add_to_send_list);
    void adjust_unordered_list(skbuff *skb);
    void retransmit_fcn();
    void timewait_fcn();
    void destroy_sock();

};
// typedef std::shared_ptr<tcp_sock> SHARED_TCP_SOCK;
class tcp_manage
{
public:
    bitmap tcp_bitmap;
    static void init();
    static tcp_manage *get();
    void add_sock(tcp_sock *sock);
    void remove_sock(tcp_sock *sock);
    tcp_sock *search_tcp_sock(uint32_t saddr, uint32_t daddr, uint16_t sport, uint16_t dport);

private:
    tcp_manage();

    static tcp_manage *tcp_manager;
    std::map<std::tuple<uint32_t, uint16_t>, tcp_sock *> tcp_map;
    std::mutex mtx;
};
void tcp_recv(skbuff *skb);
