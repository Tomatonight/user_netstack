#pragma once
#include <functional>
#include <list>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <queue>
#include <vector>
#include <atomic>
#include<time.h>
inline uint32_t gernerate_random()
{
    time_t time_;
    time(&time_);
    int t=time_;
    srand(t);
    return rand();
}
class timer
{
public:
    std::function<void()> call_back;
    u_int32_t expire;
    std::atomic<bool> cancel = false;
    bool remove = false;
};
class timer_ctl
{
public:
    static timer_ctl *timers;
    static void init();
    static timer *add_timer(uint32_t time, std::function<void()>);
    static  void delete_timer(timer *);
    static  void set_timer(timer *, bool);
    static  void update_timer(timer *t, uint32_t time_, std::function<void()>);

private:
    void tick();
    std::thread thread;
    std::shared_mutex shared_mtx;
    std::list<timer *> lists;
    std::vector<std::function<void()>> tasks;
};