#pragma once
#include<functional>
#include<list>
#include<mutex>
#include<shared_mutex>
#include<atomic>
#include<thread>
struct time_node
{
std::function<void()> fcn=nullptr;
std::atomic<bool> ignore=true;
std::atomic<uint64_t> timeout=0;
void update(uint64_t timeout);
};
class timer
{
    public:
    void add_time_node(time_node* node);
    void remove_time_node(time_node* node);
    void tick();
    static void init();
    static timer* get();
    std::thread timer_thread;
    private:
    static timer* Timer;
    std::mutex mtx;
    std::list<time_node*> timer_list;
};
