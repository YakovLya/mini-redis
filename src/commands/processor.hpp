#pragma once

#include "aof_manager/aof_manager.hpp"
#include "storage/storage.hpp"
#include <string>
#include <string_view>
#include <vector>

class CommandProcessor{
private:
    Storage& storage_;
    class AofManager* aof_manager_;
    bool save_aof_ = false;

    std::vector<std::string_view> get_tokens(std::string_view query);
    std::string to_upper(std::string str);

public:
    explicit CommandProcessor(Storage& storage);
    explicit CommandProcessor(Storage& storage, AofManager* aof_manager);
    std::string execute(std::string_view query);
};