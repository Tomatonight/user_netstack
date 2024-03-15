#include "tcp.h"
#include "socket.h"

void tcp_hdr::tcp_recv(skbuff *skb)
{
    tcp_hdr *tcp = tcp_header(skb);
    ip_hdr *ip = ip_hdr::ip_header(skb);
    // checksum

    tcp->swap();
    skb->data_len = ip->len - ip->ihl * 4 - tcp->hl * 4;
    skb->data_start = (char *)tcp + tcp->hl * 4;
    skb->tcp_start=(char*)tcp;
    tcp_sock *sock = tcp_manage::search_sock(htonl(ip->daddr), htonl(ip->saddr),
                                             htons(tcp->dport), htons(tcp->sport));
    if (!sock)
    {
        sock = tcp_manage::search_sock(ip->saddr, 0, tcp->sport, 0);
    }
    if (!sock)
    {
        printf("find no sock\n");
        delete skb;
        return;
    }

    sock->process_tcp_packet(skb, tcp);
}
int tcp_sock::tcp_transmit_directly(skbuff *skb, uint32_t seq, tcp_hdr *tcp, bool whether_add)
{

    tcp->sport = htons(sport);
    tcp->dport = htons(dport);
    tcp->seq = tcb_.send_seq;
    skb->seq = tcb_.send_seq;
    tcp->ack_seq = tcb_.expect_recv_seq;
    if (!tcp->hl)
        tcp->hl = TCP_HDR_LEN / 4;
    tcp->rsvd = 0;
    tcp->csum = 0;
    skb->protocol = IP_TCP;
    tcp->win = tcb_.windows_recv;
    tcp->urp = 0;
    tcp->swap();
    skb->tcp_start=(char*)tcp;
    tcp->csum = utils::checksum_tran((saddr), (daddr), IP_TCP, skb->data_start, skb->data_end - skb->data_start);
    ip_hdr::ip_send(this, skb);
    if (whether_add)
        send_queue.add_skbuff(skb);
    return 0;
}
int tcp_sock::tcp_transmit(skbuff *skb, tcp_hdr *tcp)
{

    if (state != TCP_SYN_SENT)
    {

        set_retransmit_timer(RETRANSMIT_TIMEOUT, std::bind(&tcp_sock::send_first_packet_in_send_queue, this));
    }
    else
    {

        set_retransmit_timer(CONNECT_TIMEOUT, std::bind(&tcp_sock::re_connect, this));
    }
    int re = tcp_transmit_directly(skb, tcb_.send_seq, tcp, true);
    tcb_.send_seq += skb->data_len;
    return re;
}
int tcp_sock::send_syn()
{
    printf("send syn\n");
    if (state != TCP_LISTEN && state != TCP_CLOSE && state != TCP_SYN_RECEIVED)
    {
        printf("send syn err\n");
        return 0;
    }
    state = TCP_SYN_SENT;
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN + sizeof(tcp_opt_mss), 0);
    skb->expand_s(sizeof(tcp_opt_mss));
    tcp_opt_mss *opt_mss = (tcp_opt_mss *)(skb->data_start);
    opt_mss->kind = 2;
    opt_mss->len = 4;
    opt_mss->mss = htons(r_max_msg_size);
    skb->data_len = 0;
    skb->expand_s(TCP_HDR_LEN);
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->syn = 1;
    tcp->hl = 1 + TCP_HDR_LEN / 4;
    int re = tcp_transmit(skb, tcp);
    tcb_.send_seq += 1;
    return re;
}
int tcp_sock::send_synack()
{
    printf("send syn ack\n");
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN, 0);
    skb->data_len = 0;

    skb->expand_s(TCP_HDR_LEN);
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->syn = 1;
    tcp->ack = 1;

    int re = tcp_transmit(skb, tcp);
    tcb_.send_seq += 1;
    delete skb;
    return re;
}
int tcp_sock::send_fin()
{
    printf("send fin\n");
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN, 0);
    skb->data_len = 0;

    skb->expand_s(TCP_HDR_LEN);
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->fin = 1;
    tcp->ack = 1;
    int re = tcp_transmit(skb, tcp);
    tcb_.send_seq += 1;
    return re;
}
int tcp_sock::send_ack()
{
    tcb_.last_send_ack=tcb_.expect_recv_seq;
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN, 0);
    skb->data_len = 0;

    skb->expand_s(TCP_HDR_LEN);
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->ack = 1;
    int re = tcp_transmit_directly(skb, tcb_.send_seq, tcp, false);
    delete skb;
    return re;
}
int tcp_sock::send_reset(uint32_t daddr_temp, uint32_t saddr_temp)
{

    std::swap(daddr, daddr_temp);
    std::swap(saddr, saddr_temp);
    send_reset();
    std::swap(daddr, daddr_temp);
    std::swap(saddr, saddr_temp);
}
int tcp_sock::send_reset()
{
    printf("send reset\n");
    skbuff *skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN, 0);
    skb->data_len = 0;

    skb->expand_s(TCP_HDR_LEN);
    tcp_hdr *tcp = (tcp_hdr *)(skb->data_start);
    tcp->ack = 1;
    tcp->rst = 1;
    tcb_.expect_recv_seq = tcb_.send_seq;
    int re = tcp_transmit_directly(skb, tcb_.send_seq, tcp, false);
    delete skb;
    return re;
}
void tcp_sock::adjust_send_queue()
{
    while (true)
    {
        skbuff *skb = send_queue.reserve_skbuff();
        if (skb && skb->seq < tcb_.un_ack_seq)
        {

            delete send_queue.pop_skbuff();
        }
        else
            break;
    }
}

