#include "../include/MultiLauncher/App.hpp"
#include "../include/external/imgui/imgui.h" // added: ensure ImGui flags available

int main() {
    MultiLauncher::App app;
    static_assert(ImGuiConfigFlags_DockingEnable != 0, "Docking NOT enabled");
    app.run();
    return 0;
}