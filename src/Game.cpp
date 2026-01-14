#include "../include/MultiLauncher/Game.hpp"
#include "../include/external/stb_image.h"
#include <fstream>
#include <string>
#include <algorithm>
#include <fstream>
#include <string>
#include <curl/curl.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include "../include/MultiLauncher/PlaytimeManager.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")
#endif

namespace MultiLauncher {

    Game::Game(const std::string& n, const LauncherType launch, const std::filesystem::path& p, const std::string& exeName, int appid) 
        : name(n), launcher(launch), path(p), executableName(exeName), gameState(STOPPED), steamAppId(appid), bannerLoaded(false), banner{nullptr, 0, 0}
    {
        if (executableName.empty()) {
            // Default executable name guessing if not provided
            executableName = name;
            executableName.erase(std::remove_if(executableName.begin(), executableName.end(), 
                [](unsigned char c){ return std::isspace(c); }), executableName.end());
            executableName += ".exe";
        }
    }

    Game::~Game() {
        if(banner.srv) {
            banner.srv->Release();
            banner.srv = nullptr;
        }
    }

    void Game::launchAsync() {
        if(status.load() != GameStatus::Idle) return;

        status.store(GameStatus::Launching);
        
        std::thread([this](){
            try {
                auto start = std::chrono::steady_clock::now();
                
                launch(); // Blocks until the game (or its process) is considered closed
                
                auto end = std::chrono::steady_clock::now();
                auto elapsedMinutes = std::chrono::duration_cast<std::chrono::minutes>(end - start).count();

                // Track playtime manually for non-Steam games
                if (steamAppId <= 0) {
                     PlaytimeManager::instance().addPlaytime(name, (int)elapsedMinutes);
                }

                status.store(GameStatus::Idle);
                gameState = STOPPED;
            } catch (const std::exception& e) {
                Logger::instance().error("Exception during game launch: " + std::string(e.what()));
                status.store(GameStatus::Idle);
                gameState = STOPPED;
            } catch (...) {
                Logger::instance().error("Unknown error during game launch");
                status.store(GameStatus::Idle);
                gameState = STOPPED;
            }
        }).detach();
    }

