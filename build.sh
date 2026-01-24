#!/usr/bin/env bash

echo "Building MultiLauncher..."

start_time=$(date +%s.%N)

g++ \
    src/main.cpp \
    src/App.cpp \
    src/Gui.cpp \
    src/Game.cpp \
    include/external/imgui/imgui.cpp \
    include/external/imgui/imgui_draw.cpp \
    include/external/imgui/imgui_tables.cpp \
    include/external/imgui/imgui_widgets.cpp \
    include/external/imgui/imgui_impl_glfw.cpp \
    include/external/imgui/imgui_impl_opengl3.cpp \
    -Iinclude \
    -Iinclude/external \
    -Iinclude/external/imgui \
    -lglfw -lGL -ldl -lpthread -lcurl \
    -static-libgcc -static-libstdc++ \
    -std=c++23 -O2 -o MultiLauncher

if [ $? -ne 0 ]; then
    echo ""
    echo "Build failed!"
#    read -n 1 -s -r -p "Press any key to continue..."
    exit 1
fi

echo "Build OK"

end_time=$(date +%s.%N)

diff=$(echo "$end_time - $start_time" | bc)

hh=$(printf "%.0f" "$(echo "$diff / 3600" | bc)")
mm=$(printf "%.0f" "$(echo "($diff % 3600) / 60" | bc)")
ss=$(printf "%.0f" "$(echo "$diff % 60" | bc)")
cc=$(printf "%02.0f" "$(echo "($diff - ($hh*3600) - ($mm*60) - $ss) * 100" | bc)")

hh=$(printf "%02d" "$hh")
mm=$(printf "%02d" "$mm")
ss=$(printf "%02d" "$ss")

echo ""
echo "Start:    $(date -d "@${start_time%.*}" +%H:%M:%S),${start_time##*.:0}"
echo "End:      $(date -d "@${end_time%.*}" +%H:%M:%S),${end_time##*.:0}"
echo "Elapsed:  $hh:$mm:$ss,$cc"

# read -n 1 -s -r -p "Press any key to continue..."