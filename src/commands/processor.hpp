#pragma once

#include "storage/storage.hpp"
#include <string>
#include <vector>

class CommandProcessor{
private:
    Storage storage_;

    std::vector<std::string> get_tokens(const std::string& query);
    std::string to_upper(std::string str);

public:
    explicit CommandProcessor(Storage& storage);
    std::string execute(const std::string& query);
};