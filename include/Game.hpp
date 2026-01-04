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
        private:
            std::string name; // name of the game
            LauncherType launcher;
            std::filesystem::path path; //path to .exe

        public: // FUNCITIONS DECLARATIONS
            Game(const std::string& n, const LauncherType launch, const std::filesystem::path& p) : name(n), launcher(launch), path(p) 
            {
                if(!std::filesystem::exists(p)){
                    throw std::runtime_error("Executable not found");
                }
            }
            ~Game() = default;
            void launch() const{
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
            const LauncherType getLauncher() const { return launcher; };
            const std::filesystem::path& getPath() const { return path; };
    };
}