#include "IScanner.hpp"
#include <vector>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "../external/JSON/json.hpp"
using json = nlohmann::json;

namespace MultiLauncher{
    class EpicScanner : public IScanner{
        public:
            std::vector<Game> scan() override {
                std::vector<Game> games;
                std::filesystem::path manifestDir = R"(C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests)";
                
                if(!std::filesystem::exists(manifestDir)){
                    Logger::instance().error(std::string("Epic Games manifest directory not found at: ") + manifestDir.string());
                    return games;
                }else{
                    Logger::instance().info(std::string("Epic games library found: ") + manifestDir.string());
                }
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
                Logger::instance().info("Total epic games found: " + std::to_string(games.size()));
                return games;
            }
    };
} // namespace MultiLauncher