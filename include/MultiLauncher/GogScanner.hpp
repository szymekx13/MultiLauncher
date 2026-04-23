#pragma once
#include "IScanner.hpp"
#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>
#ifdef _WIN32
    #include "Config.hpp"
#endif
#include "../external/JSON/json.hpp"
#include "Logger.hpp"

using json = nlohmann::json;

namespace MultiLauncher{
    class GogScanner : public IScanner{
        public:
            std::vector<Game> scan(bool forceRefresh = false) override {
                std::vector<Game> games;
                (void)forceRefresh;
#ifdef _WIN32
                std::filesystem::path settingFile = Config::getSettingsPath();
                if(!std::filesystem::exists(settingFile)){
                    // Fallback or create default if needed, but App usually creates it
                }
                
                if(std::filesystem::exists(settingFile)){
                    std::ifstream input(settingFile);
                    json j = json::parse(input);
                    if (j.contains("Windows") && j["Windows"].contains("gog")) {
                        std::string gogPath = j["Windows"]["gog"]["paths"][0]["path"];
                        std::filesystem::path manifestDir = gogPath;
                        
                        if(std::filesystem::exists(manifestDir)){
                            std::vector<std::filesystem::path> gogGames;
                            for(const auto& entry : std::filesystem::recursive_directory_iterator(manifestDir)){
                                if(entry.path().extension() == ".info"){
                                    std::string filename = entry.path().filename().string();
                                    if(filename.substr(0, 8) == "goggame-"){
                                        gogGames.push_back(entry.path());
                                    }
                                }
                            }

                            for(const auto& infoPath : gogGames){
                                try {
                                    std::ifstream f(infoPath);
                                    json data = json::parse(f);
                                    std::string name = data["name"];
                                    std::filesystem::path dirPath = manifestDir / name;
                                    std::filesystem::path exe = data["playTasks"][0]["path"];
                                    std::filesystem::path exePath = dirPath / exe;
                                    std::string idStr = data["gameId"];
                                    int gameId = std::stoi(idStr);
                                    
                                    games.emplace_back(
                                        name,
                                        Game::LauncherType::GOG,
                                        exePath,
                                        exe.filename().string(),
                                        gameId
                                    );
                                } catch (...) {}
                            }
                        }
                    }
                }
#endif
                return games;
            }
    };
} // namespace MultiLauncher
