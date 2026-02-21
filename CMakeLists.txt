cmake_minimum_required(VERSION 3.16)
project(MultiLauncher LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# --- Pliki źródłowe ---
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

if(NOT WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    find_package(CURL REQUIRED)
endif()

# Znajdowanie Pythona
find_package(Python 3.11 REQUIRED COMPONENTS Interpreter Development)

# Pobieranie pybind11
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG        v3.0.2
)
FetchContent_MakeAvailable(pybind11)

# Obsługa platform (Windows vs Linux)
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

# --- Cel główny (Executable) ---
add_executable(MultiLauncher ${SOURCES})

# Include directories
target_include_directories(MultiLauncher PRIVATE
    include
    include/external
    include/external/imgui
    ${Python_INCLUDE_DIRS}
)

# Linkowanie bibliotek
target_link_libraries(MultiLauncher PRIVATE 
    $<$<NOT:$<PLATFORM_ID:Windows>>:CURL::libcurl>
    pybind11::embed  
)

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

# Opcje kompilatora
if(MSVC)
    target_compile_options(MultiLauncher PRIVATE /O2)
else()
    target_compile_options(MultiLauncher PRIVATE -O2)
endif()

# --- SEKCJA DYSTRYBUCJI (INSTALL) ---

# Wymuszamy instalację do folderu 'dist' wewnątrz folderu build
set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/dist")

# 1. Instalacja pliku wykonywalnego
install(TARGETS MultiLauncher RUNTIME DESTINATION ".")

# 2. Instalacja Assets
install(DIRECTORY "${CMAKE_SOURCE_DIR}/assets/" DESTINATION "assets")

# 3. Instalacja wybranych narzędzi
install(PROGRAMS "${CMAKE_SOURCE_DIR}/tools/legendary/legendary.exe" DESTINATION "tools/legendary")
install(PROGRAMS "${CMAKE_SOURCE_DIR}/tools/EpicBanner/EpicBanner.py" DESTINATION "tools/EpicBanner")

if(WIN32)
    # Wyciągamy ścieżkę do folderu, w którym znajduje się python.exe
    get_filename_component(PYTHON_BIN_DIR "${Python_EXECUTABLE}" DIRECTORY)

    # 4. Instalacja zależności pythonowych do podfolderu w buildzie
    set(PIP_DEPS_DIR "${CMAKE_BINARY_DIR}/dist/site-packages")
    add_custom_command(
        OUTPUT "${PIP_DEPS_DIR}/_completed"
        COMMAND "${Python_EXECUTABLE}" -m pip install -r "${CMAKE_SOURCE_DIR}/tools/EpicBanner/requirements.txt" --target "${PIP_DEPS_DIR}"
        DEPENDS "${CMAKE_SOURCE_DIR}/tools/EpicBanner/requirements.txt"
        COMMENT "Installing Python dependencies from requirements.txt"
    )
    add_custom_target(PipInstall DEPENDS "${PIP_DEPS_DIR}/_completed")
    add_dependencies(MultiLauncher PipInstall) # Zapewniamy, że zależności są instalowane przed budowaniem

    # Instalujemy zainstalowane pakiety
    install(DIRECTORY "${PIP_DEPS_DIR}/" DESTINATION "site-packages")

    # 5. Kopiowanie DLL Pythona prosto do folderu z EXE
    if(Python_RUNTIME_LIBRARIES)
        install(FILES ${Python_RUNTIME_LIBRARIES} DESTINATION "." OPTIONAL)
    else()
        get_filename_component(PYTHON_DLL_PATH "${Python_EXECUTABLE}" DIRECTORY)
        install(FILES "${PYTHON_DLL_PATH}/python${Python_VERSION_MAJOR}${Python_VERSION_MINOR}.dll" DESTINATION "." OPTIONAL) # Fallback
    endif()

    # 6. Kopiowanie bibliotek standardowych
    if(EXISTS "${PYTHON_BIN_DIR}/Lib")
        install(DIRECTORY "${PYTHON_BIN_DIR}/Lib/" DESTINATION "Lib" 
                PATTERN "site-packages" EXCLUDE 
                PATTERN "__pycache__" EXCLUDE)
    elseif(EXISTS "${PYTHON_BIN_DIR}/../Lib")
        install(DIRECTORY "${PYTHON_BIN_DIR}/../Lib/" DESTINATION "Lib" 
                PATTERN "site-packages" EXCLUDE 
                PATTERN "__pycache__" EXCLUDE)
    endif()
endif()