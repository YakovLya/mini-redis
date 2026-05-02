#pragma once

#include <iostream>
#include <string>
#include <chrono>
#include <format>

enum class LogLevel { INFO, WARN, ERR, DEBUG };

class Logger {
private:
    static std::string loglevel(LogLevel level) {
        switch (level) {
            case LogLevel::INFO:  return "\033[32mINFO\033[0m";
            case LogLevel::WARN:  return "\033[33mWARN\033[0m";
            case LogLevel::ERR:   return "\033[31mERROR\033[0m";
            case LogLevel::DEBUG: return "\033[36mDEBUG\033[0m";
            default: return "LOG";
        }
    }

public:
    static void log(LogLevel level, const std::string& str) {
        auto now  = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
        std::cout << std::format("[{:%T}] [{}] {}\n", now, loglevel(level), str);
    }
};