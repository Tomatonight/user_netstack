#include "skbuff.h"
#include<mutex>
#include<thread>
skbuff::skbuff(uint32_t len)
{
  
    start = (char*)malloc(len);
    memset(start, 0, len);
    end = start + len;
    data_start = end;
    data_end = end;
   
}
skbuff::~skbuff()
{
    if (start)
       free(start);

}
void skbuff_list::add_skbuff(skbuff *skb)
{
    std::unique_lock<std::mutex> l(mtx);
    skb_list.push_back(skb);
}
bool skbuff_list::list_empty()
{
    std::unique_lock<std::mutex> l(mtx);
    return skb_list.empty();
}
skbuff *skbuff_list::get_first_skbuff()
{
    std::unique_lock<std::mutex> l(mtx);
    if(skb_list.empty())return nullptr;
    return skb_list.front();
}
void skbuff_list::remove_and_destroy_first_skbuff()
{
    std::unique_lock<std::mutex> l(mtx);
    skbuff *skb = skb_list.front();
    skb_list.pop_front();
    delete skb;
}
skbuff* skbuff_list::remove_first_skbuff()
{
    std::unique_lock<std::mutex> l(mtx);
    skbuff *skb = skb_list.front();
    skb_list.pop_front();
    return skb;
}
skbuff_list::~skbuff_list()
{
    std::unique_lock<std::mutex> l(mtx);
    for (skbuff *skb : skb_list)
        delete skb;
}