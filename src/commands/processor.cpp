#include "processor.hpp"

#include <algorithm>
#include <optional>
#include <sstream>

CommandProcessor::CommandProcessor(Storage& storage) : storage_(storage) {}

std::vector<std::string> CommandProcessor::get_tokens(const std::string& query) {
    std::vector<std::string> tokens;
    std::stringstream ss(query);
    std::string word;

    if (ss >> word) tokens.push_back(word);
    if (ss >> word) tokens.push_back(word);

    std::string rest;
    std::getline(ss, rest);
    size_t start_pos = rest.find_first_not_of(" ");
    if (start_pos != std::string::npos)
        tokens.push_back(rest.substr(start_pos));
    
    return tokens;
}

std::string CommandProcessor::to_upper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), toupper);
    return str;
}

std::string CommandProcessor::execute(const std::string& query) {
    std::vector<std::string> tokens = get_tokens(query);
    if (tokens.empty())
        return "-ERR no command provided\n";

    std::string command = to_upper(tokens[0]);

    if (command == "SET") {
        if (tokens.size() < 3) return "-ERR missing args, must be SET [key] [value]\r\n";
        storage_.set(tokens[1], tokens[2]);
        return "+OK\r\n";
    }

    if (command == "GET") {
        if (tokens.size() < 2) return "-ERR missing args, must be GET [key]\r\n";
        std::optional<std::string> response = storage_.get(tokens[1]);
        return response == std::nullopt ? "$-1\r\n" : "+" + response.value() + "\r\n";
    }

    if (command == "DEL") {
        if (tokens.size() < 2) return "-ERR missing args, must be DEL [key]\r\n";
        if (storage_.del(tokens[1])) return "+OK\r\n";
        return "-ERR key not found\r\n";
    }

    if (command == "EXISTS") {
        if (tokens.size() < 2) return "-ERR missing args, must be EXISTS [key]\r\n";
        return storage_.exists(tokens[1]) ? ":1\r\n" : ":0\r\n";
    }

    if (command == "HELP") {
        return "+Supported commands:\nHELP - return this message\nSET [key] [value...] - set value for key\nGET [key] - returns value for [key] if found, -1 otherwise\nDEL [key] - erase value for [key]\nEXISTS [key] - returns 1 if [key] exists, 0 otherwise\nEXPIRES [key] [ttl] - set value's ttl for [key] (in seconds)\r\n";
    }

    if (command == "EXPIRES") {
        if (tokens.size() < 3) return "-ERR missing args, must be EXPIRES [key] [ttl]\r\n";
        try {
            int32_t seconds = stoi(tokens[2]);
            if (storage_.set_expires(tokens[1], seconds)) return "+OK\r\n";
        } catch (...) {
            return "-ERR invalid ttl\r\n";
        }
        return "-ERR key not found\r\n";
    }

    return "-ERR unkown command. type HELP for list of commands\r\n";
}