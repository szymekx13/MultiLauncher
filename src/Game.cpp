#include "../include/MultiLauncher/Game.hpp"
#include "../include/external/stb_image.h"
#ifdef __linux__
#include <GL/gl.h>
#endif
#include <fstream>
#include <string>
#include <algorithm>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>
#include <chrono>
#include "../include/MultiLauncher/PlaytimeManager.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <urlmon.h>
#include <tlhelp32.h>
#pragma comment(lib, "urlmon.lib")
#else
#include <unistd.h>
#include <sys/wait.h>
#include <curl/curl.h>
#include <cstdio>
#include <dirent.h>
#include <cstring>
#endif

namespace MultiLauncher {

    Game::Game(const std::string& n, const LauncherType launch, const std::filesystem::path& p, const std::string& exeName, int appid) 
        : name(n), launcher(launch), path(p), executableName(exeName), gameState(STOPPED), steamAppId(appid), bannerLoaded(false), banner{nullptr, 0, 0}
    {
        if (executableName.empty()) {
            executableName = name;
            executableName.erase(std::remove_if(executableName.begin(), executableName.end(), 
                [](unsigned char c){ return std::isspace(c); }), executableName.end());
            executableName += ".exe";
        }
    }

    Game::~Game() {
        if(banner.srv) {
#ifdef _WIN32
            banner.srv->Release();
#else
            GLuint id = (GLuint)(intptr_t)banner.srv;
            glDeleteTextures(1, &id);
#endif
            banner.srv = nullptr;
        }
    }
#ifndef _WIN32
    bool url_exists(const std::string& url) {
        CURL* curl = curl_easy_init();
        if (!curl) return false;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L); // HEAD
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
        curl_easy_setopt(curl, ERROR_TIMEOUT, 10L);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        return res == CURLE_OK;
    }
