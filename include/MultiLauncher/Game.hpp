#pragma once
#include <string>
#include <filesystem>
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#endif

namespace MultiLauncher{
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
        private:
            std::string name; // name of the game
            LauncherType launcher;
            std::filesystem::path path; //path to .exe
            State gameState;
        public: // FUNCITIONS DECLARATIONS
            Game(const std::string& n, const LauncherType launch, const std::filesystem::path& p) : name(n), launcher(launch), path(p), gameState(STOPPED) {}
            ~Game() = default;
            const void launch() {
                gameState = STARTING;
                #ifdef _WIN32
                    HINSTANCE result = ShellExecuteA(
                        NULL,                   // hwnd
                        "open",                 // operation
                        path.string().c_str(),  // file
                        NULL,                   // Parameters
                        NULL,                   // Directory
                        SW_SHOWNORMAL           // ShowCmd
                    );

                    if(reinterpret_cast<intptr_t>(result) <= 32){
                        throw std::runtime_error("Something went wrong");
                    }
                #else
                    // Here do same thing but for Linux/Unix systems
                #endif
            }
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
    };
} // namespace MultiLauncher