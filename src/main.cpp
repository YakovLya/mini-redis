#include "server/server.hpp"
#include "storage/storage.hpp"
#include "commands/processor.hpp"

#include <iostream>

const int PORT = 4242;

int main() {
    try {
        Storage db;
        CommandProcessor processor(db);
        Server server(PORT, db, processor);
        server.run();
    } catch (const std::exception& err) {
        std::cerr << err.what() << '\n';
    }
    return 0;
}