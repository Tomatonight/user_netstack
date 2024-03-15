/*
#pragma once
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
class read_tasks
{
public:
    read_tasks()
    {
    }
    ~read_tasks()
    {
    }
    void add_task(std::function<void()> task)
    {
        std::unique_lock<std::mutex> l(mtx);
        tasks.push(task);
    }
    std::function<void()> pop_task()
    {
        std::unique_lock<std::mutex> l(mtx);
        if (tasks.empty())
            return nullptr;
        std::function<void()> task = tasks.front();
        tasks.pop();
        return task;
    }

private:
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
};
*/