#include "aof_manager/aof_manager.hpp"
#include "server/server.hpp"
#include "storage/storage.hpp"
#include "commands/processor.hpp"
#include "config.hpp"
#include "utils/logger.hpp"
#include <csignal>

volatile std::sig_atomic_t shutdown_requested = 0;

void signal_handler(int signal){
    if ((signal == SIGINT || signal == SIGTERM)) {
        shutdown_requested = 1;
    }
}

int main() {
    try {
        Storage db;
        AofManager aof_manager(config::SAVEFILE_NAME);
        CommandProcessor processor(db, &aof_manager);
        Server server(config::PORT, db, processor, config::THREADS_NUM);

        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        server.run();
    } catch (const std::exception& err) {
        Logger::log(LogLevel::ERR, "init error: " + std::string(err.what()));
    }
    return 0;
}