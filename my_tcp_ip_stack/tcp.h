#pragma once

#include "sock.h"
#include "ip.h"
#include "timer.h"
#include "skbuff.h"
#include <shared_mutex>
#include <map>
#include <functional>
#define TCP_HDR_LEN sizeof(tcp_hdr)
#define CONNECT_TIMEOUT_TIMES 5
#define CONNECT_TIMEOUT 1
#define RETRANSMIT_TIMEOUT 0
#define TIME_WAIT_TIME 20
#define TCP_FIN 0x01
#define TCP_PSH 0x02
#define TCP_RST 0x04
class tcp_hdr
{
public:
	uint16_t sport;	  /* 16位源端口号 */
	uint16_t dport;	  /* 16位目的端口号 */
	uint32_t seq;	  /* 32位序列号 */
	uint32_t ack_seq; /* 32位确认序列号,一般表示下一个期望收到的数据的序列号 */
	uint8_t rsvd : 4;
	uint8_t hl : 4;	 /* 4位首部长度 */
	uint8_t fin : 1, /* 发送端完成发送任务 */
		syn : 1,	 /* 同步序号用来发起一个连接 */
		rst : 1,	 /* 重建连接 */
		psh : 1,	 /* 接收方应该尽快将这个报文段交给应用层 */
		ack : 1,	 /* 确认序号有效 */
		urg : 1,	 /* 紧急指针有效 */
		ece : 1,	 // 通知对方网络有阻塞
		cwr : 1;	 // 通知对方已将拥塞窗口缩小
	uint16_t win;	 /* 16位窗口大小 */
	uint16_t csum;	 /* 16位校验和 */
	uint16_t urp;	 /* 16位紧急指针 */
	uint8_t data[];
	static inline tcp_hdr *tcp_header(skbuff *skb)
	{
		return (tcp_hdr *)(skb->data_start + ETH_HDR_LEN + IP_HDR_LEN);
	}
	inline void swap()
	{
		win = htons(win);
		csum = htons(csum);
		urp = htons(urp);
		dport = htons(dport);
		sport = htons(sport);
		seq = htonl(seq);
		ack_seq = htonl(ack_seq);
	}

	static void tcp_recv(skbuff *skb);

private:
} __attribute__((packed));
struct tcp_options
{
	uint16_t options;
	uint16_t mss;
};

struct tcp_opt_mss
{
	uint8_t kind;
	uint8_t len;
	uint16_t mss;
} __attribute__((packed));

enum tcp_states
{
	TCP_LISTEN = 1,	  /* 等待一个连接 */
	TCP_SYN_SENT,	  /* 已经发送了一个连接请求,等待对方的回复 */
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
/*
struct tcpiphdr
{
	uint32_t saddr;
	uint32_t daddr;
	uint8_t zero;
	uint8_t proto;
	uint16_t tlen;
} __attribute__((packed));
*/
class tcp_manage
{
public:
	static void init()
	{
		tcp_manger = new tcp_manage;
	}
	static tcp_manage *tcp_manger;
	static void add_sock(tcp_sock *sock);
	static void remove_sock(tcp_sock *sock);
	static tcp_sock *search_sock(uint32_t, uint32_t, uint16_t, uint16_t);
	static bool exist_port(uint16_t);
	static int generate_port();

private:
	std::map<std::tuple<uint32_t, uint32_t, uint16_t, uint16_t>, tcp_sock *> tcp_maps;
	std::shared_mutex shared_mtx;
};

class tcb
{
public:
	uint16_t windows_recv = 40000; //	接收窗口大小
	uint16_t windows_send = 0;	   // 发送窗口大小
	uint32_t send_seq=0;			   // 将要发送的seq
	uint32_t un_ack_seq=0;		   // 尚未确认的发送seq
	uint32_t expect_recv_seq=0;	   // 期待收到的seq
	uint32_t last_send_ack=0;
	uint32_t send_start=0;
	uint32_t recv_start=0;
	
	inline void init_send_seq()
	{
		send_seq = gernerate_random() % 30000;
		send_start = send_seq;
		un_ack_seq = send_seq;
	}
};

class tcp_sock : public sock
{
public:
	tcp_sock(tcp_sock *parent_ = nullptr) : parent(parent_)
	{
		state = TCP_CLOSE;
		protocol = IP_TCP;
	}
	~tcp_sock()
	{
	
		printf("delete sock\n");
		if (parent)
			clear_listen_sock();
		else{
			
			clear_sock();
		}

	
	}
	tcb tcb_;
	int process_tcp_packet(skbuff *skb, tcp_hdr *tcp);
	int read_(char *buf, int len);
	int write_(char *buf, int len);
	int bind_(sockaddr_in *addr);
	int connect_(sockaddr_in *addr);
	int listen_(int);
	int accept_();
	void close_();
	// private:
	int flag;
	int backoff = 0;
	tcp_sock *parent;
	uint16_t r_max_msg_size = 1000;
	uint16_t s_max_msg_size = 1000;
	// sock_queue recvsyn_queue; // 等待第二次握手
	//  std::list<tcp_sock *> client_queue; // 等待第三次握手
	sock_queue accpet_queue;		   // 握手完成
	timer *retransmit_timer = nullptr; // 超时重传
	
	void set_retransmit_timer(int seconds, std::function<void()> fcn);
	void re_connect();
	void cancel_retransmit_timer();
	void send_first_packet_in_send_queue();
	timer *send_ack_timer = nullptr; // 累积确认
	void set_send_ack_timer();
	void cancel_send_ack_timer();
	timer *keep_alive_timer = nullptr; // 长时间退出
	skbuff_head unordered_skbuff;
	int get_date_from_recv_queue(char *buf, size_t len);
	bool test_able_read(int len);

	void adjust_unordered_skbuff();
	void add_skbuff_to_unordered_skbuff(skbuff *skb);
	int tcp_transmit_directly(skbuff *skb, uint32_t seq, tcp_hdr *tcp,bool whether_add);
	int tcp_transmit(skbuff *skb, tcp_hdr *tcp);

	//----------------------
	int send_syn();
	int send_synack();
	int send_fin();
	int send_ack();
	int send_reset(uint32_t daddr, uint32_t saddr);
	int send_reset();
	void adjust_send_queue();
	void clear_listen_sock();
	int recv_while_close(skbuff *skb, tcp_hdr *tcp);
	int recv_while_listen(skbuff *skb, tcp_hdr *tcp);
	int recv_while_synsent(skbuff *skb, tcp_hdr *tcp);
	void enter_into_timewait();
	void clear_timer();
	void clear_sock();
	inline void run_read_task();
};