int tcp_sock::recv_while_close(skbuff *skb, tcp_hdr *tcp)
{
    ip_hdr *ip = ip_hdr::ip_header(skb);
    if (!tcp->rst)
        send_reset(htonl(ip->daddr), htonl(ip->saddr));
    delete skb;
}
int tcp_sock::recv_while_listen(skbuff *skb, tcp_hdr *tcp)
{
    ip_hdr *ip = ip_hdr::ip_header(skb);
    if (tcp->rst)
    {
        delete skb;
        return 0;
    }
    if (tcp->ack || !tcp->syn)
    {
        send_reset(htonl(ip->daddr), htonl(ip->saddr));
        delete skb;
        return 0;
    }
    tcp_sock *new_sock = new tcp_sock(this);
    tcp_manage::add_sock(new_sock);
    new_sock->saddr = saddr;
    new_sock->daddr = ip->saddr;
    new_sock->sport = sport;
    new_sock->dport = tcp->sport;
    tcb *new_tcb = &new_sock->tcb_;
    new_tcb->recv_start = tcp->seq;
    new_tcb->expect_recv_seq = tcp->seq + 1;
    new_tcb->init_send_seq();
    new_tcb->un_ack_seq = tcp->seq;
    new_sock->send_synack();
}
int tcp_sock::recv_while_synsent(skbuff *skb, tcp_hdr *tcp)
{
    
    if (tcp->rst)
    {
        state=TCP_CLOSE;
        flag|=TCP_RST;
        run_read_task();
        delete this;
        delete skb;
        return 0;
    }
    if (tcp->ack && tcp->syn)
    {

        if (tcp->ack_seq != tcb_.send_seq)
        {

            delete skb;
            return 0;
        }
        tcb_.recv_start = tcp->seq;
        tcb_.expect_recv_seq = tcp->seq + 1;
        tcb_.un_ack_seq = tcp->ack_seq;

        adjust_send_queue();
        send_ack();
        state = TCP_ESTABLISHED;
        run_read_task();
    }
    delete skb;
    return 0;
}
int tcp_sock::process_tcp_packet(skbuff *skb, tcp_hdr *tcp)
{
    printf("recv tcp packet\n");
    switch (state)
    {
    case TCP_CLOSE:
        printf("recv close\n");
        return recv_while_close(skb, tcp);
    case TCP_LISTEN:
        printf("recv listen\n");
        return recv_while_listen(skb, tcp);
    case TCP_SYN_SENT:
        printf("recv sent\n");
        return recv_while_synsent(skb, tcp);
    }
    if (tcp->seq < tcb_.expect_recv_seq || tcp->seq > tcb_.expect_recv_seq + tcb_.windows_recv)
    {
        printf("filter 1\n");
        send_ack();
        delete skb;
        return 0;
    }
    if (tcp->ack_seq < tcb_.un_ack_seq || tcp->ack_seq > tcb_.send_seq)
    {

        printf("filter 2\n");
        delete skb;
        return 0;
    }
    if (tcp->rst)
    {
        state=TCP_CLOSE;
        flag|=TCP_RST;
        run_read_task();
        printf("rst drop\n");
        delete skb;
        delete this;
        return 0;
    }
    if (!tcp->ack || tcp->syn)
    {
        printf("ack drop\n");
        send_reset();
        delete skb;
        delete skb;
        return 0;
    }

    if (tcp->fin)
    {
        printf("recv fin\n");
        if (tcp->ack_seq > tcb_.un_ack_seq)
        {
            tcb_.un_ack_seq = tcp->ack_seq;
        }
        if (tcp->seq != tcb_.expect_recv_seq)
        {
            printf("fin drop\n");
            delete skb;
            return 0;
        }

        switch (state)
        {
        case TCP_ESTABLISHED:
        case TCP_SYN_RECEIVED:
            state = TCP_CLOSE_WAIT;
            
            flag |= TCP_FIN;
            tcb_.expect_recv_seq += skb->data_len+1;
            send_ack();
            if(skb->data_len>0)recv_queue.add_skbuff(skb);
            else
            delete skb;
            run_read_task();
            return 0;
        case TCP_FIN_WAIT_1:
            
            flag |= TCP_FIN;
            tcb_.expect_recv_seq += skb->data_len+1;
            send_ack();
            enter_into_timewait();
            if(skb->data_len>0)recv_queue.add_skbuff(skb);
            else
            delete skb;
            run_read_task();
            return 0;
        case TCP_FIN_WAIT_2:
            
            flag |= TCP_FIN;
            tcb_.expect_recv_seq += skb->data_len+1;
            
            send_ack();
            enter_into_timewait();
         
            if(skb->data_len>0)recv_queue.add_skbuff(skb);
            else delete skb;
            run_read_task();
            return 0;
        case TCP_TIME_WAIT:
            send_ack();
            delete skb;
            return 0;
        }
    }
    switch (state)
    {
    case TCP_SYN_RECEIVED:
    {
        if (tcp->seq != tcb_.expect_recv_seq || tcp->ack_seq != tcb_.send_seq)
        {
            delete skb;
            return 0;
        }
        state = TCP_ESTABLISHED;
        break;
        //-----------------------
    }
    case TCP_CLOSE_WAIT:
    case TCP_LAST_ACK:
    {
       
        if (skb->data_len > 0&&tcp->seq>=tcb_.expect_recv_seq)
        {
            
            send_reset();
            delete this;
            delete skb;
            return 0;
        }
        else
        {
            if (tcp->ack_seq > tcb_.un_ack_seq && tcp->ack_seq <= tcb_.send_seq)
                tcb_.un_ack_seq = tcp->ack_seq;
        }
        if (state == TCP_LAST_ACK && tcb_.un_ack_seq == tcb_.send_seq)
        {
            delete this;
        }
        delete skb;
        return 0;
    }
    case TCP_FIN_WAIT_2:
    case TCP_FIN_WAIT_1:
    case TCP_ESTABLISHED:
    {
        if (tcp->ack_seq > tcb_.un_ack_seq)
        {
            tcb_.un_ack_seq = tcp->ack_seq;
            adjust_send_queue();
            if (state == TCP_FIN_WAIT_1)
            {
                state = TCP_FIN_WAIT_2;
            }
        }
        if (skb->data_len > 0)
        {
            if(state==TCP_FIN_WAIT_1||state==TCP_FIN_WAIT_2)
            {
                send_reset();
                delete skb;
                delete this;
                return 0;
            }
          set_send_ack_timer();
            skb->seq = tcp->seq;
            if (tcp->seq == tcb_.expect_recv_seq)
            {
                printf("recv data\n");
                tcb_.expect_recv_seq = tcp->seq + skb->data_len;
                recv_queue.add_skbuff(skb);
            }
            else
            {
                printf("recv unordered_data\n");
                add_skbuff_to_unordered_skbuff(skb);
                adjust_unordered_skbuff();
            }
            run_read_task();
            return 0;
        }
    }
    }
    delete skb;
    return 0;
}
bool tcp_sock::test_able_read(int len)
{
    int re = 0;
    std::list<skbuff *>::iterator i;
    FOR_EACH_SKBUFF((&recv_queue), i)
    {
        skbuff *skb = *i;
        tcp_hdr *tcp = (tcp_hdr *)(skb->tcp_start);
        if (tcp->psh)
        {
   
            return true;
        }
   
        re += skb->data_len;
    
        if(re>=len)break;
    }
    return re >= len;
}
int tcp_sock::get_date_from_recv_queue(char *buf, size_t len)
{

    int re = 0;
    while (re < len)
    {
        skbuff *skb = recv_queue.reserve_skbuff();
        if (!skb)
        {
            return re;
        }
        if (skb->data_len + re >= len)
        {
            int out = len - re;
            memcpy(buf + re, skb->data_start, out);
            skb->data_start += out;
            skb->data_len -= out;
            re = len;
            return re;
        }
        else
        {
            recv_queue.pop_skbuff();
            memcpy(buf + re, skb->data_start, skb->data_len);
            re += skb->data_len;
            skb->data_start += skb->data_len;
            delete skb;
        }
    }
    return re;
}

