#include "../include/MultiLauncher/Gui.hpp"
#include "../include/external/imgui/imgui_impl_dx11.h"
#include "../include/external/imgui/imgui_impl_win32.h"

namespace MultiLauncher{
    void Gui::init() {
    // tu można np. zainicjalizować ImGui kontekst
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
}

void Gui::shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void Gui::render(GameManager& manager) {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("MultiLauncher");
    for(auto& game : manager.getGames()){
        ImGui::Text("%s (%s)", game.getName().c_str(), game.getLauncher().c_str());
        ImGui::SameLine();
        std::string btnLabel = "Launch##" + game.getName();
        if(ImGui::Button(btnLabel.c_str())){
            game.launch();
        }
    }
    ImGui::End();

    ImGui::Render();
}
}