cmake_minimum_required(VERSION 3.16)
project(mini-redis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src)

set(SOURCES
    src/main.cpp
    src/storage/storage.cpp
    src/commands/processor.cpp
)

add_executable(mredis ${SOURCES})