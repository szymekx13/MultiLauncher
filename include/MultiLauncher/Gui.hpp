#pragma once
#include "GameManager.hpp"
#include "../external/imgui/imgui.h"
#ifdef _WIN32
    #include <windows.h>
    #include <d3d11.h>
#endif

#include <memory>
#include <unordered_map>

namespace MultiLauncher{

    class Gui{
        public:
#ifdef _WIN32
            void init(HWND hwnd, ID3D11Device* device, ID3D11DeviceContext* deviceContext, ID3D11RenderTargetView* rtv);
#else
            void init(void* window);
#endif
            void render(GameManager& manager);
            void shutdown();

#ifdef _WIN32
            HWND getHwnd() const { return hwnd_; }
            ID3D11Device* getDevice() const { return pd3dDevice_; }
            ID3D11DeviceContext* getDeviceContext() const { return pd3dDeviceContext_; }
#endif

#ifdef _WIN32
            bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height);
#else
            bool LoadTextureFromFile(const char* filename, void** out_srv, int* out_width, int* out_height);
#endif

        private:
#ifdef _WIN32
            HWND hwnd_ = NULL;
            IDXGISwapChain* pSwapChain_ = nullptr;
            ID3D11Device* pd3dDevice_ = nullptr;
            ID3D11DeviceContext* pd3dDeviceContext_ = nullptr;
            ID3D11RenderTargetView* mainRenderTargetView_ = nullptr;

            // Texture resources
#endif
            
            // Texture resources
#ifdef _WIN32
            ID3D11ShaderResourceView* m_iconSteam = nullptr;
            ID3D11ShaderResourceView* m_iconEpic = nullptr;
            ID3D11ShaderResourceView* m_iconGog = nullptr;
            std::unordered_map<std::string, ID3D11ShaderResourceView*> m_textureCache;
#else
            void* m_iconSteam = nullptr;
            void* m_iconEpic = nullptr;
            void* m_iconGog = nullptr;
            std::unordered_map<std::string, void*> m_textureCache;
#endif
    };

}