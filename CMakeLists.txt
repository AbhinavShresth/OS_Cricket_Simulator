cmake_minimum_required(VERSION 3.20)
project(T20_Cricket_Simulator VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(cricket_sim ${SOURCES})

target_include_directories(cricket_sim PRIVATE include/)

target_link_libraries(cricket_sim PRIVATE Threads::Threads)

target_compile_options(cricket_sim PRIVATE -Wall -Wextra)