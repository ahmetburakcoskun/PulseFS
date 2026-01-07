cmake_minimum_required(VERSION 3.15)
project(PulseFS VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui
  GIT_TAG        docking
)
FetchContent_MakeAvailable(imgui)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)

set(SOURCES
    src/main.cpp
    src/Core/MftScanner.cpp
    src/Core/UsnMonitor.cpp
    src/Engine/SearchIndex.cpp
    src/ImGui/ImGuiManager.cpp
    src/ImGui/ImGuiTheme.cpp
    src/Platform/Win32Window.cpp
    src/Renderer/D3D11Renderer.cpp
    src/UI/IconCache.cpp
    src/UI/MainWindow.cpp
    src/UI/SearchPanel.cpp
    ${IMGUI_SOURCES}
)

add_executable(PulseFS WIN32 ${SOURCES})

target_include_directories(PulseFS PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

if(WIN32)
    target_link_libraries(PulseFS PRIVATE advapi32 d3d11 d3dcompiler dwmapi)
endif()

if(MSVC)
    target_compile_options(PulseFS PRIVATE /W4 /permissive- /Zc:__cplusplus /DUNICODE /D_UNICODE)
else()
    target_compile_options(PulseFS PRIVATE -Wall -Wextra)
endif()
