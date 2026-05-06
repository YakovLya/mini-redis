#pragma once

#include <string_view>

namespace config {
    inline constexpr int PORT = 4242;
    inline constexpr int THREADS_NUM = 8;
    inline constexpr std::string_view SAVEFILE_NAME = "aof_save";

    inline constexpr int EPOLL_TIMEOUT = 100;
    inline constexpr int BUFFER_SIZE = 4096;
    inline constexpr int IDLE_CLIENT_TTL = 60; // TTL for idle client's (in seconds)

    inline constexpr int DEFAULT_VALUE_TTL = -1; // in seconds, -1 for unlimited
    inline constexpr int ACTIVE_CLEAN_PER_LOOP = 20; // How many values tries to clean each timeout
}