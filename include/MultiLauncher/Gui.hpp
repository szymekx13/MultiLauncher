#pragma once
#include "GameManager.hpp"
#include "../external/imgui/imgui.h"
#ifdef _WIN32
    #include <windows.h>
    #include <d3d11.h>
#endif

namespace MultiLauncher{
#include <memory>

    class Gui{
        public:
            // Initialize GUI with an existing Win32 HWND and D3D11 device/context/RTV
            void init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* rtv);
            void render(GameManager& manager);
            void shutdown();

            // getters needed by callers (kept for compatibility)
            HWND getHwnd() const { return hwnd_; }
            ID3D11Device* getDevice() const { return pd3dDevice_; }
            ID3D11DeviceContext* getDeviceContext() const { return pd3dDeviceContext_; }

        private:
#ifdef _WIN32
            HWND hwnd_ = NULL;
            IDXGISwapChain* pSwapChain_ = nullptr; // kept for compatibility but not owned when passed in
            ID3D11Device* pd3dDevice_ = nullptr;
            ID3D11DeviceContext* pd3dDeviceContext_ = nullptr;
            ID3D11RenderTargetView* mainRenderTargetView_ = nullptr;
#endif
    };
}