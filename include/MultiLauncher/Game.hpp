#pragma once
#include "Logger.hpp"
#include <string>
#include <filesystem>
#include <stdexcept>
#include <utility>
#include <atomic>
#include <thread>
#ifdef _WIN32
#include <windows.h>
#include <d3d11.h>
#endif

namespace MultiLauncher{
    struct BannerTexture {
#ifdef _WIN32
        ID3D11ShaderResourceView* srv = nullptr;
#else
        void* srv = nullptr;
#endif
        int width = 0;
        int height = 0;
    };

    class Game{
        public:
            enum LauncherType{
                EPIC,
                STEAM,
                GOG
            };
            enum State{
                STOPPED,
                STARTING,
                RUNNING
            };
            enum GameStatus{
                Idle,
                Launching,
                Running,
                Downloading,
                Installing,
                Error
            };
            enum BannerStatus{
                BannerNotLoaded,
                BannerDownloading,
                BannerReadyToLoad,
                BannerLoaded,
                BannerFailed
            };
        private:
            std::string name;
            LauncherType launcher;
            std::filesystem::path path;
            std::string executableName;
            State gameState;
            int steamAppId;
            mutable bool bannerLoaded;
            mutable BannerTexture banner;
            float progress = 0.0f;
            std::string eta = "";

            // Internal helper
#ifdef _WIN32
            bool LoadTextureFromFile(ID3D11Device* device, const wchar_t* filename, BannerTexture& out_banner) const;
#else
            bool LoadTextureFromFile(const char* filename, BannerTexture& out_banner) const;
#endif

        public:
            void updateStatus();
            bool isProcessRunning(const std::string& processName) const;
            std::atomic<GameStatus> status = GameStatus::Idle; 
            std::atomic<BannerStatus> bannerStatus = BannerStatus::BannerNotLoaded;
            
            Game(const std::string& n, const LauncherType launch, const std::filesystem::path& p, const std::string& exeName = "", int appid = -1);
            ~Game();

            const void launch();
            
            // GETTERS
            const std::string& getName() const { return name; };
            const std::filesystem::path& getPath() const { return path; };
            const float getProgress() const { return progress; };
            const std::string getETA() const { return eta; };
            const std::string getState() const{
                switch(gameState){
                    case STOPPED: return "Not running";
                    case STARTING: return "Starting";
                    case RUNNING: return "Running";
                }
                return "Unknown";
            }
            const std::string getLauncher() const {
                switch (launcher){
                    case EPIC: return "Epic Games Store";
                    case STEAM: return "Steam";
                    case GOG: return "GOG Galaxy";
                } 
                return "Unknown";
            }
            const std::string& getExeName() const {
                return executableName;
            }
            void setState(State s) { gameState = s; }
            void setProgress(float p) { progress = p; }
            void setETA(const std::string& e) { eta = e; }
            void launchAsync();

#ifdef _WIN32
            bool loadBanner(ID3D11Device* device);
#else
            bool loadBanner();
#endif
            const BannerTexture& getBanner() const { return banner; }
            int getSteamAppId() const { return steamAppId; }

            Game(Game&& other) noexcept 
                : name(std::move(other.name)), 
                  launcher(std::move(other.launcher)), 
                  path(std::move(other.path)), 
                  executableName(std::move(other.executableName)),
                  gameState(std::move(other.gameState)), 
                  status(other.status.load()),
                  steamAppId(other.steamAppId),
                  bannerLoaded(other.bannerLoaded),
                  banner(other.banner),
                  bannerStatus(other.bannerStatus.load())
            {
                other.banner.srv = nullptr;
                other.bannerLoaded = false;
                other.bannerStatus = BannerStatus::BannerNotLoaded;
            }

            Game& operator=(Game&& other) noexcept{
                if(this != &other){
                    name = std::move(other.name);
                    launcher = std::move(other.launcher);
                    path = std::move(other.path);
                    executableName = std::move(other.executableName);
                    gameState = std::move(other.gameState);
                    status.store(other.status.load());
                    
                    steamAppId = other.steamAppId;
                    bannerLoaded = other.bannerLoaded;
                    banner = other.banner;
                    bannerStatus.store(other.bannerStatus.load());
                    
                    other.banner.srv = nullptr;
                    other.bannerLoaded = false;
                    other.bannerStatus = BannerStatus::BannerNotLoaded;
                }
                return *this;
            }

            Game(const Game&) = delete;
            Game& operator=(const Game&) = delete;
    };
} // namespace MultiLauncher