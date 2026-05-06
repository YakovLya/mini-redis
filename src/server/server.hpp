#pragma once

#include "commands/processor.hpp"
#include "storage/storage.hpp"
#include "thread_pool/thread_pool.hpp"
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <csignal>

extern volatile std::sig_atomic_t shutdown_requested;

struct ClientData{
    std::deque<char> buffer;
    std::chrono::system_clock::time_point last_activity;
    std::atomic<int> active_tasks{0};
};

class Server{
private:
    int server_fd {-1};
    int epoll_fd {-1};
    Storage& storage_;
    CommandProcessor& processor_;
    std::unordered_map<int, ClientData> client_buffers_;
    ThreadPool pool_;
    std::mutex mutex_;
    std::atomic<bool> running_ {true};

    bool new_connection();
    bool new_bytes(int client_fd);
    void clean_idle_clients();
    
public:
    explicit Server(int port, Storage& db, CommandProcessor& processor, size_t threads_num);
    void run();
    void stop();
    ~Server();
};