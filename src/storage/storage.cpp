#include "storage.hpp"
#include "config.hpp"
#include <chrono>
#include <optional>

int64_t Storage::get_current_time_ms() const {
    return std::chrono::duration_cast<std::chrono::milliseconds> \
            (std::chrono::steady_clock::now().time_since_epoch()).count();
}

bool Storage::is_expired(const Value& value, int64_t now) {
    return (value.expires_at != -1 && value.expires_at <= now);
}

void Storage::set(const std::string& key, const std::string& value) {
    storage_[key] = {value, -1};
    if (config::DEFAULT_VALUE_TTL != -1)
        set_expires(key, config::DEFAULT_VALUE_TTL);
    cleanup_it = storage_.begin();
}

std::optional<std::string> Storage::get(const std::string& key) {
    if (storage_.count(key)) {
        int64_t ms = get_current_time_ms();
        if (is_expired(storage_[key], ms)) {
            storage_.erase(key);
            return std::nullopt;
        }
                
        return storage_[key].value;
    }
    return std::nullopt;
}

bool Storage::del(const std::string& key) {
    return storage_.erase(key) > 0;
}

bool Storage::exists(const std::string& key) {
    if (storage_.count(key)) {
        int64_t ms = get_current_time_ms();
        if (is_expired(storage_[key], ms)) {
            storage_.erase(key);
            return false;
        }

        return true;
    }
    return false;
}

bool Storage::set_expires(const std::string& key, const int32_t seconds) {
    if (storage_.count(key)) {
        storage_[key].expires_at = get_current_time_ms() + static_cast<int64_t>(seconds) * 1000;
        return true;
    }
    return false;
}

void Storage::active_clean(int32_t max_clean_per_loop) {
    if (!storage_.empty()) {
        int checked = 0;
        int64_t ms = get_current_time_ms();

        if (cleanup_it == storage_.end())
            cleanup_it = storage_.begin();
        
        while (cleanup_it != storage_.end() && checked < max_clean_per_loop) {
            if (cleanup_it->second.expires_at != -1 && cleanup_it->second.expires_at <= ms) {
                cleanup_it = storage_.erase(cleanup_it);
            } else {
                ++ cleanup_it;
            }
            ++ checked;
        }
    }
}