    const void Game::launch() {
        gameState = STARTING;
        
        #ifdef _WIN32
        if (launcher == STEAM) {
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

            Logger::instance().info("Launching Steam game: " + name);

            if (ShellExecuteExA(&shExInfo))
            {
                // We set Launching status; the background poller will pick up "Running" once the process is found
                status = GameStatus::Launching;
                
                if (shExInfo.hProcess != NULL) {
                    WaitForSingleObject(shExInfo.hProcess, 5000); 
                    CloseHandle(shExInfo.hProcess);
                }
            }
            else
            {
                Logger::instance().error("ShellExecute failed for Steam game: " + name);
                status = GameStatus::Idle;
            }
            return;
        }

        // Non-Steam games (Executable path) -> CreateProcess with Pipe Capture
        SECURITY_ATTRIBUTES saAttr; 
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
        saAttr.bInheritHandle = TRUE; 
        saAttr.lpSecurityDescriptor = NULL; 

        HANDLE hChildStd_OUT_Rd = NULL;
        HANDLE hChildStd_OUT_Wr = NULL;

        if (!CreatePipe(&hChildStd_OUT_Rd, &hChildStd_OUT_Wr, &saAttr, 0)) {
            Logger::instance().error("Stdout Rd CreatePipe failed"); 
            return;
        }

        if (!SetHandleInformation(hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0)) {
            Logger::instance().error("Stdout SetHandleInformation failed"); 
            return;
        }

        STARTUPINFOA si = {0};
        si.cb = sizeof(STARTUPINFOA);
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = hChildStd_OUT_Wr;
        si.hStdError = hChildStd_OUT_Wr;
        
        PROCESS_INFORMATION pi = {0};
        std::string exePath = path.string();
        std::string workDir = path.parent_path().string();
        std::string cmdLine = "\"" + exePath + "\"";

        if (CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE, 0, NULL, workDir.c_str(), &si, &pi)) 
        {
            status = GameStatus::Launching;
            
            CloseHandle(hChildStd_OUT_Wr);
            
            // Still read output to log it, but don't block status on it infinitely
            CHAR chBuf[4096]; 
            DWORD dwRead;
            while (true) {
                BOOL bSuccess = ReadFile(hChildStd_OUT_Rd, chBuf, 4095, &dwRead, NULL);
                if (!bSuccess || dwRead == 0) break; 

                chBuf[dwRead] = 0;
                Logger::instance().info("[" + name + "] " + std::string(chBuf));
            }

            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hChildStd_OUT_Rd);
        }
        else
        {
            Logger::instance().error("CreateProcess failed (" + std::to_string(GetLastError()) + ") for: " + name);
            CloseHandle(hChildStd_OUT_Wr);
            CloseHandle(hChildStd_OUT_Rd);
        }
        #else
            // Linux/Unix impl
        #endif
    }

    bool Game::LoadTextureFromFile(ID3D11Device* device, const wchar_t* filename, BannerTexture& out_banner) const {
        if (!device) return false;

        // Convert wstring to string for stbi
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

    // Helper: generate sanitised key for banner lookup
    // "Cyberpunk 2077" -> "cyberpunk2077"
    static std::string makeBannerKey(const std::string& name) {
        std::string s = name;
        std::transform(s.begin(), s.end(), s.begin(), 
            [](unsigned char c){ return std::tolower(c); });
        
        s.erase(std::remove_if(s.begin(), s.end(), 
            [](unsigned char c){ return !std::isalnum(c); }), s.end());
        return s;
    }

    bool Game::loadBanner(ID3D11Device* device) const {
        if (bannerLoaded) return true;

        // Try Steam first if AppID is valid
        if (steamAppId > 0) {
             if (!std::filesystem::exists("assets/cache")) {
                std::filesystem::create_directories("assets/cache");
            }

            std::wstring url = L"https://cdn.cloudflare.steamstatic.com/steam/apps/" + std::to_wstring(steamAppId) + L"/library_hero.jpg";
            std::wstring local = L"assets/cache/" + std::to_wstring(steamAppId) + L"_hero.jpg";

            if (!std::filesystem::exists(local)) {
                HRESULT hr = URLDownloadToFileW(nullptr, url.c_str(), local.c_str(), 0, nullptr);
                // If download fails, we fall through to local check? Or strict steam only?
                // Let's assume if download fails, we might still have a manual override file.
            }

            if(LoadTextureFromFile(device, local.c_str(), banner)){
                bannerLoaded = true;
                return true;
            }
        }

        // Fallback or Non-Steam: Check Local "assets/banners/{key}.jpg/png"
        std::string key = makeBannerKey(name);
        
        // Convert to wide string for generic load function if needed, 
        // but LoadTextureFromFile takes wchar_t* and expects a path.
        // Let's rely on relative paths being convertible or use filesystem.
        
        std::vector<std::string> exts = { ".jpg", ".png" };
        for(const auto& ext : exts) {
            std::string pathStr = "assets/banners/" + key + ext;
            // Convert to wstring for our helper
            std::wstring wpath(pathStr.begin(), pathStr.end());
            
            if(std::filesystem::exists(pathStr)) {
                 if(LoadTextureFromFile(device, wpath.c_str(), banner)){
                    bannerLoaded = true;
                    return true;
                 }
            }
        }

        return bannerLoaded;
    }

    void Game::updateStatus() {
        bool running = isProcessRunning(executableName);
        if (running) {
            status = GameStatus::Running;
            gameState = RUNNING;
        } else {
            // Only set to Idle if we aren't currently "Launching" (to avoid race conditions)
            if (status != GameStatus::Launching) {
                status = GameStatus::Idle;
                gameState = STOPPED;
            }
        }
    }

    #include <tlhelp32.h>
    #include <algorithm>
    bool Game::isProcessRunning(const std::string& processName) const {
        #ifdef _WIN32
        bool found = false;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(hSnapshot, &pe32)) {
                // Convert search name to lower case
                std::string searchName = processName;
                std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);

                do {
                    std::string currentProcess = pe32.szExeFile;
                    std::transform(currentProcess.begin(), currentProcess.end(), currentProcess.begin(), ::tolower);

                    if (searchName == currentProcess) {
                        found = true;
                        break;
                    }
                } while (Process32Next(hSnapshot, &pe32));
            }
            CloseHandle(hSnapshot);
        }
        return found;
        #else
        return false;
        #endif
    }

}
