#pragma once

#include <condition_variable>
#include <functional>
#include <queue>
#include <thread>
#include <vector>

class ThreadPool {
private:
    std::vector<std::jthread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex tasks_mutex_;
    std::condition_variable condition_;

public:

    ThreadPool(size_t threads);
    void add_task(std::function<void()> task);
    ~ThreadPool();
};