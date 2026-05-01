#pragma once

#include <unordered_map>
#include <string>
#include <cstdint>

struct Value{
    std::string value;
    int64_t expires_at = -1;
};

class Storage {
private:
    std::unordered_map<std::string, Value> storage_;
    std::unordered_map<std::string, Value>::iterator cleanup_it;

    int64_t get_current_time_ms() const;
    bool is_expired(const Value& value, int64_t now);

public:
    Storage() = default;

    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool set_expires(const std::string& key, const int32_t seconds);

    void active_clean(int32_t max_clean_per_loop);
};