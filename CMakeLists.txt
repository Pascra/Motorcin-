cmake_minimum_required(VERSION 3.22)
project(Motorcin CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(DEFINED ENV{VCPKG_ROOT})
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# STB_IMAGE (header-only)
# Descarga stb_image.h y colócalo en: src/external/stb_image.h

add_executable(Motorcin
  src/Main.cpp
  src/core/Application.cpp
  src/core/Window.cpp
  src/core/Renderer.cpp
  src/core/Shader.cpp
  src/core/Model.cpp
  src/core/Input.cpp
  src/core/Time.cpp
  src/core/Camera.cpp
  src/core/Texture.cpp
)

target_include_directories(Motorcin PRIVATE 
  src
  src/external
)

if (MSVC)
  target_compile_options(Motorcin PRIVATE /MP)
  target_compile_definitions(Motorcin PRIVATE NOMINMAX)
endif()

target_compile_definitions(Motorcin PRIVATE SDL_MAIN_HANDLED)

target_link_libraries(Motorcin
  PRIVATE
    SDL3::SDL3
    glad::glad
    assimp::assimp
)

if (WIN32)
  add_custom_command(TARGET Motorcin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:SDL3::SDL3>
      $<TARGET_FILE_DIR:Motorcin>)
  add_custom_command(TARGET Motorcin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
      $<TARGET_FILE:assimp::assimp>
      $<TARGET_FILE_DIR:Motorcin>)
endif()