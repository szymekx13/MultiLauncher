#include "../include/MultiLauncher/App.hpp"
#include "../include/external/imgui/imgui.h" // added: ensure ImGui flags available

int main() {
    MultiLauncher::App app;
    app.run();
    return 0;
}