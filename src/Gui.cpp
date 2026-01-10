#include "../include/MultiLauncher/Gui.hpp"
#include "../include/MultiLauncher/Logger.hpp"
#include "../include/external/imgui/imgui_impl_dx11.h"
#include "../include/external/imgui/imgui_impl_win32.h"
#include "../include/external/imgui/imgui.h"
#include "../include/external/imgui/imgui_internal.h" // added: DockBuilder declarations
#include <cfloat> // added for FLT_MIN
#include <thread> // added: run launches off UI thread
#ifdef _WIN32
    #include <windows.h>
    #include <d3d11.h>
    #include <tchar.h>
    #pragma comment(lib, "d3d11.lib")
#endif

    // Forward-declare ImGui WndProc handler in global namespace (header leaves it in #if 0)
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    namespace MultiLauncher{

// We'll keep a pointer accessible in WndProc for simple handling
static Gui* g_gui_instance = nullptr;

// add global font pointers so render() can use them
static ImFont* g_mainFont = nullptr;
static ImFont* g_mainBold = nullptr;

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (::ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg)
    {
    case WM_SIZE:
        if (g_gui_instance && g_gui_instance->getDeviceContext()) {
            // handled by swapchain resizing in more complete implementations
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}



void Gui::init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* rtv) {
#ifdef _WIN32
    // Store provided D3D objects and HWND; App owns device/swapchain
    hwnd_ = hwnd;
    pd3dDevice_ = device;
    pd3dDeviceContext_ = deviceContext;
    mainRenderTargetView_ = rtv;

    // ImGui context and backend init (do not create device/swapchain here)
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // Enable Keyboard Controls and Docking (Viewports temporarily disabled to avoid missing UpdatePlatformWindows)
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

    // Try to load custom fonts but don't abort if missing.
    ImFont* f1 = io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Regular.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (f1) g_mainFont = f1;
    ImFont* f2 = io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Bold.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (f2) g_mainBold = f2;
    // Only override default if we actually loaded a font
    if (g_mainFont) io.FontDefault = g_mainFont;

    ImGui::StyleColorsDark();

    // Small style tweaks (big visual impact)
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.FramePadding = ImVec2(8,4);
    style.ItemSpacing = ImVec2(6,6);
    style.WindowPadding = ImVec2(10,10);

    // Optional: tweak a couple colors
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Header] = ImVec4(0.12f, 0.52f, 0.90f, 0.45f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.12f, 0.52f, 0.90f, 0.65f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.55f, 0.95f, 0.40f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.15f, 0.55f, 0.95f, 0.75f);

    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(pd3dDevice_, pd3dDeviceContext_);

    // set global instance for WndProc usage
    g_gui_instance = this;
#else
    // ...existing code for non-Windows if any...
#endif
}

void Gui::shutdown() {
#ifdef _WIN32
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // Note: App owns the D3D device/swapchain and window; do not Release() them here
    mainRenderTargetView_ = nullptr;
    pSwapChain_ = nullptr;
    pd3dDeviceContext_ = nullptr;
    pd3dDevice_ = nullptr;

    hwnd_ = NULL;
    UnregisterClass(_T("MultiLauncherWndClass"), GetModuleHandle(NULL));
    g_gui_instance = nullptr;
#else
    // ...existing code...
#endif
}

