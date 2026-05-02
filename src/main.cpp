#include "server/server.hpp"
#include "storage/storage.hpp"
#include "commands/processor.hpp"
#include "config.hpp"
#include "utils/logger.hpp"

int main() {
    try {
        Storage db;
        CommandProcessor processor(db);
        Server server(config::PORT, db, processor, config::THREADS_NUM);
        server.run();
    } catch (const std::exception& err) {
        Logger::log(LogLevel::ERR, "init error: " + std::string(err.what()));
    }
    return 0;
}