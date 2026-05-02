#pragma once

#include "storage/storage.hpp"
#include <string>
#include <string_view>
#include <vector>

class CommandProcessor{
private:
    Storage& storage_;

    std::vector<std::string_view> get_tokens(std::string_view query);
    std::string to_upper(std::string str);

public:
    explicit CommandProcessor(Storage& storage);
    std::string execute(std::string_view query);
};