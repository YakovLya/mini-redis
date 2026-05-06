#include "aof_manager.hpp"
#include "commands/processor.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string_view>

AofManager::AofManager(std::string_view filename) : filename_(filename) {
    if (!std::filesystem::exists(filename_))
        std::ofstream create_file_(filename_);

    file_.open(filename_, std::ios::app);
    if (!file_.is_open())
        throw std::runtime_error("could not open save file");
}

void AofManager::save(std::string_view query) {
    std::cout << "SAVED|\n";
    std::unique_lock lock(mutex_);

    file_.clear();
    file_.seekp(0, std::ios::end);
    file_ << query << '\n';
    file_.flush();
}

void AofManager::load(class CommandProcessor* processor) {
    std::ifstream load_file_(filename_);
    std::string query;
    while (std::getline(load_file_, query)) {
        processor->execute(query);
    }
}

AofManager::~AofManager() {
    if (file_.is_open())
        file_.close();
}