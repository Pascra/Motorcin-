cmake_minimum_required(VERSION 3.22)
project(Motorcin CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)

file(GLOB_RECURSE SRC_FILES src/*.cpp)

add_executable(Motorcin ${SRC_FILES})
target_include_directories(Motorcin PRIVATE src)
target_link_libraries(Motorcin PRIVATE SDL3::SDL3 glad::glad)
