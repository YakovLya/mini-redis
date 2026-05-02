#include "server/server.hpp"
#include "storage/storage.hpp"
#include "commands/processor.hpp"
#include "config.hpp"

#include <iostream>

int main() {
    try {
        Storage db;
        CommandProcessor processor(db);
        Server server(config::PORT, db, processor);
        server.run();
    } catch (const std::exception& err) {
        std::cerr << err.what() << '\n';
    }
    return 0;
}