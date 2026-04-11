#pragma once
#include "IScanner.hpp"
#include <filesystem>
#include <string>
#include <fstream>
#include <iostream>
#include "../JSON/json.hpp"

using json = nlohmann::json;

namespace MultiLauncher{
    class GogScanner : public IScanner{
        public:
            std::vector<Game> scan(bool forceRefresh = false) override {
                std::vector<Game> games;
                // TODO:
                // we have to scan C:Program Files (x86)\GOG Galaxy\Games\
                // and find goggame-XXXXXXX.info
#ifdef _WIN32
                // std::filesystem::path manifestDir = R"(C:\Program Files (x86)\GOG Galaxy\Games)"; instead we will use settings.json
                std::filesystem::path settingFile = std::filesystem::current_path() / "assets" / "settings.json";
                if(!std::filesystem::exists(settingFile)){
                    std::ofstream settingsJson (settingFile);
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
                    settingsJson << j.dump(4);
                    settingsJson.close();
                }
                std::ifstream file (settingFile);
                json j = json::parse(file);
                std::string gogPath = j["Windows"]["gog"]["paths"][0]["path"];
                std::filesystem::path manifestDir = gogPath;
#else
                return games;  // there is no official GOG launcher for linux
#endif
                if(!exists(manifestDir) ){
                    return games;
                }
                std::vector<std::filesystem::path> gogGames;
                std::filesystem::path gogFile = "";
                for(const auto& entry : std::filesystem::recursive_directory_iterator(manifestDir)){
                    if(entry.path().extension() == ".info"){
                        std::string filename = entry.path().filename().string();
                        if(filename.substr(0, 8) == "goggame-"){ //goggame-
                            Logger::instance().info("Found gog info file");
                            gogFile = entry;
                            gogGames.push_back(entry);
                        }
                    }
                }

                try{
                    for(auto i : gogGames){
                        std::ifstream file;
                        file.open(i); // otwieranie pliku
                        json data = json::parse(file);
                        std::string name = data["name"]; // name of the game
                        std::filesystem::path dirPath = R"(C:\Program Files (x86)\GOG Galaxy\Games\)" / std::filesystem::path(name);
                        std::filesystem::path exe = data["playTasks"][0]["path"]; // executable name
                        std::filesystem::path exePath = dirPath / exe; // full path to the executable
                        std::string id = data["gameId"]; // gameId
                        int gameId = std::stoi(id);
                        games.emplace_back(
                            name,
                            Game::LauncherType::GOG,
                            exePath,
                            exe.filename().string(),
                            gameId
                        );

                        file.close();
                    }
                }catch(json::parse_error &e){
                    std::cout << "There was a parse error: " << e.what() << std::endl;
                }catch(std::ios_base::failure &e){
                    std::cout << "Error while opening a file: " << e.what() << std::endl;
                }
                for(int i = 0; i < games.size(); i++){
                    std::cout << games[i].getName() << std::endl;
                }
                if(games.empty()){
                    std::string manifestDirStr = manifestDir.string();
                    Logger::instance().info("No GOG games found in " + manifestDirStr);
                }
                return games;
            }
    };
} // namespace MultiLauncher