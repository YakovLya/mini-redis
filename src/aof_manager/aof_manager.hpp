#pragma once

#include <fstream>
#include <mutex>
#include <string>
#include <string_view>

class AofManager{
private:
    std::string filename_;
    std::ofstream file_;
    std::mutex mutex_;

public:
    explicit AofManager(std::string_view filename);
    void save(std::string_view query);
    void load(class CommandProcessor* processor);
    ~AofManager();
};