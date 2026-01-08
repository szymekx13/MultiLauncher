@echo off
echo Building MultiLauncher...

g++ ^
 src\main.cpp ^
 src\App.cpp ^
 src\Gui.cpp ^
 include\external\imgui\imgui.cpp ^
 include\external\imgui\imgui_draw.cpp ^
 include\external\imgui\imgui_tables.cpp ^
 include\external\imgui\imgui_widgets.cpp ^
 include\external\imgui\imgui_impl_win32.cpp ^
 include\external\imgui\imgui_impl_dx11.cpp ^
 -Iinclude ^
 -Iinclude\external ^
 -Iinclude\external\imgui ^
 -ld3d11 ^
 -ldxgi ^
 -ldwmapi ^
 -ld3dcompiler_47 ^
 -lgdi32 ^
 -luser32 ^
 -lole32 ^
 -static-libgcc ^
 -static-libstdc++ ^
 -std=c++23 ^
 -O2 ^
 -o MultiLauncher.exe

if %errorlevel% neq 0 (
    echo Build failed
    exit /b %errorlevel%
)

echo Build OK
