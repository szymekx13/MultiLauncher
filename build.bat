@echo off
setlocal EnableDelayedExpansion

echo Building MultiLauncher...

set "start=%time: =0%"

g++ ^
 src\main.cpp ^
 src\App.cpp ^
 src\Gui.cpp ^
 src\Game.cpp ^
 include\external\imgui\imgui.cpp ^
 include\external\imgui\imgui_draw.cpp ^
 include\external\imgui\imgui_tables.cpp ^
 include\external\imgui\imgui_widgets.cpp ^
 include\external\imgui\imgui_impl_win32.cpp ^
 include\external\imgui\imgui_impl_dx11.cpp ^
 -Iinclude ^
 -Iinclude\external ^
 -Iinclude\external\imgui ^
 -ld3d11 -ldxgi -ldwmapi -ld3dcompiler -lgdi32 -luser32 -lole32 -lurlmon ^
 -static-libgcc -static-libstdc++ ^
 -std=c++23 -O2 -o MultiLauncher.exe

if %errorlevel% neq 0 (
    echo.
    echo Build failed!
    pause
    exit /b %errorlevel%
)

echo Build OK

set "end=%time: =0%"

set /a "start_ms=1%start:~0,2%*360000 + 1%start:~3,2%*6000 + 1%start:~6,2%*100 + 1%start:~9,2%"
set /a "end_ms=1%end:~0,2%*360000   + 1%end:~3,2%*6000   + 1%end:~6,2%*100   + 1%end:~9,2%"

set /a "diff=end_ms - start_ms"

set /a "hh = diff / 360000"
set /a "mm = (diff %% 360000) / 6000"
set /a "ss = (diff %% 6000)  / 100"
set /a "cc = diff %% 100"

echo.
echo Start:    %start%
echo End:      %end%
echo Elapsed:  %hh%:%mm:~-2%:%ss:~-2%,%cc:~-2%