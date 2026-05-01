#include <deque>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unordered_map>

const int PORT = 4242;
const int BUFFER_SIZE = 4096;

class Server {
private:
    int server_fd = -1;
    int epoll_fd = -1;

public:

    Server(int port) {
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

        std::cout << "server listening on the port: " << port << '\n';
    }

    void run() {
        std::unordered_map<int, std::deque<char>> client_buffers;

        while (true) {
            struct epoll_event events[64];
            int n = epoll_wait(epoll_fd, events, 64, -1);

            for (int i = 0; i < n; ++i) {
                if (events[i].data.fd == server_fd) {
                    sockaddr_in client_addr{};
                    socklen_t addr_len = sizeof(client_addr);

                    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
                    if (client_fd < 0) {
                        std::cerr << "new client accept error\n";
                        continue;
                    }

                    int flags = fcntl(client_fd, F_GETFL, 0);
                    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

                    struct epoll_event event;
                    event.events = EPOLLIN;
                    event.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) < 0) {
                        std::cerr << "epoll add client error\n";
                    }

                    client_buffers[client_fd].clear();

                    std::cout << "new client accepted! ( " << client_fd << " )\n";       
                } else {
                    int client_fd = events[i].data.fd;
                    char buffer[BUFFER_SIZE];
                    ssize_t bytes_read = read(client_fd, &buffer, BUFFER_SIZE);
                    if (bytes_read == 0) {
                        std::cout << "client closed connection ( " << client_fd << " )\n";
                        close(client_fd);
                        continue;
                    }

                    if (bytes_read < 0) {
                        std::cout << "connection shuttered ( " << client_fd << " )\n";
                        close(client_fd);
                        continue;
                    }

                    auto& client_buffer = client_buffers[client_fd];
                    client_buffer.insert(
                        client_buffer.end(), 
                        buffer,
                        buffer + bytes_read
                    );

                    std::cout << "read from client ( " << client_fd << " )\n" << buffer; 
                    while (true) {
                        auto it = std::find(client_buffer.begin(),client_buffer.end(), '\n');
                        if (it == client_buffer.end()) {
                            break;
                        }
                        std::string command(client_buffer.begin(), it);

                        if (!command.empty() && command.back() == '\r') {
                            command.pop_back();
                        }

                        client_buffer.erase(client_buffer.begin(), std::next(it));

                        std::cout << "got new command ( " << client_fd << " ): " << command << '\n';
                    } 

                    write(client_fd, "+OK\r\n", 5);
                }
            }
        }
    }

    ~Server() {
        if (server_fd != -1)
            close(server_fd);
    }
};

int main() {
    try {
        Server server(PORT);
        server.run();
    } catch (const std::exception& err) {
        std::cerr << err.what() << '\n';
    }
    return 0;
}