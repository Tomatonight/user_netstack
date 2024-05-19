#include "tcp.h"
#include "check.h"
#include "netdev.h"
tcp_sock::tcp_sock()
{
    retransmit = new time_node;
    time_wait = new time_node;
    retransmit->fcn = std::bind(&tcp_sock::retransmit_fcn, this);
    time_wait->fcn = std::bind(&tcp_sock::timewait_fcn, this);
    timer::get()->add_time_node(retransmit);
    timer::get()->add_time_node(time_wait);
}
tcp_sock::~tcp_sock()
{
    printf("delete sock\n");
    timer::get()->remove_time_node(retransmit);
    timer::get()->remove_time_node(time_wait);
    delete retransmit;
    delete time_wait;
}
void tcp_sock::send_fin()
{
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN);
    skb->data_start = skb->data_start - TCP_HDR_LEN;
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->fin = true;
    tcp->ack = 1;
    tcp->hl = TCP_HDR_LEN / 4;
    transmit_skb(skb, tcp, 1, true);
}
void tcp_sock::send_rst()
{
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN);
    skb->data_start = skb->data_start - TCP_HDR_LEN;
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->ack = 1;
    tcp->rst = true;
    tcp->hl = TCP_HDR_LEN / 4;
    transmit_skb(skb, tcp, 0, 0);
    delete skb;
}
void tcp_sock::send_ack()
{
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN);
    skb->data_start = skb->data_start - TCP_HDR_LEN;
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->ack = 1;
    tcp->hl = TCP_HDR_LEN / 4;
    transmit_skb(skb, tcp, 0, false);
    delete skb;
}
void tcp_sock::send_syn()
{
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN + sizeof(tcp_opt_mss));
    skb->data_start = skb->data_end - sizeof(tcp_opt_mss);
    tcp_opt_mss *tcp_opt = (tcp_opt_mss *)skb->data_start;
    tcp_opt->kind = 2;
    tcp_opt->len = 4;
    tcp_opt->mss = htons(read_max_segment_size);
    skb->data_start = skb->data_start - TCP_HDR_LEN;
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->syn = true;
    tcp->hl = TCP_HDR_LEN / 4 + 1;
    transmit_skb(skb, tcp, 1, true);
}
void tcp_sock::send_data(char *buf, int len)
{

    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN + len);
    skb->data_start = skb->data_end - len;
    skb->payload = skb->data_start;
    memcpy(skb->data_start, buf, len);
    skb->data_start = skb->data_start - TCP_HDR_LEN;
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->ack = 1;
    skb->payload_len = len;
    tcp->hl = TCP_HDR_LEN / 4;
    transmit_skb(skb, tcp, len, true);
}
int tcp_sock::connect_(sockaddr_in *addr)
{

    std::unique_lock<std::mutex> l(occupy_mtx);
    if (connect_state == TCP_CONNECT || tcp_state != TCP_CLOSE)
        return -1;
    daddr = ntohl(addr->sin_addr.s_addr);
    dport = ntohs(addr->sin_port);
    if (!sport || !saddr)
    {
        sockaddr_in s_addr;
        memset(&s_addr, 0, sizeof(sockaddr_in));
        s_addr.sin_port = htons(sport);
        s_addr.sin_addr.s_addr = htonl(saddr);
        bind_(&s_addr);
    }
    srand(time(0));
    next_seq = rand() % 60000 + 10000;
    un_ack_seq = 0;
    tcp_state = TCP_SYN_SENT;
    send_syn();
    occupy_cv.wait_for(l, std::chrono::seconds(CONNECT_WAIT_TIME));
    if (tcp_state != TCP_ESTABLISHED)
    {
        //  std::unique_lock<std::mutex> l(send_list.mtx);
        retransmit->ignore = true;
        tcp_state = TCP_CLOSE;
        skbuff *skb = send_list.skb_list.front();
        delete skb;
        send_list.skb_list.pop_front();
        tcp_state = TCP_CLOSE;
        return -1;
    }
    else
    {
        connect_state = TCP_CONNECT;
        return 0;
    }
}
int tcp_sock::write_(char *buf, int len)
{
    std::unique_lock<std::mutex> l(occupy_mtx);
    // std::unique_lock<std::mutex> l(send_list.mtx);
    if (tcp_state != TCP_ESTABLISHED && tcp_state != TCP_CLOSE_WAIT)
    {
        printf("err state %d\n", tcp_state);
        return -1;
    }
    int left_len = len;
    while (left_len > 0)
    {
        int send_len = left_len < send_max_segment_size ? left_len : send_max_segment_size;
        left_len -= send_len;
        send_data(buf, send_len);
        buf += send_len;
    }
    return len;
}
void tcp_sock::close_()
{
    // std::unique_lock<std::mutex> l(recv_list.mtx);
    // std::unique_lock<std::mutex> l(occupy_mtx);
    occupy_mtx.lock();
    closed = true;
    switch (tcp_state)
    {
    case TCP_ESTABLISHED:
    {
        send_fin();
        tcp_state = TCP_FIN_WAIT_1;
        break;
    }
    case TCP_CLOSE_WAIT:
    {
        send_fin();
        tcp_state = TCP_LAST_ACK;
        break;
    }
    case TCP_LISTEN:
    {
        //**************
        // to do
        break;
    }
    case TCP_CLOSE:
    {
        destroy_sock();
        return;
    }
    default:
    {
        printf("close err\n");
        destroy_sock();
        return;
    }
    }
    occupy_mtx.unlock();
};
int tcp_sock::read_(char *buf, int len)
{
    std::unique_lock<std::mutex> l(occupy_mtx);
    if (tcp_state != TCP_ESTABLISHED && tcp_state != TCP_FIN_WAIT_1 && tcp_state != TCP_FIN_WAIT_2 && tcp_state != TCP_CLOSE_WAIT)
        return -1;
    int left_len = len;
    char *buffer = buf;

    while (!test_if_could_read(len))
    {
        recv_list.cv.wait_for(l, std::chrono::milliseconds(50));
    }
    int delete_sum = 0;
    if (recv_list.skb_list.empty())
    {
        if (tcp_flag & TCP_FLAG_FIN)
            return 0;
        else if (tcp_flag & TCP_FLAG_RST)
        {
            return -1;
        }
        else
        {
            printf("tcp read error\n");
            return -1;
        }
    }
    for (skbuff *skb : recv_list.skb_list)
    {
        if (left_len > skb->payload_len)
        {
            delete_sum++;
            memcpy(buffer, skb->payload, skb->payload_len);
            left_len -= skb->payload_len;
            buffer += skb->payload_len;
        }
        else
        {
            memcpy(buffer, skb->payload, left_len);
            skb->payload_len -= left_len;
            skb->payload += left_len;
            left_len = 0;
            break;
        }
    }
    while (delete_sum-- > 0)
    {
        skbuff *skb = recv_list.remove_first_skbuff();
        if (!skb)
        {
            printf("delete skb err\n");
        }
        delete skb;
    }
    return len - left_len;
};
int tcp_sock::bind_(sockaddr_in *addr_)
{

    if (tcp_state != TCP_CLOSE || bind_state == TCP_BIND)
        return -1;
    uint32_t addr = ntohl(addr_->sin_addr.s_addr);
    uint16_t port = ntohs(addr_->sin_port);
    if (!addr)
    {
        addr = (netdev::get()->netdev_ip);
    }
    if (addr != netdev::get()->netdev_ip)
    {
        printf("bind ip err\n");
        return -1;
    }
    if (!port)
    {
        srand(time(0));
        port = rand() % TCP_MAX_PORT_NUM + TCP_BASE_PORT;
        while (tcp_manage::get()->tcp_bitmap.test(port))
        {
            port = rand() % TCP_MAX_PORT_NUM + TCP_BASE_PORT;
        }
        tcp_manage::get()->tcp_bitmap.set_value(port, true);
    }
    saddr = addr;
    sport = port;
    bind_state = TCP_BIND;
    tcp_manage::get()->add_sock((this));
}
void tcp_sock::handle_recv_(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    switch (tcp_state)
    {
    case TCP_LISTEN:
    {
        recv_while_listen(skb, tcp, ip);
        break;
    }
    case TCP_SYN_SENT:
    {
        recv_while_synsent(skb, tcp, ip);
        break;
    }
    case TCP_SYN_RECEIVED:
    {
        // to do
        break;
    }
    case TCP_ESTABLISHED:
    {
        recv_while_establish(skb, tcp, ip);
        break;
    }
    case TCP_FIN_WAIT_1:
    {
        recv_while_finwait1(skb, tcp, ip);
        break;
    }
    case TCP_FIN_WAIT_2:
    {
        recv_while_finwait2(skb, tcp, ip);
        break;
    }
    case TCP_CLOSE:
    {
        recv_while_closed(skb, tcp, ip);
        break;
    }
    case TCP_CLOSE_WAIT:
    {
        recv_while_closewait(skb, tcp, ip);
        break;
    }
    case TCP_CLOSING:
    {
        recv_while_closing(skb, tcp, ip);
        break;
    }
    case TCP_LAST_ACK:
    {
        recv_while_last_ack(skb, tcp, ip);
        break;
    }
    case TCP_TIME_WAIT:
    {
        recv_while_timewait(skb, tcp, ip);
        break;
    }
    }
}
// void tcp_sock::handle_recv(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
// {
//     // std::unique_lock<std::mutex> l(recv_list.mtx);
//     switch (tcp_state)
//     {
//     case TCP_CLOSE:
//         recv_while_closed(skb, tcp, ip);
//         return;
//     case TCP_LISTEN:
//         recv_while_listen(skb, tcp, ip);
//         return;
//     case TCP_SYN_SENT:
//         recv_while_synsent(skb, tcp, ip);
//         return;
//     }
//     // 确认收到的seq是新的
//     if (tcp->seq < expect_seq || tcp->seq >= expect_seq + windows_recv)
//     {
//         if (!tcp->rst)
//             send_ack();
//         delete skb;
//         return;
//     }
//     if (tcp->rst)
//     {
//         enter_into_timewait();
//         delete skb;
//         return;
//     }
//     if (tcp->syn || !tcp->ack)
//     {
//         send_rst();
//         destroy_sock();
//         delete skb;
//         return;
//     }
//     // 过滤重传，更新ack
//     switch (tcp_state)
//     {
//     case TCP_SYN_RECEIVED:
//     {
//         // for server;连接成功建立 to do
//         break;
//     }
//     case TCP_ESTABLISHED:
//     case TCP_FIN_WAIT_1:
//     case TCP_FIN_WAIT_2:
//     case TCP_CLOSE_WAIT:
//     case TCP_CLOSING:
//     case TCP_LAST_ACK:
//     {
//         if (tcp->ack_seq > un_ack_seq && tcp->ack_seq <= next_seq)
//         {
//             un_ack_seq = tcp->ack_seq;
//         }
//     }
//     }
//     // 处理数据
//     if (tcp->fin)
//     {
//         printf("fffffffff %d %d %d\n", tcp->seq, expect_seq, skb->payload_len);
//     }
//     switch (tcp_state)
//     {
//     case TCP_ESTABLISHED:
//     case TCP_FIN_WAIT_1:
//     case TCP_FIN_WAIT_2:
//     {
//         if (skb->payload_len > 0)
//         {
//             adjust_unordered_list(skb);
//             occupy_cv.notify_all();
//         }
//         break;
//     }
//     case TCP_CLOSE_WAIT:
//     case TCP_CLOSING:
//     case TCP_LAST_ACK:
//     case TCP_TIME_WAIT:
//     {
        
