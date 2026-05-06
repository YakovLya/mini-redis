#include "processor.hpp"
#include "config.hpp"
#include "utils/logger.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string_view>

CommandProcessor::CommandProcessor(Storage& storage) : storage_(storage), aof_manager_(nullptr), save_aof_(false) {}
CommandProcessor::CommandProcessor(Storage& storage, AofManager* aof_manager) 
                    : storage_(storage), aof_manager_(aof_manager), save_aof_(false) {
                        Logger::log(LogLevel::INFO, "start recover from AOF file");
                        aof_manager_->load(this);
                        Logger::log(LogLevel::INFO, "end recover from AOF file");
                        save_aof_ = true;
                    }

std::vector<std::string_view> CommandProcessor::get_tokens(std::string_view query) {
    std::vector<std::string_view> tokens;

    auto start = query.find_first_not_of(' ');
    if (start == std::string_view::npos) return tokens;
    query.remove_prefix(start);

    for (int i = 0; i < 2; ++i) {
        auto space_pos = query.find(' ');
        if (space_pos == std::string_view::npos) {
            if (!query.empty()) 
                tokens.emplace_back(query);
            return tokens;
        }
        
        tokens.emplace_back(query.substr(0, space_pos));
        
        auto next_word = query.find_first_not_of(' ', space_pos);
        if (next_word == std::string_view::npos) return tokens;
        query.remove_prefix(next_word);
    }

    if (!query.empty())
        tokens.emplace_back(query);
    
    return tokens;
}

std::string CommandProcessor::to_upper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), toupper);
    return str;
}

std::string CommandProcessor::execute(std::string_view query) {
    std::vector<std::string_view> tokens = get_tokens(query);
    if (tokens.empty()) return "-ERR no command provided\n";

    auto is_cmd = [](std::string_view token, std::string_view cmd) {
        return std::ranges::equal(token, cmd, [](char a,char b) {
            return toupper(a) == toupper(b);
        });
    };

    if (is_cmd(tokens[0], "SET")) {
        if (tokens.size() < 3) return "-ERR missing args, must be SET [key] [value]\r\n";
        storage_.set(std::string(tokens[1]), std::string(tokens[2]));
        if (save_aof_) {
            aof_manager_->save(query);
            if (config::DEFAULT_VALUE_TTL != -1) {
                int64_t expires_at = storage_.get_current_time_ms() + static_cast<int64_t>(config::DEFAULT_VALUE_TTL) * 1000;
                aof_manager_->save(std::format("EXPIRESAT {} {}", tokens[1], expires_at));
            }
        }
        return "+OK\r\n";
    }

    if (is_cmd(tokens[0], "GET")) {
        if (tokens.size() < 2) return "-ERR missing args, must be GET [key]\r\n";
        std::optional<std::string> response = storage_.get(std::string(tokens[1]));
        return response == std::nullopt ? "$-1\r\n" : "+" + response.value() + "\r\n";
    }

    if (is_cmd(tokens[0], "DEL")) {
        if (tokens.size() < 2) return "-ERR missing args, must be DEL [key]\r\n";
        if (storage_.del(std::string(tokens[1]))) { 
            if (save_aof_) aof_manager_->save(query);
            return "+OK\r\n";
        }
        return "-ERR key not found\r\n";
    }

    if (is_cmd(tokens[0], "EXISTS")) {
        if (tokens.size() < 2) return "-ERR missing args, must be EXISTS [key]\r\n";
        return storage_.exists(std::string(tokens[1])) ? ":1\r\n" : ":0\r\n";
    }

    if (is_cmd(tokens[0], "HELP")) {
        return "+Supported commands:\nHELP - return this message\nSET [key] [value...] - set value for key\nGET [key] - returns value for [key] if found, -1 otherwise\nDEL [key] - erase value for [key]\nEXISTS [key] - returns 1 if [key] exists, 0 otherwise\nEXPIRES [key] [ttl] - set value's ttl for [key] (in seconds)\r\n";
    }

    if (is_cmd(tokens[0], "EXPIRES")) {
        if (tokens.size() < 3) return "-ERR missing args, must be EXPIRES [key] [ttl]\r\n";
        try {
            int32_t seconds = stoi(std::string(tokens[2]));
            if (storage_.set_expires(std::string(tokens[1]), seconds)) {
                if (save_aof_) {
                    int64_t expires_at = storage_.get_current_time_ms() + static_cast<int64_t>(seconds) * 1000;
                    aof_manager_->save(std::format("EXPIRESAT {} {}", tokens[1], expires_at));
                }
                return "+OK\r\n";
            }
        } catch (...) {
            return "-ERR invalid ttl\r\n";
        }
        return "-ERR key not found\r\n";
    }

    if (is_cmd(tokens[0], "EXPIRESAT")) {
        if (tokens.size() < 3) return "-ERR missing args, must be EXPIRES [key] [ttl]\r\n";
        try {
            int64_t timestamp = stoll(std::string(tokens[2]));
            if (storage_.set_expiresat(std::string(tokens[1]), timestamp)) {
                if (save_aof_) aof_manager_->save(query);
                return "+OK\r\n";
            }
        } catch (...) {
            return "-ERR invalid ttl\r\n";
        }
        return "-ERR key not found\r\n";
    }

    return "-ERR unkown command. type HELP for list of commands\r\n";
}
