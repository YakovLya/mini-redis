#include "processor.hpp"

#include <algorithm>
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
        return "error: no command provided\n";

    std::string command = to_upper(tokens[0]);

    if (command == "SET") {
        if (tokens.size() < 3) return "error: missing args, must be SET [key] [value]\n";
        storage_.set(tokens[1], tokens[2]);
        return "OK\n";
    }

    if (command == "GET") {
        if (tokens.size() < 2) return "error: missing args, must be GET [key]\n";
        return storage_.get(tokens[1]);
    }

    if (command == "DEL") {
        if (tokens.size() < 2) return "error: missing args, must be DEL [key]\n";
        if (storage_.del(tokens[1])) return "OK\n";
        return "error: key not found\n";
    }

    if (command == "EXISTS") {
        if (tokens.size() < 2) return "error: missing args, must be EXISTS [key]\n";
        return storage_.exists(tokens[1]) ? "1\n" : "0\n";
    }

    if (command == "HELP") {
        return "Supported commands:\nHELP - return this message\nSET [key] [value...] - set value for key\nGET [key] - returns value for [key] if found, -1 otherwise\nDEL [key] - erase value for [key]\nEXISTS [key] - returns 1 if [key] exists, 0 otherwise\nEXPIRES [key] [ttl] - set value's ttl for [key] (in seconds)\n";
    }

    if (command == "EXPIRES") {
        if (tokens.size() < 3) return "error: missing args, must be EXPIRES [key] [ttl]\n";
        try {
            int32_t seconds = stoi(tokens[2]);
            if (storage_.set_expires(tokens[1], seconds)) return "OK\n";
        } catch (...) {
            return "error: invalid ttl";
        }
        return "error: key not found\n";
    }

    return "error: unkown command. type HELP for list of commands\n";
}