#endif

    void Game::launchAsync() {
        if(status.load() != GameStatus::Idle) return;

        status.store(GameStatus::Launching);
        
        std::thread([this](){
            try {
                auto start = std::chrono::steady_clock::now();
                
                launch();
                
                auto end = std::chrono::steady_clock::now();
                auto elapsedMinutes = std::chrono::duration_cast<std::chrono::minutes>(end - start).count();

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
        std::string exePath = path.string();
        
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
                // Background poller will update to Running when process is detected
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
        // std::string exePath = path.string(); // Already defined
        std::string workDir = path.parent_path().string();
        std::string cmdLine = "\"" + exePath + "\"";

        if (CreateProcessA(NULL, (LPSTR)cmdLine.c_str(), NULL, NULL, TRUE, 0, NULL, workDir.c_str(), &si, &pi)) 
        {
            status = GameStatus::Launching;
            
            CloseHandle(hChildStd_OUT_Wr);
            
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
            // Linux
            Logger::instance().info("Launching game: " + name);
            
            pid_t pid = fork();
            if (pid == 0) {
                // Child process
                if (launcher == STEAM) {
                    execlp("xdg-open", "xdg-open", exePath.c_str(), nullptr);
                } else {
                    execl(exePath.c_str(), exePath.c_str(), nullptr);
                }
                exit(1);
            } else if (pid > 0) {
                // Parent process
                status = GameStatus::Launching;
                int statusCode;
                waitpid(pid, &statusCode, 0);
            } else {
                Logger::instance().error("Failed to fork process for: " + name);
            }
#endif
    }

#ifdef _WIN32
    bool Game::LoadTextureFromFile(ID3D11Device* device, const wchar_t* filename, BannerTexture& out_banner) const {

        if (!device) return false;

        char str[1024];
        WideCharToMultiByte(CP_UTF8, 0, filename, -1, str, 1024, NULL, NULL);

        Logger::instance().info(std::string("Attempting to load banner from: ") + str);

        int image_width = 0;
        int image_height = 0;
        int channels = 0;
        unsigned char* image_data = stbi_load(str, &image_width, &image_height, &channels, 4);
        if (image_data == NULL) {
            Logger::instance().error(std::string("Failed to load image: ") + str + " - " + stbi_failure_reason());
            return false;
        }

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
#else
    bool Game::LoadTextureFromFile(const char* filename, BannerTexture& out_banner) const {
        int image_width = 0;
        int image_height = 0;
        int channels = 0;
        unsigned char* image_data = stbi_load(filename, &image_width, &image_height, &channels, 4);
        if (image_data == NULL) {
            Logger::instance().error(std::string("Failed to load image: ") + filename + " - " + stbi_failure_reason());
            return false;
        }

        GLuint image_texture;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);

        // Setup filtering parameters for display
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Upload pixels into texture
#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
        stbi_image_free(image_data);

        out_banner.srv = (void*)(intptr_t)image_texture;
        out_banner.width = image_width;
        out_banner.height = image_height;

        return true;
    }
#endif

    static std::string makeBannerKey(const std::string& name) {
        std::string s = name;
        std::transform(s.begin(), s.end(), s.begin(), 
            [](unsigned char c){ return std::tolower(c); });
        
        s.erase(std::remove_if(s.begin(), s.end(), 
            [](unsigned char c){ return !std::isalnum(c); }), s.end());
        return s;
    }

#ifdef _WIN32
    bool Game::loadBanner(ID3D11Device* device) {
        if (bannerStatus == BannerLoaded) return true;
        if (bannerStatus == BannerDownloading) return false;
        if (bannerStatus == BannerFailed) return false;

        // Ready to load texture from disk (Main Thread)
        if (bannerStatus == BannerReadyToLoad) {
            std::string key = makeBannerKey(name);
            std::vector<std::string> exts = { ".jpg", ".png" };
            
            // Check steam cache first
            if (steamAppId > 0) {
                 std::wstring local = L"assets/cache/" + std::to_wstring(steamAppId) + L"_hero.jpg";
                 if(std::filesystem::exists(local)) {
                     if(LoadTextureFromFile(device, local.c_str(), banner)){
                        bannerLoaded = true;
                        bannerStatus = BannerLoaded;
                        return true;
                     }
                 }
            }
            
            // Local fallbacks
            for(const auto& ext : exts) {
                std::string pathStr = "assets/banners/" + key + ext;
                std::wstring wpath(pathStr.begin(), pathStr.end());
                if(std::filesystem::exists(pathStr)) {
                     if(LoadTextureFromFile(device, wpath.c_str(), banner)){
                        bannerLoaded = true;
                        bannerStatus = BannerLoaded;
                        return true;
                     }
                }
            }
            // If we reached here, even though we were ready to load, something failed
            bannerStatus = BannerFailed;
            return false;
        }

        // Only here if BannerNotLoaded

        // Check if we already have files locally to skip download
        if (steamAppId > 0) {
             if (!std::filesystem::exists("assets/cache")) {
                std::filesystem::create_directories("assets/cache");
            }
            std::wstring local = L"assets/cache/" + std::to_wstring(steamAppId) + L"_hero.jpg";
            if(std::filesystem::exists(local)) {
                bannerStatus = BannerReadyToLoad;
                return false; // Will load next frame
            }

            // Needs download
            bannerStatus = BannerDownloading;
            std::thread([this, local]() {
                std::wstring url = L"https://cdn.cloudflare.steamstatic.com/steam/apps/" + std::to_wstring(steamAppId) + L"/library_hero.jpg";
                HRESULT hr = URLDownloadToFileW(nullptr, url.c_str(), local.c_str(), 0, nullptr);
                
                if (FAILED(hr) || !std::filesystem::exists(local)) {
                    // Cleanup
                    if (std::filesystem::exists(local)) std::filesystem::remove(local);
                    
                    url = L"https://cdn.cloudflare.steamstatic.com/steam/apps/" + std::to_wstring(steamAppId) + L"/header.jpg";
                    hr = URLDownloadToFileW(nullptr, url.c_str(), local.c_str(), 0, nullptr);
                    
                    if (FAILED(hr)) {
                        bannerStatus = BannerFailed;
                        return;
                    }
                }
                bannerStatus = BannerReadyToLoad;
            }).detach();

            return false;
        }

        // No steam ID, check local files
        // If they exist they will be caught by ReadyToLoad logic after we set it
        std::string key = makeBannerKey(name);
        std::vector<std::string> exts = { ".jpg", ".png" };
        for(const auto& ext : exts) {
            std::string pathStr = "assets/banners/" + key + ext;
            if(std::filesystem::exists(pathStr)) {
                 bannerStatus = BannerReadyToLoad;
                 return false;
            }
        }
        
        bannerStatus = BannerFailed;
        return false;
    }
#else
    bool Game::loadBanner() {
        if (bannerStatus == BannerLoaded) return true;
        if (bannerStatus == BannerDownloading) return false;
        if (bannerStatus == BannerFailed) return false;

        // Ready to load texture from disk (Main Thread)
        if (bannerStatus == BannerReadyToLoad) {
            if (steamAppId > 0) {
                std::string local = "assets/cache/" + std::to_string(steamAppId) + "_hero.jpg";
                if(std::filesystem::exists(local)) {
                     if(LoadTextureFromFile(local.c_str(), banner)){
                        bannerLoaded = true;
                        bannerStatus = BannerLoaded;
                        return true;
                    }
                }
            }
             // Local fallback
            std::string key = makeBannerKey(name);
            std::vector<std::string> exts = { ".jpg", ".png" };
            for(const auto& ext : exts) {
                std::string pathStr = "assets/banners/" + key + ext;
                if(std::filesystem::exists(pathStr)) {
                     if(LoadTextureFromFile(pathStr.c_str(), banner)){
                        bannerLoaded = true;
                        bannerStatus = BannerLoaded;
                        return true;
                     }
                }
            }
             // If we reached here, even though we were ready to load, something failed
            bannerStatus = BannerFailed;
            return false;
        }

        // Only here if BannerNotLoaded

        if (steamAppId > 0) {
             if (!std::filesystem::exists("assets/cache")) {
                std::filesystem::create_directories("assets/cache");
            }
            std::string local = "assets/cache/" + std::to_string(steamAppId) + "_hero.jpg";
            
            // Check if we have it cached
            if(std::filesystem::exists(local)) {
                bannerStatus = BannerReadyToLoad;
                return false;
            }

            // Needs download
            bannerStatus = BannerDownloading;
            
            std::thread([this, local]() {
                Logger::instance().info("Downloading banner for appid " + std::to_string(steamAppId));
                std::string url = "https://cdn.cloudflare.steamstatic.com/steam/apps/" + std::to_string(steamAppId) + "/library_hero.jpg";
                
                auto tryDownload = [&](const std::string& u) -> bool {
                    if(!url_exists(u)) return false;
                    
                    CURL* curl = curl_easy_init();
                    if(!curl) return false;
                    
                    FILE* file = fopen(local.c_str(), "wb");
                    if(!file){
                        curl_easy_cleanup(curl);
                        return false;
                    }
                    curl_easy_setopt(curl, CURLOPT_URL, u.c_str());
                    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
                    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
                    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
                    curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0");
                    
                    CURLcode res = curl_easy_perform(curl);
                    fclose(file);
                    curl_easy_cleanup(curl);
                    return res == CURLE_OK;
                };

                if(!tryDownload(url)){
                    url = "https://cdn.cloudflare.steamstatic.com/steam/apps/" + std::to_string(steamAppId) + "/header.jpg";
                    if(!tryDownload(url)){
                         if(std::filesystem::exists(local)) std::filesystem::remove(local);
                         bannerStatus = BannerFailed;
                         return;
                    }
                }
                bannerStatus = BannerReadyToLoad;
            }).detach();

            return false;
        }
        
        // Local fallback check
        std::string key = makeBannerKey(name);
        std::vector<std::string> exts = { ".jpg", ".png" };
        for(const auto& ext : exts) {
            std::string pathStr = "assets/banners/" + key + ext;
            if(std::filesystem::exists(pathStr)) {
                 bannerStatus = BannerReadyToLoad;
                 return false;
            }
        }
        
        bannerStatus = BannerFailed;
        return false;
    }
#endif

    void Game::updateStatus() {
        bool running = isProcessRunning(executableName);
        if (running) {
            status = GameStatus::Running;
        } else {
            if (status != GameStatus::Launching) {
                status = GameStatus::Idle;
                gameState = STOPPED;
            }
        }
    }

    bool Game::isProcessRunning(const std::string& processName) const {
        #ifdef _WIN32
        bool found = false;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot != INVALID_HANDLE_VALUE) {
            PROCESSENTRY32 pe32;
            pe32.dwSize = sizeof(PROCESSENTRY32);
            if (Process32First(hSnapshot, &pe32)) {
                // Case-insensitive comparison
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
        // Linux
        DIR* dir = opendir("/proc");
        if (dir == nullptr) return false;
        
        bool found = false;
        struct dirent* entry;
        
        while ((entry = readdir(dir)) != nullptr) {
            // Skip non-numeric directories
            if (entry->d_type != DT_DIR) continue;
            
            bool isNumeric = true;
            for (char* c = entry->d_name; *c; c++) {
                if (!isdigit(*c)) {
                    isNumeric = false;
                    break;
                }
            }
            if (!isNumeric) continue;
            
            // Read the cmdline file
            std::string cmdlinePath = std::string("/proc/") + entry->d_name + "/cmdline";
            std::ifstream cmdlineFile(cmdlinePath);
            if (!cmdlineFile.is_open()) continue;
            
            std::string cmdline;
            std::getline(cmdlineFile, cmdline, '\0');
            cmdlineFile.close();
            
            // Extract executable name from path
            size_t lastSlash = cmdline.find_last_of('/');
            std::string exeName = (lastSlash != std::string::npos) ? cmdline.substr(lastSlash + 1) : cmdline;
            
            // Compare case-insensitively
            std::string searchName = processName;
            std::transform(searchName.begin(), searchName.end(), searchName.begin(), ::tolower);
            std::transform(exeName.begin(), exeName.end(), exeName.begin(), ::tolower);
            
            if (searchName == exeName) {
                found = true;
                break;
            }
        }
        
        closedir(dir);
        return found;
        #endif
    }

}
