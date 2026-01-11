#include "../include/MultiLauncher/Game.hpp"
#include "../include/external/stb_image.h"
#include <fstream>
#include <string>

#ifdef _WIN32
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#endif

namespace MultiLauncher {

    Game::Game(const std::string& n, const LauncherType launch, const std::filesystem::path& p, int appid) 
        : name(n), launcher(launch), path(p), gameState(STOPPED), steamAppId(appid), bannerLoaded(false), banner{nullptr, 0, 0}
    {
    }

    Game::~Game() {
        if(banner.srv) {
            banner.srv->Release();
            banner.srv = nullptr;
        }
    }

    const void Game::launch() {
        gameState = STARTING;
        #ifdef _WIN32
            SHELLEXECUTEINFOA shExInfo = {0};
            shExInfo.cbSize = sizeof(shExInfo);
            shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            shExInfo.hwnd = NULL;
            shExInfo.lpVerb = "open";
            shExInfo.lpFile = path.string().c_str();
            shExInfo.lpParameters = NULL;
            shExInfo.lpDirectory = NULL;
            shExInfo.nShow = SW_SHOWNORMAL;
            shExInfo.hInstApp = NULL;

            if (ShellExecuteExA(&shExInfo))
            {
                status = GameStatus::Running;
                WaitForSingleObject(shExInfo.hProcess, INFINITE);
                CloseHandle(shExInfo.hProcess);
            }
            else
            {
                throw std::runtime_error("Something went wrong");
            }
        #else
            // Linux/Unix impl
        #endif
    }

    bool Game::LoadTextureFromFile(ID3D11Device* device, const wchar_t* filename, BannerTexture& out_banner) const {
        if (!device) return false;

        // Convert wstring to string for stbi (supports utf8 but let's just use string)
        char str[1024];
        WideCharToMultiByte(CP_UTF8, 0, filename, -1, str, 1024, NULL, NULL);

        int image_width = 0;
        int image_height = 0;
        int channels = 0;
        unsigned char* image_data = stbi_load(str, &image_width, &image_height, &channels, 4);
        if (image_data == NULL) return false;

        // Create texture
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
        device->CreateTexture2D(&desc, &subResource, &pTexture);

        if(pTexture == NULL) {
            stbi_image_free(image_data);
            return false;
        }

        // Create texture view
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
        ZeroMemory(&srvDesc, sizeof(srvDesc));
        srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = desc.MipLevels;
        srvDesc.Texture2D.MostDetailedMip = 0;
        device->CreateShaderResourceView(pTexture, &srvDesc, &out_banner.srv);
        pTexture->Release();

        out_banner.width = image_width;
        out_banner.height = image_height;
        stbi_image_free(image_data);

        return true;
    }

    bool Game::loadBanner(ID3D11Device* device) const {
        if (bannerLoaded || steamAppId <= 0)
            return bannerLoaded;

        // Create cleanup directory if needed
         if (!std::filesystem::exists("assets/cache")) {
            std::filesystem::create_directories("assets/cache");
        }

        std::wstring url = L"https://cdn.cloudflare.steamstatic.com/steam/apps/" + std::to_wstring(steamAppId) + L"/library_hero.jpg";
        std::wstring local = L"assets/cache/" + std::to_wstring(steamAppId) + L"_hero.jpg";

        if (!std::filesystem::exists(local)) {
            HRESULT hr = URLDownloadToFileW(nullptr, url.c_str(), local.c_str(), 0, nullptr);
            if (FAILED(hr)) {
                return false;
            }
        }

        if(LoadTextureFromFile(device, local.c_str(), banner)){
            bannerLoaded = true;
        }
        
        return bannerLoaded;
    }

}
