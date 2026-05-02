#pragma once

#include "commands/processor.hpp"
#include "storage/storage.hpp"
#include "thread_pool/thread_pool.hpp"
#include <chrono>
#include <deque>

struct ClientBuffer{
    std::deque<char> buffer;
    std::chrono::steady_clock::time_point last_activity;
};

class Server{
private:
    int server_fd = -1;
    int epoll_fd = -1;
    Storage& storage_;
    CommandProcessor& processor_;
    std::unordered_map<int, ClientBuffer> client_buffers_;
    ThreadPool pool_;

    bool new_connection();
    bool new_bytes(int client_fd);
    void clean_idle_clients();
    
public:
    explicit Server(int port, Storage& db, CommandProcessor& processor, size_t threads_num);
    void run();
    ~Server();
};