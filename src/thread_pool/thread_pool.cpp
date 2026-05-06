#include "thread_pool.hpp"
#include <functional>
#include <mutex>
#include <stop_token>


ThreadPool::ThreadPool(std::size_t threads) {
    for (size_t i = 0; i < threads; ++i){
        workers_.emplace_back([this] (std::stop_token stop) {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(tasks_mutex_);
                    if (!condition_.wait(lock, stop, [this] { return !tasks_.empty(); }))
                        return;
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

void ThreadPool::add_task(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(tasks_mutex_);
        tasks_.emplace(std::move(task));
    }
    condition_.notify_one();
}

ThreadPool::~ThreadPool() {
    for (auto& worker : workers_)
        worker.request_stop();
    condition_.notify_all();
    workers_.clear();
}