//         delete skb;
//         return;
//     }
//     }
//     // 处理fin
//     if (tcp->fin)
//     {
//         printf("fin\n");
//     }
//     if (tcp->fin && tcp->seq == expect_seq - skb->payload_len)
//     {
//         printf("recv find\n");
//         switch (tcp_state)
//         {
//         case TCP_CLOSE:
//         case TCP_LISTEN:
//         case TCP_SYN_SENT:
//         case TCP_CLOSE_WAIT:
//         case TCP_CLOSING:
//         case TCP_LAST_ACK:
//         case TCP_TIME_WAIT:
//         {
//             if (skb->payload_len == 0)
//                 delete skb;
//             // delete skb;
//             return;
//         }
//         }
//         expect_seq += 1;
//         tcp_flag |= TCP_FLAG_FIN;
//         send_ack();
//         occupy_cv.notify_all();
//         switch (tcp_state)
//         {
//         case TCP_SYN_RECEIVED:
//         case TCP_ESTABLISHED:
//         {
//             tcp_state = TCP_CLOSE_WAIT;
//             occupy_cv.notify_all();
//             if (skb->payload_len == 0)
//                 delete skb;
//             break;
//         }
//         case TCP_FIN_WAIT_1:
//         {
//             tcp_state = TCP_CLOSING;
//             if (skb->payload_len == 0)
//                 delete skb;
//         }
//         case TCP_FIN_WAIT_2:
//         {
//             enter_into_timewait();
//             if (skb->payload_len == 0)
//                 delete skb;
//         }
//         }
//     }
// }
void tcp_sock::clear_send_list()
{
    while (true)
    {
        if (send_list.list_empty())
            return;
        skbuff *skb = send_list.get_first_skbuff();
        if (!skb)
            break;
        else if (skb->seq < un_ack_seq)
        {
            //    printf("clear one \n");
            send_list.remove_and_destroy_first_skbuff();
            continue;
        }
        else
        {
            break;
        }
    }
}
void tcp_sock::transmit_skb(skbuff *skb, tcp_hdr *tcp, int add_seq, bool add_to_send_list)
{
    tcp->sport = htons(sport);
    tcp->dport = htons(dport);
    tcp->seq = htonl(next_seq);
    tcp->ack_seq = htonl(expect_seq);
    //  tcp->csum = 0;
    tcp->win = htons(windows_recv);
    // tcp->urp = 0;
    //   tcp->ack = 1;
    tcp->csum = checksum_tran(htonl(saddr), htonl(daddr), 6, (char *)tcp, skb->payload_len + tcp->hl * 4);

    skb->tcp_hdr = tcp;
    skb->seq = next_seq;
    printf("send seq %d ack_seq %llu\n", next_seq, expect_seq);
    ip_send(this, skb);
    next_seq += add_seq;
    if (add_to_send_list)
    {
        send_list.add_skbuff(skb);
        retransmit->update(RETRANSMIT_TIME);
        retransmit->ignore = false;
    }
}
void tcp_sock::recv_while_closed(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    send_rst();
}
void tcp_sock::recv_while_listen(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    // to do
}
void tcp_sock::recv_while_synsent(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
  //  printf("recv_while_synsent\n");
    if (tcp->ack_seq != next_seq)
    {
        //   printf("drop 1\n");
        delete skb;
        return;
    }
    if (tcp->rst)
    {
        //   printf("drop 2\n");
        delete skb;
        tcp_state = TCP_CLOSE;
        destroy_sock();
        occupy_cv.notify_all();
        return;
    }
    if (!tcp->syn || !tcp->ack)
    {
        //  printf("drop 3\n");
        delete skb;
        return;
    }
    expect_seq = tcp->seq + 1;
    un_ack_seq = tcp->ack_seq;
    tcp_state = TCP_ESTABLISHED;
    send_ack();
    occupy_cv.notify_all();
    delete skb;
}
void tcp_sock::retransmit_fcn()
{
    std::unique_lock<std::mutex> l(occupy_mtx);
    //  printf("%d\n", send_list.skb_list.size());
    // printf("retransmit\n");
    clear_send_list();
    if (send_list.skb_list.empty())
    {
        retransmit->ignore = true;
        return;
    }
    skbuff *skb = send_list.get_first_skbuff();
    skb->data_start = (char *)skb->tcp_hdr;
    printf("retransmit %d %d\n", skb->tcp_hdr->seq, un_ack_seq);
    ip_send(this, skb);
    // printf("%d\n", send_list.skb_list.size());
    if (send_list.skb_list.empty())
    {

        retransmit->ignore = true;
        return;
    }
    else
    {
        if (tcp_state == TCP_SYN_SENT)
            retransmit->update(RETRAN_CONN_TIME);
        else
            retransmit->update(RETRANSMIT_TIME);
    }
}
void tcp_sock::timewait_fcn()
{
    destroy_sock();
}
bool tcp_sock::test_if_could_read(int len)
{
    // std::unique_lock<std::mutex> l(recv_list.mtx);
    int sum = 0;
    for (skbuff *skb : recv_list.skb_list)
    {
        tcp_hdr *tcp = skb->tcp_hdr;
        if (tcp->psh)
            return true;
        sum += skb->payload_len;
        if (sum >= len)
            return true;
    }
    if (recv_list.skb_list.empty() && (tcp_flag & TCP_FLAG_FIN || tcp_flag & TCP_FLAG_RST))
        return true;
    return false;
}
void tcp_sock::enter_into_timewait()
{
    printf("enter into timewait\n");
    tcp_state = TCP_TIME_WAIT;
    time_wait->update(TIMEWAIT_TIME);
    time_wait->ignore = false;
}
void tcp_sock::destroy_sock()
{
    tcp_state = TCP_CLOSE;
    if (closed)
    {
        if (bind_state == TCP_BIND)
        {
            tcp_manage::get()->remove_sock((this));
        }
        delete this;
    }
}
bool tcp_sock::filter_seq(tcp_hdr *tcp)
{
    return (tcp->seq < expect_seq || tcp->seq >= expect_seq + windows_recv);
}
bool tcp_sock::updata_ack(tcp_hdr *tcp)
{
    if (tcp->ack_seq > un_ack_seq && tcp->ack_seq <= next_seq)
    {
        un_ack_seq = tcp->ack_seq;
        return true;
    }
    return false;
}
bool tcp_sock::filter_flag(tcp_hdr *tcp)
{
    return (tcp->syn || !tcp->ack);
}
bool tcp_sock::filter_rst(tcp_hdr *tcp, skbuff *skb)
{
    if (tcp->rst)
    {
        tcp_flag |= TCP_FLAG_RST;
        delete skb;
        tcp_state = TCP_CLOSE;
        destroy_sock();
        return true;
    }
    return false;
}
void tcp_sock::recv_while_establish(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    if (filter_seq(tcp))
    {
        send_ack();
        delete skb;
        return;
    }
    if (filter_flag(tcp))
    {
        delete skb;
        return;
    }
    if (filter_rst(tcp, skb))
    {
        return;
    }
    updata_ack(tcp);
    if (tcp->fin)
    {
        if (tcp->seq != expect_seq)
        {
            delete skb;
            return;
        }
        if (skb->payload_len > 0)
        {
            adjust_unordered_list(skb);
        }
        tcp_state = TCP_CLOSE_WAIT;
        tcp_flag |= TCP_FLAG_FIN;
        expect_seq += 1;
        send_ack();
        if (skb->payload_len == 0)
            delete skb;
        occupy_cv.notify_all();
    }
    else
    {
        if (skb->payload_len > 0)
        {
            adjust_unordered_list(skb);
            send_ack();
            occupy_cv.notify_all();
        }
        else
        {
            delete skb;
            return;
        }
    }
}
void tcp_sock::recv_while_finwait1(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    if (filter_seq(tcp))
    {
        send_ack();
        delete skb;
        return;
    }
    if (filter_flag(tcp))
    {
        delete skb;
        return;
    }
    if (filter_rst(tcp, skb))
    {
        return;
    }
    updata_ack(tcp);
    if (tcp->fin && expect_seq != tcp->seq)
    {

        delete skb;
        return;
    }
    if (tcp->fin)
    {

        if (skb->payload_len > 0)
        {
            adjust_unordered_list(skb);
        }
        if (un_ack_seq == next_seq)
        {

            tcp_state = TCP_TIME_WAIT;
            enter_into_timewait();
        }
        else
        {
            tcp_state = TCP_CLOSING;
        }
        tcp_flag |= TCP_FLAG_FIN;
        expect_seq += 1;
        occupy_cv.notify_all();
        if (skb->payload_len == 0)
            delete skb;
        send_ack();
        return;
    }
    if (next_seq == un_ack_seq)
    {
        tcp_state = TCP_FIN_WAIT_2;
        delete skb;
    }
    if (skb->payload_len > 0)
    {
        adjust_unordered_list(skb);
        occupy_cv.notify_all();
        send_ack();
    }
    else
        delete skb;
}
void tcp_sock::recv_while_finwait2(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    if (filter_seq(tcp))
    {
        send_ack();
        delete skb;
        return;
    }
    if (filter_flag(tcp))
    {
        delete skb;
        return;
    }
    if (filter_rst(tcp, skb))
    {
        return;
    }
    updata_ack(tcp);
    if (tcp->fin && expect_seq != tcp->seq)
    {
        delete skb;
        return;
    }
    if (tcp->fin)
    {
        if (skb->payload_len > 0)
        {
            adjust_unordered_list(skb);
        }
        tcp_flag |= TCP_FLAG_FIN;
        tcp_state = TCP_TIME_WAIT;
        expect_seq += 1;
        enter_into_timewait();
        occupy_cv.notify_all();
        if (skb->payload_len == 0)
            delete skb;
        send_ack();
        return;
    }
    else if (skb->payload_len > 0)
    {
        adjust_unordered_list(skb);
        occupy_cv.notify_all();
        send_ack();
    }
    else
    {
        delete skb;
    }
}
void tcp_sock::recv_while_closewait(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    if (filter_seq(tcp))
    {
        send_ack();
        delete skb;
        return;
    }
    if (filter_flag(tcp))
    {
        delete skb;
        return;
    }
    if (filter_rst(tcp, skb))
    {
        return;
    }
    updata_ack(tcp);
}
void tcp_sock::recv_while_last_ack(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    if (filter_seq(tcp))
    {
        send_ack();
        delete skb;
        return;
    }
    if (filter_flag(tcp))
    {
        delete skb;
        return;
    }
    if (filter_rst(tcp, skb))
    {
        return;
    }
    updata_ack(tcp);
    if (un_ack_seq == next_seq)
    {
        destroy_sock();
    }
}
void tcp_sock::recv_while_closing(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    if (filter_seq(tcp))
    {
        send_ack();
        delete skb;
        return;
    }
    if (filter_flag(tcp))
    {
        delete skb;
        return;
    }
    if (filter_rst(tcp, skb))
    {
        return;
    }
    updata_ack(tcp);
    if (un_ack_seq == next_seq)
    {
        tcp_state = TCP_TIME_WAIT;
        enter_into_timewait();
    }
}
void tcp_sock::recv_while_timewait(skbuff *skb, tcp_hdr *tcp, ip_hdr *ip)
{
    if (filter_seq(tcp))
    {
        send_ack();
        delete skb;
        return;
    }
    if (filter_flag(tcp))
    {
        delete skb;
        return;
    }
    if (filter_rst(tcp, skb))
    {
        return;
    }
    if (tcp->ack_seq < next_seq)
        send_ack();
}