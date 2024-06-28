#include "timer.h"
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
timer *timer::Timer = nullptr;
void timer::init()
{
    Timer = new timer;
    Timer->timer_thread = std::move(std::thread([=]()
                                                {
    while(true)
    {
        Timer->tick();
    } }));
}
timer *timer::get()
{
    return Timer;
}
void time_node::update(uint64_t timeout_)
{
    timeval tv;
    gettimeofday(&tv, NULL);
    timeout = tv.tv_sec * 1000000 + tv.tv_usec + timeout_;
}
void timer::add_time_node(time_node *node)
{
    std::unique_lock<std::mutex> l(mtx);
    timer_list.push_back(node);
}
void timer::remove_time_node(time_node *node)
{
    std::unique_lock<std::mutex> l(mtx);
    timer_list.remove(node);
    //  printf("timer remove error\n");
}
void timer::tick()
{
    usleep(300000);
    timeval tv;
    gettimeofday(&tv, NULL);
    uint64_t now_time = tv.tv_sec * 1000000 + tv.tv_usec;
    //  printf("tick\n");
    std::unique_lock<std::mutex> l(mtx);
    for (time_node *item : timer_list)
    {
        if (item->ignore)
            continue;
        if (item->timeout < now_time)
        {
            item->fcn();
        }
    }
}