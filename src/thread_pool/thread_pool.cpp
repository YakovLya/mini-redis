#include "thread_pool.hpp"
#include <functional>
#include <mutex>


ThreadPool::ThreadPool(std::size_t threads) : is_stop_(false) {
    for (size_t i = 0; i < threads; ++i){
        workers_.emplace_back([this] {
            while(true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(tasks_mutex_);
                    condition_.wait(lock, [this] {
                        return is_stop_ || !tasks_.empty();
                    });
                    if (is_stop_ && tasks_.empty()) return;
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
    {
        std::unique_lock<std::mutex> lock(tasks_mutex_);
        is_stop_ = true;
    }
    condition_.notify_all();
    for(std::thread& worker : workers_)
        worker.join();
}
