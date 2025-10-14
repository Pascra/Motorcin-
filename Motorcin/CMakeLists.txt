cmake_minimum_required(VERSION 3.10)

project(Motorcin)

find_package(fmt CONFIG REQUIRED)

add_executable(Motorcin src/Main.cpp)

target_link_libraries(Motorcin PRIVATE fmt::fmt)