#pragma once
#include "IScanner.hpp"
#include "EpicProvider.hpp"
#include "Logger.hpp"
#include "../external/JSON/json.hpp"
#include <vector>
#include <filesystem>
#ifdef _WIN32
    #include "Config.hpp"
#endif
#include <fstream>
#include <iostream>
#include <stdexcept>

using json = nlohmann::json;

namespace MultiLauncher{
    class EpicScanner : public IScanner{
        public:
            std::vector<Game> scan(bool forceRefresh = false) override {
                std::vector<Game> games;
    #ifdef _WIN32
                std::filesystem::path settingFile = Config::getSettingsPath();
                std::filesystem::create_directories(settingFile.parent_path());
                // std::filesystem::path manifestDir = R"(C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests)"; insted of this we will use settings.json
                if(!std::filesystem::exists(settingFile)){
                    std::ofstream settingJson (settingFile);
                    json j = json::parse(R"({
                        "Windows": {
                            "steam": {
                            "paths": [
                                {
                                "type": "native",
                                "path": "C:\\Program Files (x86)\\Steam\\steamapps\\libraryfolders.vdf",
                                "format": "steam_vdf"
                                }
                            ]
                            },
                            "epic": {
                            "paths": [
                                {
                                "type": "manifest_dir",
                                "path": "C:\\ProgramData\\Epic\\EpicGamesLauncher\\Data\\Manifests"
                                }
                            ]
                            },
                            "gog": {
                            "paths": [
                                {
                                "type": "game_dir",
                                "path": "C:\\Program Files (x86)\\GOG Galaxy\\Games"
                                }
                            ]
                            }
                        },
                        "Linux": {
                            "steam": {
                            "paths": [
                                {
                                "type": "native",
                                "path": "/home/.local/share/Steam/steamapps/libraryfolders.vdf"
                                },
                                {
                                "type": "flatpak",
                                "path": "/home/.var/app/com.valvesoftware.Steam/data/Steam/steamapps/libraryfolders.vdf"
                                }
                            ]
                            }
                        },
                        "MacOS": {}
                        })");
                    settingJson << j.dump(4);
                    settingJson.close();
                }
                std::ifstream input(settingFile);
                json j = json::parse(input);
                std::string manDir = j["Windows"]["epic"]["paths"][0]["path"];
                std::filesystem::path manifestDir = manDir;
    #else
                std::filesystem::path manifestDir = "";
    #endif

                if(!manifestDir.empty() && std::filesystem::exists(manifestDir)){
                    Logger::instance().info(std::string("Epic games library found: ") + manifestDir.string());
                    for(const auto& entry : std::filesystem::directory_iterator(manifestDir)){
                        if(entry.path().extension() != ".item"){
                            continue;
                        }
                        std::ifstream file(entry.path());
                        if(!file){
                            continue;
                        }

                        json j;

                        try{
                            file >> j;
                        }catch(...){
                            continue;
                        }

                        if(!j.contains("DisplayName") ||
                           !j.contains("InstallLocation") ||
                           !j.contains("LaunchExecutable")){
                            continue;
                        }
                        std::string name = j["DisplayName"].get<std::string>();

                        if (name.find("Version:") != std::string::npos ||
                            name.find("++") != std::string::npos ||
                            name.find("Update") != std::string::npos ||
                            name.find("v.") != std::string::npos) {
                            continue;
                        }

                        std::string launchExe = j["LaunchExecutable"].get<std::string>();
                        std::filesystem::path installLoc = j["InstallLocation"].get<std::string>();
                        std::filesystem::path exePath = installLoc / launchExe;

                        games.emplace_back(
                            name,
                            Game::LauncherType::EPIC,
                            exePath,
                            launchExe
                        );
                    }
                    Logger::instance().info("Total epic games found via manifests: " + std::to_string(games.size()));
                } else if (!manifestDir.empty()) {
                    Logger::instance().error(std::string("Epic Games manifest directory not found at: ") + manifestDir.string());
                }

                if (EpicProvider::isAvailable()) {
                    auto legendaryGames = EpicProvider::listGames(forceRefresh);
                    for (const auto& lg : legendaryGames) {                        bool found = false;
                        for (const auto& existing : games) {
                            if (existing.getName() == lg.title) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            if(lg.title.find("Epic") != std::string::npos){
                                continue;
                            }
                            games.emplace_back(
                                lg.title,
                                Game::LauncherType::EPIC,
                                "",
                                lg.appName
                            );
                        }
                    }
                    Logger::instance().info("Total epic games after Legendary sync: " + std::to_string(games.size()));
                }

                return games;
            }
    };
} // namespace MultiLauncher