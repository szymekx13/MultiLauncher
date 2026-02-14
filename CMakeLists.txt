cmake_minimum_required(VERSION 3.16)
project(MultiLauncher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Source files
set(SOURCES
    src/main.cpp
    src/App.cpp
    src/Gui.cpp
    src/Game.cpp
    include/external/imgui/imgui.cpp
    include/external/imgui/imgui_draw.cpp
    include/external/imgui/imgui_tables.cpp
    include/external/imgui/imgui_widgets.cpp
)

find_package(CURL REQUIRED)

# Platforms
if(WIN32)
    list(APPEND SOURCES
        include/external/imgui/imgui_impl_win32.cpp
        include/external/imgui/imgui_impl_dx11.cpp
    )
else()
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.8
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)

    list(APPEND SOURCES
        include/external/imgui/imgui_impl_glfw.cpp
        include/external/imgui/imgui_impl_opengl3.cpp
    )
endif()

# Executable
add_executable(MultiLauncher ${SOURCES})

# Copy assets and tools to build directory
add_custom_command(TARGET MultiLauncher POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/tools
        ${CMAKE_BINARY_DIR}/tools
)

# Include directories
target_include_directories(MultiLauncher PRIVATE
    include
    include/external
    include/external/imgui
)

# Libraries
target_link_libraries(MultiLauncher PRIVATE CURL::libcurl)

if(WIN32)
    target_link_libraries(MultiLauncher PRIVATE
        d3d11 dxgi dwmapi d3dcompiler gdi32 user32 ole32 urlmon shell32
        -static-libgcc -static-libstdc++
    )
else()
    target_link_libraries(MultiLauncher PRIVATE
        glfw
        GL dl pthread
    )
endif()

# Compiler options
if(MSVC)
    target_compile_options(MultiLauncher PRIVATE /O2)
else()
    target_compile_options(MultiLauncher PRIVATE -O2)
endif()