void tcp_sock::adjust_unordered_skbuff()
{
    skbuff *skb = unordered_skbuff.reserve_skbuff();
    if (skb->seq < tcb_.expect_recv_seq)
    {
        printf("adjust err\n");
        return;
    }
    while (skb && skb->seq == tcb_.expect_recv_seq)
    {
        tcb_.expect_recv_seq += skb->data_len;
        recv_queue.add_skbuff(skb);
        unordered_skbuff.pop_skbuff();
        skb = unordered_skbuff.reserve_skbuff();
    }
}
void tcp_sock::add_skbuff_to_unordered_skbuff(skbuff *skb)
{
    unordered_skbuff.insert_skbuff_depend_seq(skb);
}
int tcp_sock::read_(char *buf, int len)
{
    printf("read state %d\n",state);
    switch (state)
    {
    case TCP_CLOSE:
        return -1;
    case TCP_CLOSING:
    case TCP_LAST_ACK:
    case TCP_TIME_WAIT:
    case TCP_LISTEN:
    case TCP_SYN_SENT:
    case TCP_SYN_RECEIVED:
        return -1;
    case TCP_CLOSE_WAIT:
        
        if (flag & TCP_FIN)
        {
            if (!recv_queue.empty())
                break;
            return 0;
        }
        if (recv_queue.empty())
        {
            return -1;
        }
    case TCP_ESTABLISHED:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
        break;
    }
    if (!test_able_read(len))
    {
        return -2;
    }
    return get_date_from_recv_queue(buf, len);
}
int tcp_sock::write_(char *buf, int len)
{
    if (state != TCP_ESTABLISHED && state != TCP_CLOSE_WAIT)
    {
        return -1;
    }
    int re = 0;
    while (re < len)
    {
        int add = s_max_msg_size > len - re ? len - re : s_max_msg_size;

        skbuff *new_skb = new skbuff(ETH_HDR_LEN + IP_HDR_LEN + TCP_HDR_LEN + add, 0);
        new_skb->expand_s(add);
        memcpy(new_skb->data_start, buf + re, add);
        new_skb->data_len = add;
        new_skb->expand_s(TCP_HDR_LEN);
        tcp_hdr *tcp = (tcp_hdr *)new_skb->data_start;
        tcp->ack = 1;
        re += add;
        if (re >= len)
        {
            tcp->psh = 1;
        }
        tcp_transmit(new_skb, tcp);
    }
    return re;
}
int tcp_sock::bind_(sockaddr_in *addr)
{
}
int tcp_sock::connect_(sockaddr_in *addr)
{
    if (!sport)
    {
        sport = htons(tcp_manage::generate_port());
        saddr = htonl(netdev::netdev_eth->netdev_ip);
    }
    daddr = (addr->sin_addr.s_addr);
    dport = (addr->sin_port);
    tcp_manage::add_sock(this);
    tcb_.init_send_seq();
    send_syn();
}
int tcp_sock::listen_(int)
{
}
int tcp_sock::accept_()
{
}
void tcp_sock::set_send_ack_timer()
{
    if(tcb_.last_send_ack==tcb_.expect_recv_seq)
    {
        return;
    }
    std::function<void()> fcn = [&]()
    {

        if (tcb_.expect_recv_seq == tcb_.un_ack_seq)
        {
            timer_ctl::set_timer(send_ack_timer, true);
            return;
        }
        send_ack();

        timer_ctl::update_timer(send_ack_timer, 0, fcn);
    };
    if (!send_ack_timer)
    {
        retransmit_timer = timer_ctl::add_timer(0, fcn);
    }
      timer_ctl::set_timer(send_ack_timer, false);
    
}
void tcp_sock::cancel_send_ack_timer()
{
    send_ack_timer->cancel = true;
}

