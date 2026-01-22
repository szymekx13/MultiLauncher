#include "../include/MultiLauncher/Gui.hpp"
#include "../include/MultiLauncher/Logger.hpp"
#include "../include/MultiLauncher/EpicProvider.hpp"
#include "../include/external/imgui/imgui_impl_dx11.h"
#include "../include/external/imgui/imgui_impl_win32.h"
#include "../include/external/imgui/imgui.h"
#include "../include/external/imgui/imgui_internal.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../include/external/stb_image.h"
#include "../include/MultiLauncher/PlaytimeManager.hpp"
#include <cfloat>
#include <thread>
#include <algorithm> 
#ifdef _WIN32
    #include <windows.h>
    #include <d3d11.h>
    #include <tchar.h>
    #pragma comment(lib, "d3d11.lib")
#endif

    // WndProc helper
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    namespace MultiLauncher{

static Gui* g_gui_instance = nullptr;

// global font pointers for render()
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

bool Gui::LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    int channels = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, &channels, 4);
    if (image_data == NULL)
        return false;

    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = image_width;
    desc.Height = image_height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;

    ID3D11Texture2D* pTexture = NULL;
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = image_data;
    subResource.SysMemPitch = desc.Width * 4;
    subResource.SysMemSlicePitch = 0;
    pd3dDevice_->CreateTexture2D(&desc, &subResource, &pTexture);

    if(pTexture == NULL) {
        stbi_image_free(image_data);
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory(&srvDesc, sizeof(srvDesc));
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    pd3dDevice_->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
    pTexture->Release();

    *out_width = image_width;
    *out_height = image_height;
    stbi_image_free(image_data);

    return true;
}

