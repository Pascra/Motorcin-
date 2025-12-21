cmake_minimum_required(VERSION 3.20)
project(MotorcinEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encuentra los paquetes necesarios
find_package(SDL3 CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)

# Añade los directorios de inclusión
include_directories(${CMAKE_SOURCE_DIR}/src)

# Recopila archivos fuente
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/**/*.cpp"
)

file(GLOB_RECURSE HEADERS 
    "src/*.h"
    "src/**/*.h"
)

# Debug: muestra qué archivos encontró (opcional)
message(STATUS "Archivos fuente encontrados: ${SOURCES}")

# Crea el ejecutable (¡CORREGIDO! era ${SOURCE_FILES})
add_executable(MotorcinEngine ${SOURCES} ${HEADERS})

# Enlaza las bibliotecas
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