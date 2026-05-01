#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

const int PORT = 4242;

class Server {
private:
    int server_fd = -1;

public:

    Server(int port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            throw std::runtime_error("socket init error");
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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

        std::cout << "server listening on the port: " << port << '\n';
    }

    void run() {
        while (true) {
            sockaddr_in client_addr{};
            socklen_t addr_len = sizeof(client_addr);

            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd < 0) {
                std::cerr << "new client accept error\n";
                continue;
            }

            std::cout << "new client accepted!\n";
            close(client_fd);
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