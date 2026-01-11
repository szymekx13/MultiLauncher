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
        ID3D11ShaderResourceView* srv = nullptr;
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
                Running
            };
        private:
            std::string name; // name of the game
            LauncherType launcher;
            std::filesystem::path path; //path to .exe
            State gameState;
            
            // New members
            int steamAppId;
            mutable bool bannerLoaded;
            mutable BannerTexture banner;

            // Internal helper
            bool LoadTextureFromFile(ID3D11Device* device, const wchar_t* filename, BannerTexture& out_banner) const;

        public:
            std::atomic<GameStatus> status = GameStatus::Idle; 
            
            Game(const std::string& n, const LauncherType launch, const std::filesystem::path& p, int appid = -1);
            ~Game();

            const void launch();
            
            // GETTERS
            const std::string& getName() const { return name; };
            const std::filesystem::path& getPath() const { return path; };
            const std::string getState() const{
                switch(gameState){
                    case STOPPED: return "Not running";
                    case STARTING: return "Starting";
                    case RUNNING: return "Runing";
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
            const std::string getExeName() const {
                std::string nameC = this->name;
                for(auto it = nameC.begin(); it != nameC.end(); ){
                    if(*it == ' '){
                        it = nameC.erase(it);
                    }else{
                        ++it;
                    }
                }
                nameC += ".exe";
                return nameC;
            }
            void setState(State s) { gameState = s; }
            void launchAsync(){
                if(status != GameStatus::Idle){
                    return;
                }

                status = GameStatus::Launching;
                std::thread([this](){
                    try{
                        status = GameStatus::Launching;
                        launch();
                    }catch(...){
                        Logger::instance().error("Failed to launch " + name);
                    }
                    status = GameStatus::Idle;
                }).detach();
            }

            // Banner support
            bool loadBanner(ID3D11Device* device) const;
            const BannerTexture& getBanner() const { return banner; }
            int getSteamAppId() const { return steamAppId; }

            Game(Game&& other) noexcept 
                : name(std::move(other.name)), 
                  launcher(std::move(other.launcher)), 
                  path(std::move(other.path)), 
                  gameState(std::move(other.gameState)), 
                  status(other.status.load()),
                  steamAppId(other.steamAppId),
                  bannerLoaded(other.bannerLoaded),
                  banner(other.banner)
            {
                // steal srv ownership
                other.banner.srv = nullptr;
                other.bannerLoaded = false;
            }

            // Simple move assignment (simplified)
            Game& operator=(Game&& other) noexcept{
                if(this != &other){
                    name = std::move(other.name);
                    launcher = std::move(other.launcher);
                    path = std::move(other.path);
                    gameState = std::move(other.gameState);
                    status.store(other.status.load());
                    
                    steamAppId = other.steamAppId;
                    bannerLoaded = other.bannerLoaded;
                    banner = other.banner;
                    
                    other.banner.srv = nullptr;
                    other.bannerLoaded = false;
                }
                return *this;
            }

            Game(const Game&) = delete;
            Game& operator=(const Game&) = delete;
    };
} // namespace MultiLauncher