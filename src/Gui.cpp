#include "../include/MultiLauncher/Gui.hpp"
#include "../include/external/imgui/imgui_impl_dx11.h"
#include "../include/external/imgui/imgui_impl_win32.h"
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
    ImGui::StyleColorsDark();

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
    // Build UI only. Frame creation/rendering handled by main loop.
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
}

} // namespace MultiLauncher