void Gui::init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* rtv) {
#ifdef _WIN32
    hwnd_ = hwnd;
    pd3dDevice_ = device;
    pd3dDeviceContext_ = deviceContext;
    mainRenderTargetView_ = rtv;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    // Enable Keyboard Controls and Docking
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_DockingEnable;

    ImFont* f1 = io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Regular.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (f1) g_mainFont = f1;
    ImFont* f2 = io.Fonts->AddFontFromFileTTF("assets/fonts/Inter_18pt-Bold.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
    if (f2) g_mainBold = f2;
    if (g_mainFont) io.FontDefault = g_mainFont;

    // "Moonlight Grey" Theme
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.12f, 0.14f, 1.00f); // #1e1e24 Deep Grey
    colors[ImGuiCol_ChildBg]                = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_PopupBg]                = ImVec4(0.12f, 0.12f, 0.14f, 0.94f);
    colors[ImGuiCol_Border]                 = ImVec4(0.28f, 0.28f, 0.30f, 0.50f);
    colors[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg]                = ImVec4(0.18f, 0.18f, 0.21f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.24f, 0.24f, 0.28f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.28f, 0.28f, 0.32f, 1.00f);
    colors[ImGuiCol_TitleBg]                = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive]          = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.12f, 0.12f, 0.14f, 1.00f);
    colors[ImGuiCol_MenuBarBg]              = ImVec4(0.15f, 0.15f, 0.18f, 1.00f);
    colors[ImGuiCol_ScrollbarBg]            = ImVec4(0.12f, 0.12f, 0.14f, 0.53f);
    colors[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
    colors[ImGuiCol_CheckMark]              = ImVec4(0.30f, 0.65f, 1.00f, 1.00f); // Ocean Blue
    colors[ImGuiCol_SliderGrab]             = ImVec4(0.30f, 0.65f, 1.00f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]       = ImVec4(0.50f, 0.80f, 1.00f, 1.00f);
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.55f, 0.90f, 0.70f); // Soft Ocean Blue
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.30f, 0.65f, 1.00f, 1.00f); // Bright Ocean Blue
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.15f, 0.50f, 0.85f, 1.00f);
    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.55f, 0.90f, 0.40f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.30f, 0.65f, 1.00f, 0.60f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.20f, 0.55f, 0.90f, 0.80f);
    colors[ImGuiCol_Separator]              = ImVec4(0.28f, 0.28f, 0.30f, 0.50f);
    colors[ImGuiCol_SeparatorHovered]       = ImVec4(0.20f, 0.55f, 0.90f, 0.78f);
    colors[ImGuiCol_SeparatorActive]        = ImVec4(0.20f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_ResizeGrip]             = ImVec4(0.20f, 0.55f, 0.90f, 0.25f);
    colors[ImGuiCol_ResizeGripHovered]      = ImVec4(0.30f, 0.65f, 1.00f, 0.67f);
    colors[ImGuiCol_ResizeGripActive]       = ImVec4(0.30f, 0.65f, 1.00f, 0.95f);
    colors[ImGuiCol_Tab]                    = ImVec4(0.12f, 0.12f, 0.14f, 0.86f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.30f, 0.65f, 1.00f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.28f, 0.28f, 0.30f, 1.00f);
    colors[ImGuiCol_TabUnfocused]           = ImVec4(0.10f, 0.10f, 0.12f, 0.97f);
    colors[ImGuiCol_TabUnfocusedActive]     = ImVec4(0.18f, 0.18f, 0.21f, 1.00f);
    colors[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    colors[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    colors[ImGuiCol_TextSelectedBg]         = ImVec4(0.20f, 0.55f, 0.90f, 0.35f);
    colors[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
    colors[ImGuiCol_NavHighlight]           = ImVec4(0.20f, 0.55f, 0.90f, 1.00f);
    colors[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);

    // Rounding and Padding
    style.WindowRounding    = 12.0f;
    style.ChildRounding     = 6.0f;
    style.FrameRounding     = 6.0f;
    style.PopupRounding     = 6.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding      = 12.0f;
    style.TabRounding       = 6.0f;

    style.WindowPadding     = ImVec2(12, 12);
    style.FramePadding      = ImVec2(10, 6);
    style.ItemSpacing       = ImVec2(8, 8);
    style.ItemInnerSpacing  = ImVec2(6, 6);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 14.0f;
    style.GrabMinSize       = 10.0f;

    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;

    ImGui_ImplWin32_Init(hwnd_);
    ImGui_ImplDX11_Init(pd3dDevice_, pd3dDeviceContext_);

    // set global instance for WndProc usage
    g_gui_instance = this;

    int w, h;
    LoadTextureFromFile("assets/images/steam_logo.png", &m_iconSteam, &w, &h);
    LoadTextureFromFile("assets/images/epic_logo.png", &m_iconEpic, &w, &h);
    LoadTextureFromFile("assets/images/gog_logo.png", &m_iconGog, &w, &h);

#endif
}

void Gui::shutdown() {
#ifdef _WIN32
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    mainRenderTargetView_ = nullptr;
    pSwapChain_ = nullptr;
    pSwapChain_ = nullptr;
    pd3dDeviceContext_ = nullptr;
    pd3dDevice_ = nullptr;

    if(m_iconSteam) { m_iconSteam->Release(); m_iconSteam = nullptr; }
    if(m_iconEpic) { m_iconEpic->Release(); m_iconEpic = nullptr; }
    if(m_iconGog) { m_iconGog->Release(); m_iconGog = nullptr; }
    for(auto& pair : m_textureCache) {
        if(pair.second) pair.second->Release();
    }
    m_textureCache.clear();

    hwnd_ = NULL;
    UnregisterClass(_T("MultiLauncherWndClass"), GetModuleHandle(NULL));
    g_gui_instance = nullptr;
    
    #endif
}

void Gui::render(GameManager& manager) {
    auto lock = manager.lockGames();
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID); // docking-branch: set viewport

    ImGuiWindowFlags host_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                  ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                                  ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

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

        // Split: left 40% for Games, bottom 25% for Logs, rest for Details
        ImGuiID dock_left = 0, dock_bottom = 0;
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Left, 0.40f, &dock_left, &dock_main_id);
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.25f, &dock_bottom, &dock_main_id);

        ImGui::DockBuilderDockWindow("Games", dock_left);
        ImGui::DockBuilderDockWindow("Details", dock_main_id);
        ImGui::DockBuilderDockWindow("Logs", dock_bottom);

        ImGui::DockBuilderFinish(dockspace_id);
        dock_layout_initialized = true;
    }

    static std::string selected_game_name;

    ImGui::PushFont(g_mainBold);
    ImGui::Begin("Games");
    ImGui::PopFont();

    if (EpicProvider::isAvailable()) {
        if (ImGui::Button("Connect Epic Account", ImVec2(ImGui::GetContentRegionAvail().x, 30))) {
            EpicProvider::authenticate();
        }
    } else {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Epic support requires legendary");
    }
    ImGui::Spacing();


    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    static int sort_mode = 0;
    const char* sort_items[] = { "Sort by Name", "Sort by Launcher" };
    if(ImGui::BeginCombo("##sort", sort_items[sort_mode])) {
        for(int n=0; n<IM_ARRAYSIZE(sort_items); n++) {
            bool is_selected = (sort_mode == n);
            if(ImGui::Selectable(sort_items[n], is_selected))
                sort_mode = n;
            if(is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }


    static char game_filter[128] = "";
    ImGui::InputTextWithHint("##game_filter", "Search games...", game_filter, sizeof(game_filter));

    static int launcher_filter = 0;
    const int LF_STEAM = 1, LF_EPIC = 2, LF_GOG = 4;

    auto IconButton = [](ID3D11ShaderResourceView* icon, const char* label, bool active, const ImVec4& color, int& flags, int flag_bit) {
        if(active) ImGui::PushStyleColor(ImGuiCol_Button, color);
        if(icon) {
            if(ImGui::ImageButton(label, (ImTextureID)icon, ImVec2(20,20))) { // slightly smaller
                flags ^= flag_bit;
            }
        } else {
             // Fallback
             if(ImGui::Button(label)) flags ^= flag_bit;
        }
        if(active) ImGui::PopStyleColor();
        if(ImGui::IsItemHovered()) ImGui::SetTooltip("Filter: %s", label);
    };

    ImGui::TextDisabled("Filters:");
    ImGui::SameLine();
    bool steam_active = (launcher_filter & LF_STEAM);
    IconButton(m_iconSteam, "Steam", steam_active, ImVec4(0.10f,0.45f,0.85f,1.0f), launcher_filter, LF_STEAM);
    
    ImGui::SameLine();
    bool epic_active = (launcher_filter & LF_EPIC);
    IconButton(m_iconEpic, "Epic", epic_active, ImVec4(0.55f,0.10f,0.85f,1.0f), launcher_filter, LF_EPIC);

    ImGui::SameLine();
    bool gog_active = (launcher_filter & LF_GOG);
    IconButton(m_iconGog, "GOG", gog_active, ImVec4(0.05f,0.75f,0.35f,1.0f), launcher_filter, LF_GOG);

    ImGui::Separator();


    if (ImGui::BeginTable("GamesTable", 2,
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::PushFont(g_mainBold);
        ImGui::TableSetupColumn("Game", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableHeadersRow();
        ImGui::PopFont();

        // Filtering
        std::vector<Game*> displayList;
        auto& games = manager.getGames();
        for(auto& uptr : games) {
            Game* game = uptr.get();
             // text filter
            if (game_filter[0] &&
                game->getName().find(game_filter) == std::string::npos)
                continue;

            // launcher filter
            if (launcher_filter)
            {
                const std::string& L = game->getLauncher();
                bool match = false;
                if ((launcher_filter & LF_STEAM) && L.find("Steam") != std::string::npos) match = true;
                if ((launcher_filter & LF_EPIC) && L.find("Epic") != std::string::npos) match = true;
                if ((launcher_filter & LF_GOG) && (L.find("GOG") != std::string::npos || L.find("gog") != std::string::npos)) match = true;
                if (!match) continue;
            }
            displayList.push_back(game);
        }

        // Sort
        if(sort_mode == 0) { // Name
            std::sort(displayList.begin(), displayList.end(), [](Game* a, Game* b){
                return a->getName() < b->getName();
            });
        } else if (sort_mode == 1) { // Launcher
            std::sort(displayList.begin(), displayList.end(), [](Game* a, Game* b){
                if(a->getLauncher() != b->getLauncher())
                    return a->getLauncher() < b->getLauncher();
                return a->getName() < b->getName(); // secondary sort by name
            });
        }

        // Render games
        for (size_t i = 0; i < displayList.size(); ++i)
        {
            Game* game = displayList[i];
            
            // Use pointer as ID for stability
            ImGui::PushID(game);

            auto status = game->status.load();

            const char* label = "Launch";
            if(status == Game::GameStatus::Launching) label = "Launching...";
            else if(status == Game::GameStatus::Running) label = "Running";

            bool disabled = (status != Game::GameStatus::Idle);

            ImGui::TableNextRow(ImGuiTableRowFlags_None, 36.0f);


            ImGui::TableSetColumnIndex(0);
            bool selected = (selected_game_name == game->getName());

            ImGui::PushItemFlag(ImGuiItemFlags_AllowOverlap, true);
            if (ImGui::Selectable(
                    ("##sel"), // Label hidden
                    selected,
                    ImGuiSelectableFlags_SpanAllColumns,
                    ImVec2(0, 48))) 
            {
                selected_game_name = game->getName();
            }
            ImGui::PopItemFlag();

            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)){
                game->launchAsync();
            }

            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::TextUnformatted(game->getName().c_str());
            ImGui::TextDisabled("%s", game->getLauncher().c_str());
            
            if (status == Game::GameStatus::Downloading || status == Game::GameStatus::Installing) {
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.3f, 0.65f, 1.0f, 1.0f));
                ImGui::ProgressBar(game->getProgress(), ImVec2(ImGui::GetContentRegionAvail().x * 0.8f, 4), "");
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::TextDisabled("%.0f%%", game->getProgress() * 100.0f);
            }
            ImGui::EndGroup();


            ImGui::TableSetColumnIndex(1);

            if (status == Game::GameStatus::Downloading || status == Game::GameStatus::Installing) {
                if (ImGui::Button("Cancel", ImVec2(-FLT_MIN, 28))) {
                    ProcessRunner::runAsync("tools/legendary/legendary.exe cancel", [](int){});
                    game->status = Game::GameStatus::Idle;
                }
            } else if (status == Game::GameStatus::Error) {
                if (ImGui::Button("Resume", ImVec2(-FLT_MIN, 28))) {
                    EpicProvider::installGame(*game);
                }
            } else {
                if(disabled){
                    ImGui::BeginDisabled();
                }
                if (ImGui::Button(label, ImVec2(-FLT_MIN, 28)))
                {
                    Logger::instance().info("Launching " + game->getName());
                    if (game->getLauncher().find("Epic") != std::string::npos) {
                        EpicProvider::launchGame(game->getName());
                    } else {
                        game->launchAsync();
                    }
                }
                if(disabled) ImGui::EndDisabled();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::End();

    // Details Panel
    ImGui::PushFont(g_mainBold);
    ImGui::Begin("Details");
    ImGui::PopFont();
    if (!selected_game_name.empty()) {
        bool found = false;
        for (auto& g : manager.getGames()) {
            if (g->getName() == selected_game_name) {
                if (g->loadBanner(pd3dDevice_)) {
                    const auto& banner = g->getBanner();
                    if(banner.srv) {
                        float availW = ImGui::GetContentRegionAvail().x;
                        float aspect = (float)banner.width / (float)banner.height;
                        if(aspect == 0) aspect = 460.0f / 215.0f; // safety
                        float h = availW / aspect;

                        ImGui::Image((ImTextureID)banner.srv, ImVec2(availW, h));


                        ImVec2 p0 = ImGui::GetItemRectMin();
                        ImVec2 p1 = ImGui::GetItemRectMax();
                        ImDrawList* dl = ImGui::GetWindowDrawList();

                        // Dark overlay at bottom
                        dl->AddRectFilledMultiColor(
                            ImVec2(p0.x, p1.y - 80),
                            p1,
                            IM_COL32(0,0,0,0),
                            IM_COL32(0,0,0,0),
                            IM_COL32(0,0,0,200),
                            IM_COL32(0,0,0,200)
                        );
                        
                        if(g_mainBold) {
                            dl->AddText(
                                g_mainBold,
                                28.0f,
                                ImVec2(p0.x + 20, p1.y - 40),
                                IM_COL32(255,255,255,255),
                                g->getName().c_str()
                            );
                        }
                    }
                } else {
                     ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.12f, 1.0f));
                     if(ImGui::BeginChild("BannerFallback", ImVec2(0, 150), true)) {
                        // Center text
                         auto windowWidth = ImGui::GetWindowSize().x;
                         auto textWidth   = ImGui::CalcTextSize("No Banner Available").x;

                         ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
                         ImGui::SetCursorPosY(ImGui::GetWindowSize().y * 0.5f - 10.0f);
                         ImGui::TextDisabled("No banner available");
                     }
                     ImGui::EndChild();
                     ImGui::PopStyleColor();
                }

                ImGui::Separator();
                ImGui::Text("Name: %s", g->getName().c_str());
                ImGui::Text("Launcher: %s", g->getLauncher().c_str());
                
                float hours = PlaytimeManager::instance().getHours(g->getName(), g->getSteamAppId());
                if (hours > 0.0f) {
                    ImGui::Text("Playtime: %.1f h", hours);
                } else {
                    ImGui::Text("Playtime: --");
                }


                const char* btnLabel = "Launch";
                auto status = g->status.load();
                if(status == Game::GameStatus::Launching) btnLabel = "Launching...";
                else if(status == Game::GameStatus::Running) btnLabel = "Running";
                
                bool disabled = (status != Game::GameStatus::Idle);
                
                if (status == Game::GameStatus::Downloading || status == Game::GameStatus::Installing) {
                    ImGui::Text("Status: %s", status == Game::GameStatus::Downloading ? "Downloading" : "Installing");
                    ImGui::ProgressBar(g->getProgress(), ImVec2(-FLT_MIN, 20));
                    ImGui::Text("Speed: %s", g->getETA().c_str());
                    if (ImGui::Button("Cancel Installation", ImVec2(140, 36))) {
                        ProcessRunner::runAsync("tools/legendary/legendary.exe cancel", [](int){});
                        g->status = Game::GameStatus::Idle;
                    }
                } else {
                    if(disabled) ImGui::BeginDisabled();
                    if (ImGui::Button(btnLabel, ImVec2(120, 36))) {
                        OutputDebugStringA(("MultiLauncher: launching from Details: " + g->getName() + "\n").c_str());
                        if (g->getLauncher().find("Epic") != std::string::npos) {
                            EpicProvider::launchGame(g->getName());
                        } else {
                            g->launchAsync();
                        }
                    }
                    if(disabled) ImGui::EndDisabled();

                    if (g->getLauncher().find("Epic") != std::string::npos && status == Game::GameStatus::Idle) {
                        ImGui::SameLine();
                        if (ImGui::Button("Install/Repair", ImVec2(120, 36))) {
                            EpicProvider::installGame(*g);
                        }
                    }
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

    // Logs Panel
    ImGui::Begin("Logs");

    if (ImGui::Button("Clear")) Logger::instance().clear();
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &Logger::instance().autoscroll);

    ImGui::Separator();

    ImGui::BeginChild("LogScroll");

    for (auto& l : Logger::instance().getLogs()) {
        if (l.lvl == MultiLauncher::Logger::LogLevel::Error)
            ImGui::TextColored(ImVec4(1,0.3f,0.3f,1), "[ERROR] %s", l.s.c_str());
        else
            ImGui::TextUnformatted(l.s.c_str());
    }

    if (Logger::instance().autoscroll)
        ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
    ImGui::End();


    ImGui::End(); // End MultiLauncherRoot
}

} // namespace MultiLauncher