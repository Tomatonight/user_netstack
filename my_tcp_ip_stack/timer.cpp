#include "timer.h"
#include "unistd.h"
#include <time.h>
timer_ctl *timer_ctl::timers = nullptr;
void timer_ctl::init()
{
    timers = new timer_ctl;
    while (true)
    {
        usleep(500000);
        timers->tick();
        for (auto task : timers->tasks)
        {
            task();
        }
        timers->tasks.clear();

    }
}
timer *timer_ctl::add_timer(uint32_t time_, std::function<void()> cb)
{
    timer *t = new timer;
    time_t time_now;
    time(&time_now);
    t->expire = time_now + time_;
    t->call_back = cb;
    std::unique_lock<std::shared_mutex> l(timers->shared_mtx);
    timers->lists.push_back(t);
    return t;
}
void timer_ctl::delete_timer(timer *time)
{
    time->remove = true;
}
void timer_ctl::tick()
{
    std::unique_lock<std::shared_mutex> l(timers->shared_mtx);
    time_t now;
    time(&now);

    for (std::list<timer *>::iterator it = lists.begin(); it != lists.end();)
    {
        std::list<timer *>::iterator t = it;
        it++;
        timer *time = *t;
        if (time->remove)
        {
            delete time;
            lists.erase(t);
            continue;
        }
        if (time->cancel)
            continue;
        if (time->expire < now)
        {
            tasks.push_back(time->call_back);
            time->cancel = true;
        }
    }

}
void timer_ctl::set_timer(timer *time, bool cancel)
{
    time->cancel = cancel;
}
void timer_ctl::update_timer(timer *t, uint32_t time_, std::function<void()> cb)
{
  
    time_t time_now;
    time(&time_now);
    t->expire = time_now + time_;
       t->cancel = true;
    t->call_back = cb;
    t->cancel = false;
}