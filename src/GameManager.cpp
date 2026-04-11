#include "../include/MultiLauncher/GameManager.hpp"
#include "../include/external/JSON/json.hpp"
#include "../include/MultiLauncher/Logger.hpp"
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

namespace MultiLauncher{
    void GameManager::updatePaths(std::string steamPath, std::string epicPath, std::string gogPath){
        std::filesystem::path settingsPath = std::filesystem::current_path() / "assets" / "settings.json";
        std::ifstream input(settingsPath);
        if(!input){
            Logger::instance().error("Cannot open settings file, nothing saved");
            return;
        } 
        json j = json::parse(input);
        input.close();

#ifdef _WIN32

        if(!steamPath.empty()) j["Windows"]["steam"]["paths"][0]["path"] = steamPath;
        if(!epicPath.empty()) j["Windows"]["epic"]["paths"][0]["path"] = epicPath;
        if(!gogPath.empty()) j["Windows"]["gog"]["paths"][0]["path"] = gogPath;
#else

        if(!steamPath.empty()) j["Linux"]["steam"]["paths"][0]["path"] = steamPath;
        // since there is no official launcher for epic and gog we will leave it for now
#endif

        Logger::instance().info("Changed paths to - steam: " + steamPath);
        Logger::instance().info("Epic: " + epicPath);
        Logger::instance().info("Gog: " + gogPath);

        std::ofstream updateSettings (settingsPath);
        updateSettings << j.dump(4);
        updateSettings.close();
    }
}// namespace MultiLauncher