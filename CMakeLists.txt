cmake_minimum_required(VERSION 3.20)
project(MotorcinEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encuentra los paquetes necesarios
find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# Define todos los archivos fuente (.cpp y .h)
set(SOURCE_FILES
    src/Core/Main.cpp
    src/Core/Application.cpp
    src/Core/Application.h
    src/Core/Window.cpp
    src/Core/Window.h
    src/Core/Time.cpp
    src/Core/Time.h
    src/Core/Input.cpp
    src/Core/Input.h
    
    src/Rendering/Renderer.cpp
    src/Rendering/Renderer.h
    src/Rendering/Shader.cpp
    src/Rendering/Shader.h
    src/Rendering/Texture.cpp
    src/Rendering/Texture.h
    src/Rendering/Model.cpp
    src/Rendering/Model.h
    src/Rendering/Camera.cpp
    src/Rendering/Camera.h
)

add_executable(MotorcinEngine ${SOURCE_FILES})

target_link_libraries(MotorcinEngine PRIVATE 
    SDL3::SDL3
    glad::glad
    assimp::assimp
)

# Copiar assets al directorio de build
add_custom_command(TARGET MotorcinEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:MotorcinEngine>/assets
)