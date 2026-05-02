#include "server.hpp"
#include "config.hpp"
#include "thread_pool/thread_pool.hpp"
#include "utils/logger.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

bool Server::new_connection() {
    sockaddr_in client_addr{};
    socklen_t addr_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
    if (client_fd < 0) {
        Logger::log(LogLevel::ERR, "new client accept error");
        return false;
    }

    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
        Logger::log(LogLevel::ERR, "epoll add client error");
        return false;
    }

    client_buffers_[client_fd].buffer.clear();
    client_buffers_[client_fd].last_activity = std::chrono::steady_clock::now();

    Logger::log(LogLevel::INFO, "new client accepted, fd = " + std::to_string(client_fd));
    return true;     
}

bool Server::new_bytes(int client_fd) {
    char buffer[config::BUFFER_SIZE];
    ssize_t bytes_read = read(client_fd, &buffer, sizeof(buffer));
    if (bytes_read == 0) {
        Logger::log(LogLevel::INFO, "client closed connection, fd = " + std::to_string(client_fd));
        close(client_fd);
        return false;
    }

    if (bytes_read < 0) {
        Logger::log(LogLevel::INFO, "connection shuttered, fd = " + std::to_string(client_fd));
        close(client_fd);
        return false;
    }

    auto& client_buffer = client_buffers_[client_fd].buffer;
    client_buffer.insert(
        client_buffer.end(), 
        buffer,
        buffer + bytes_read
    );
    client_buffers_[client_fd].last_activity = std::chrono::steady_clock::now();

    while (true) {
        auto it = std::find(client_buffer.begin(),client_buffer.end(), '\n');
        if (it == client_buffer.end()) {
            break;
        }
        std::string query(client_buffer.begin(), it);

        if (!query.empty() && query.back() == '\r') {
            query.pop_back();
        }

        client_buffer.erase(client_buffer.begin(), std::next(it));

        pool_.add_task([this, client_fd, query]() {
            std::string response = processor_.execute(query);
            write(client_fd, response.c_str(), response.size());
        });
    } 

    return true;
}

void Server::clean_idle_clients() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = client_buffers_.begin(); it != client_buffers_.end(); ) {
        auto duration = std::chrono::duration_cast<std::chrono::seconds>
        (now - it->second.last_activity).count();

        if (duration > config::IDLE_CLIENT_TTL) {
            Logger::log(LogLevel::INFO, "clean idle client's buffer, fd = " + std::to_string(it->first));
            close(it->first);
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, it->first, nullptr);
            it = client_buffers_.erase(it);
        } else {
            ++ it;
        }
    }
}

Server::Server(int port, Storage& db, CommandProcessor& processor,
                 size_t threads_num) : storage_(db), processor_(processor), pool_(threads_num) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        throw std::runtime_error("socket init error");
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        throw std::runtime_error("bind error");
    }

    if (listen(server_fd, 128) < 0) {
        throw std::runtime_error("listen error");
    }

    epoll_fd = epoll_create1(0);
    if (epoll_fd < 0) {
        throw std::runtime_error("epoll create error");
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) < 0) {
        throw std::runtime_error("epoll add server error");
    }

    Logger::log(LogLevel::INFO, "server listening on the port: " + std::to_string(port));
}

void Server::run() {
    while (true) {
        struct epoll_event events[64];
        int n = epoll_wait(epoll_fd, events, 64, config::EPOLL_TIMEOUT);

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == server_fd) {
                new_connection();  
            } else {
                new_bytes(events[i].data.fd);
            }
        }

        clean_idle_clients();
        storage_.active_clean(config::ACTIVE_CLEAN_PER_LOOP);
    }
}

Server::~Server() {
    if (server_fd != -1)
        close(server_fd);
}