void tcp_sock::set_retransmit_timer(int seconds, std::function<void()> fcn)
{
    
    if (!retransmit_timer)
    {
        retransmit_timer = timer_ctl::add_timer(seconds, fcn);
    }
    else
    {
        timer_ctl::update_timer(retransmit_timer, seconds, fcn);
    }
}
void tcp_sock::send_first_packet_in_send_queue()
{

    skbuff *skb = send_queue.reserve_skbuff();

    if (!skb)
    {
        cancel_retransmit_timer();
        return;
    }

    set_retransmit_timer(RETRANSMIT_TIMEOUT,
                         std::bind(&tcp_sock::send_first_packet_in_send_queue, this));

    skb->data_start=skb->tcp_start;
       printf("re tran \n");
    ip_hdr::ip_send(this, skb);
      printf("re tran end\n");
}
void tcp_sock::cancel_retransmit_timer()
{
    retransmit_timer->cancel = true;
}
void tcp_sock::re_connect()
{
  
    if (state != TCP_ESTABLISHED)
    {
        if (backoff > CONNECT_TIMEOUT_TIMES)
        {
            state = TCP_CLOSE;
            run_read_task();
        }
        else
        {
            if (state != TCP_SYN_SENT)
                return;
            send_first_packet_in_send_queue();
            backoff++;
            timer_ctl::update_timer(this->retransmit_timer, CONNECT_TIMEOUT * backoff, std::bind(&tcp_sock::re_connect, this));
        }
    }
  
}
void tcp_sock::clear_listen_sock()
{

    /*
     while (true)
     {
         tcp_sock *sock = recvsyn_queue.pop_sock();
         if (!sock)
             break;
         sock->send_reset();
           sock->clear_sock();
         delete sock;
     }*/
    while (true)
    {
        tcp_sock *sock = accpet_queue.pop_sock();
        if (!sock)
            break;
        sock->close_();
        delete sock;
    }
    clear_timer();
    tcp_manage::remove_sock(this);
}

