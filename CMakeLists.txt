cmake_minimum_required(VERSION 3.15)
project(PulseFS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCES
    src/main.cpp
    src/Core/MftScanner.cpp
    src/Core/UsnMonitor.cpp
    src/Engine/SearchIndex.cpp
)

add_executable(PulseFS ${SOURCES})

target_include_directories(PulseFS PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(WIN32)
    target_link_libraries(PulseFS PRIVATE advapi32)
endif()

if(MSVC)
    target_compile_options(PulseFS PRIVATE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(PulseFS PRIVATE -Wall -Wextra)
endif()
