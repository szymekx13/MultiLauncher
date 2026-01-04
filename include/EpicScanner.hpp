#include "IScanner.hpp"
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "json.hpp"
using json = nlohmann::json;


namespace MultiLauncher{
    class EpicScanner : public IScanner{
        public:
            std::vector<Game> scan() override {
                std::vector<Game> games;
                // Now we need to scan .item (wich is JSON) files in Epic installation dir
                // parse them, and get the games info
                std::filesystem::path manifestDir = R"(C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests)";
                
                if(!std::filesystem::exists(manifestDir)){
                    throw std::runtime_error("Manifest directory not found");
                    return games;
                }
                for(const auto& entry : std::filesystem::directory_iterator(manifestDir)){
                    if(entry.path().extension() != ".item"){
                        continue; // skip non .item files
                    }
                    std::ifstream file(entry.path());
                    if(!file){
                        continue; // skip if file can't be opened
                    }
                    
                    json j;

                    try{
                        file >> j;
                    }catch(...){
                        continue; // skip if JSON parsing fails
                    }

                    if(!j.contains("DisplayName") ||
                       !j.contains("InstallLocation") ||
                       !j.contains("LaunchExecutable")){
                        continue; // skip if required fields are missing
                    }
                    std::string name = j["DisplayName"].get<std::string>();
                    std::filesystem::path exePath =
                        std::filesystem::path(j["InstallLocation"].get<std::string>()) /
                        j["LaunchExecutable"].get<std::string>();

                    games.emplace_back(
                        name,
                        Game::LauncherType::EPIC,
                        exePath
                    );
                }
                return games;
            }
    };
}