void tcp_sock::close_()
{

    switch (state)
    {
    case TCP_CLOSE:
        tcp_manage::remove_sock(this);
        delete this;
        return;
    case TCP_CLOSING:
    case TCP_LAST_ACK:
    case TCP_TIME_WAIT:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
        return;
    case TCP_LISTEN:
        //  clear_listen_sock();
        break;
    case TCP_SYN_SENT:
    case TCP_SYN_RECEIVED:
    case TCP_ESTABLISHED:
        send_fin();
        state = TCP_FIN_WAIT_1;
        break;
    case TCP_CLOSE_WAIT:
        send_fin();
        state = TCP_LAST_ACK;
        break;
    }
    return;
}

void tcp_sock::enter_into_timewait()
{
    printf("enter into timewait\n");
    state = TCP_TIME_WAIT;
    auto task = [=]()
    {
        printf("timewait clear sock\n");
        delete this;
        return;
    };
    timer_ctl::update_timer(retransmit_timer, TIME_WAIT_TIME, task);
}
void tcp_sock::clear_timer()
{
    if (send_ack_timer)
        timer_ctl::delete_timer(send_ack_timer);
    if (retransmit_timer)
        timer_ctl::delete_timer(retransmit_timer);
}
void tcp_sock::clear_sock()
{
    
    clear_timer();
    if (skt)
    {
        skt->sock = nullptr;
    }
      
    if (sport && dport)
        tcp_manage::remove_sock(this);
       
}
void tcp_sock::run_read_task()
{
    if (read_task!=nullptr)
    {
        read_task();
        read_task = nullptr;
    }
}