#include "aof_manager/aof_manager.hpp"
#include "server/server.hpp"
#include "storage/storage.hpp"
#include "commands/processor.hpp"
#include "config.hpp"
#include "utils/logger.hpp"

int main() {
    try {
        Storage db;
        AofManager aof_manager(config::SAVEFILE_NAME);
        CommandProcessor processor(db, &aof_manager);
        Server server(config::PORT, db, processor, config::THREADS_NUM);
        server.run();
    } catch (const std::exception& err) {
        Logger::log(LogLevel::ERR, "init error: " + std::string(err.what()));
    }
    return 0;
}