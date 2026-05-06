#pragma once

#include <string>

namespace config {
    const int PORT = 4242;
    const int THREADS_NUM = 8;
    const std::string SAVEFILE_NAME = "aof_save";

    const int EPOLL_TIMEOUT = 100;
    const int BUFFER_SIZE = 4096;
    const int IDLE_CLIENT_TTL = 60; // TTL for idle client's (in seconds)

    const int DEFAULT_VALUE_TTL = -1; // in seconds, -1 for unlimited
    const int ACTIVE_CLEAN_PER_LOOP = 20; // How many values tries to clean each timeout
}