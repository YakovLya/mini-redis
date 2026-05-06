#include "storage.hpp"
#include "config.hpp"
#include <string>
#include <utils/logger.hpp>

#include <chrono>
#include <mutex>
#include <optional>
#include <shared_mutex>

int64_t Storage::get_current_time_ms() const {
    return std::chrono::floor<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool Storage::is_expired(const Value& value, int64_t now) {
    return (value.expires_at != -1 && value.expires_at <= now);
}

void Storage::set_expires_internal(const std::string& key, const int64_t timestamp) {
    if (storage_.count(key)) storage_[key].expires_at = timestamp;
}

void Storage::set(const std::string& key, const std::string& value) {
    std::unique_lock lock(rw_mutex_);
    storage_[key] = {value, -1};
    if (config::DEFAULT_VALUE_TTL != -1)
        set_expires_internal(key, get_current_time_ms() + static_cast<int64_t>(config::DEFAULT_VALUE_TTL) * 1000);
    cleanup_it = storage_.begin();
}

std::optional<std::string> Storage::get(const std::string& key) { 
    {
        std::shared_lock lock(rw_mutex_);
        if (storage_.count(key)) {
            int64_t ms = get_current_time_ms();
            if (!is_expired(storage_[key], ms)) return storage_[key].value;
        } else return std::nullopt;
    }

    del(key);
    return std::nullopt;
}

bool Storage::del(const std::string& key) {
    std::unique_lock lock(rw_mutex_);
    return storage_.erase(key) > 0;
}

bool Storage::exists(const std::string& key) {
    {
        std::shared_lock lock(rw_mutex_);
        if (storage_.count(key)) {
            int64_t ms = get_current_time_ms();
            if (!is_expired(storage_[key], ms)) return true;
        } else return false;
    }

    del(key);
    return false;
}

bool Storage::set_expires(const std::string& key, const int32_t seconds) {
    std::unique_lock lock(rw_mutex_);
    if (storage_.count(key)) {
        set_expires_internal(key, get_current_time_ms() + static_cast<int64_t>(seconds) * 1000);
        return true;
    }
    return false;
}

bool Storage::set_expiresat(const std::string& key, const int64_t timestamp) {
    std::unique_lock lock(rw_mutex_);
    if (storage_.count(key)) {
        set_expires_internal(key, timestamp);
        return true;
    }
    return false;
}

void Storage::active_clean(int32_t max_clean_per_loop) {
    std::unique_lock lock(rw_mutex_);
    if (!storage_.empty()) {
        int checked = 0;
        int64_t ms = get_current_time_ms();

        if (cleanup_it == storage_.end())
            cleanup_it = storage_.begin();
        
        while (cleanup_it != storage_.end() && checked < max_clean_per_loop) {
            if (cleanup_it->second.expires_at != -1 && cleanup_it->second.expires_at <= ms) {
                Logger::log(LogLevel::INFO, "clean expired key " + cleanup_it->first);
                cleanup_it = storage_.erase(cleanup_it);
            } else {
                ++ cleanup_it;
            }
            ++ checked;
        }
    }
}