void Gui::render(GameManager& manager) {
    // Root DockSpace / Host window
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID); // docking-branch: set viewport

    ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // Make background of host window transparent to allow docking passthru center
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("MultiLauncherRoot", nullptr, host_flags);
    ImGui::PopStyleVar(2);

    // Create DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MultiLauncherDockspace");
    ImGui::DockSpace(dockspace_id, ImVec2(0,0), ImGuiDockNodeFlags_PassthruCentralNode);

    // Setup default layout only once
    static bool dock_layout_initialized = false;
    if (!dock_layout_initialized) {
        // Build a simple default layout:
        ImGui::DockBuilderRemoveNode(dockspace_id); // clear any existing layout
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);
        ImGuiID dock_main_id = dockspace_id;

        // Split: left 25% for Games, bottom 25% for Logs, rest for Details
        ImGuiID dock_left = 0, dock_bottom = 0;
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.25f, &dock_left, &dock_main_id);
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_main_id);

        ImGui::DockBuilderDockWindow("Games", dock_left);
        ImGui::DockBuilderDockWindow("Details", dock_main_id);
        ImGui::DockBuilderDockWindow("Logs", dock_bottom);

        ImGui::DockBuilderFinish(dockspace_id);
        dock_layout_initialized = true;
    }

    // selected game visible for all panels in this frame
    static std::string selected_game_name;

    // Panels: Games
    ImGui::Begin("Games");

    static char game_filter[128] = "";
    ImGui::InputTextWithHint("##game_filter", "Filter games...", game_filter, sizeof(game_filter));

    // launcher filter bitmask
    static int launcher_filter = 0;
    const int LF_STEAM = 1, LF_EPIC = 2, LF_GOG = 4;

    ImGui::SameLine();
    ImGui::Text("Launcher:");
    ImGui::SameLine();

    // Steam
    bool active = (launcher_filter & LF_STEAM);
    if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f,0.45f,0.85f,1.0f));
    if (ImGui::SmallButton("S")) launcher_filter ^= LF_STEAM;
    if (active) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Steam");

    ImGui::SameLine();
    // Epic
    active = (launcher_filter & LF_EPIC);
    if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f,0.10f,0.85f,1.0f));
    if (ImGui::SmallButton("E")) launcher_filter ^= LF_EPIC;
    if (active) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Epic");

    ImGui::SameLine();
    // GOG
    active = (launcher_filter & LF_GOG);
    if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.05f,0.75f,0.35f,1.0f));
    if (ImGui::SmallButton("G")) launcher_filter ^= LF_GOG;
    if (active) ImGui::PopStyleColor();
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("GOG");

    ImGui::Separator();

    // ===== TABLE =====
    if (ImGui::BeginTable("GamesTable", 2,
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("Game", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableHeadersRow();

        auto& games = manager.getGames();
        for (size_t i = 0; i < games.size(); ++i)
        {
            auto& game = games[i];

            // text filter
            if (game_filter[0] &&
                game.getName().find(game_filter) == std::string::npos)
                continue;

            // launcher filter
            if (launcher_filter)
            {
                const std::string& L = game.getLauncher();
                bool match = false;
                if ((launcher_filter & LF_STEAM) && L.find("Steam") != std::string::npos) match = true;
                if ((launcher_filter & LF_EPIC) && L.find("Epic") != std::string::npos) match = true;
                if ((launcher_filter & LF_GOG) && (L.find("GOG") != std::string::npos || L.find("gog") != std::string::npos)) match = true;
                if (!match) continue;
            }

            ImGui::TableNextRow(ImGuiTableRowFlags_None, 36.0f);

            // ===== COLUMN 0: GAME =====
            ImGui::TableSetColumnIndex(0);
            bool selected = (selected_game_name == game.getName());

            ImGui::PushItemFlag(ImGuiItemFlags_AllowOverlap, true);
            if (ImGui::Selectable(
                    ("##sel" + std::to_string(i)).c_str(),
                    selected,
                    ImGuiSelectableFlags_SpanAllColumns,
                    ImVec2(0, 36)))
            {
                selected_game_name = game.getName();
            }
            ImGui::PopItemFlag();

            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
                game.launch();
            }

            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::TextUnformatted(game.getName().c_str());
            ImGui::TextDisabled("%s", game.getLauncher().c_str());
            ImGui::EndGroup();

            // ===== COLUMN 1: ACTION =====
            ImGui::TableSetColumnIndex(1);
            ImGui::PushID((int)i);

            if (ImGui::Button("Launch", ImVec2(-FLT_MIN, 28)))
            {
                Logger::instance().info("Launching " + game.getName());
                Game copy = game;
                std::thread([copy]() mutable {
                    try { copy.launch(); }
                    catch (...) {}
                }).detach();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::End();



    // Panels: Details (shows details for selected game)
    ImGui::Begin("Details");
    if (!selected_game_name.empty()) {
        bool found = false;
        // iterate non-const so we can call launch()
        for (auto& g : manager.getGames()) {
            if (g.getName() == selected_game_name) {
                ImGui::Text("Name: %s", g.getName().c_str());
                ImGui::Text("Launcher: %s", g.getLauncher().c_str());
                ImGui::Separator();
                if (ImGui::Button("Launch")) {
                    OutputDebugStringA(("MultiLauncher: launching from Details: " + g.getName() + "\n").c_str());
                    Game gameCopy = g;
                    std::thread([gameCopy]() mutable {
                        try {
                            gameCopy.launch();
                        } catch (...) {
                            OutputDebugStringA("MultiLauncher: exception in details launch thread\n");
                        }
                    }).detach();
                }
                found = true;
                break;
            }
        }
        if (!found) {
            ImGui::TextUnformatted("Selected game not found.");
        }
    } else {
        ImGui::TextUnformatted("Select a game to see details here.");
    }
    ImGui::End();

    // Panels: Logs (dockable)
    ImGui::Begin("Logs");
    if (ImGui::Button("Clear")) {
        Logger::instance().clear();
    }
    ImGui::Separator();
    ImGui::BeginChild("LogRegion", ImVec2(0,0), false, ImGuiWindowFlags_HorizontalScrollbar);
    auto logs = Logger::instance().getLogs();
    for (const auto& line : logs) {
        ImGui::TextUnformatted(line.c_str());
    }
    if (!logs.empty()) ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    ImGui::End();

    ImGui::End(); // End MultiLauncherRoot
}

} // namespace MultiLauncher