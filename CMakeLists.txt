cmake_minimum_required(VERSION 3.16)
project(mini-redis VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-fsanitize=thread -g -O1)
    add_link_options(-fsanitize=thread)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-O3)
endif()

find_package(Threads REQUIRED)

include_directories(src)

set(SOURCES
    src/main.cpp
    src/server/server.cpp
    src/storage/storage.cpp
    src/commands/processor.cpp
)

add_executable(mredis ${SOURCES})
target_link_libraries(mredis PRIVATE Threads::Threads)