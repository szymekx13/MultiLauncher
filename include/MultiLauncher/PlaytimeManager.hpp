#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <fstream>
#include <regex>
#include <iostream>
#include "../external/JSON/json.hpp"

namespace MultiLauncher {

    class PlaytimeManager {
    public:
        static PlaytimeManager& instance() {
            static PlaytimeManager inst;
            return inst;
        }

        void init() {
            loadLocal();
            scanSteam();
        }

        // Returns hours
        float getHours(const std::string& gameName, int steamAppId) {
            float minutes = 0.0f;
            
            if (steamAppId > 0 && steamPlaytimeMap.count(steamAppId)) {
                minutes = (float)steamPlaytimeMap[steamAppId];
            } 
            else if (localPlaytimeMap.count(gameName)) {
                minutes = (float)localPlaytimeMap[gameName];
            }

            return minutes / 60.0f;
        }

        void addPlaytime(const std::string& gameName, int minutes) {
            if (minutes <= 0) return;
            
            localPlaytimeMap[gameName] += minutes;
            saveLocal();
        }

    private:
        PlaytimeManager() {}
        
        std::unordered_map<std::string, int> localPlaytimeMap; // Name -> Minutes
        std::unordered_map<int, int> steamPlaytimeMap;       // AppID -> Minutes

        void loadLocal() {
            if (!std::filesystem::exists("playtime.json")) return;
            try {
                std::ifstream i("playtime.json");
                nlohmann::json j;
                i >> j;
                for (auto& element : j.items()) {
                    localPlaytimeMap[element.key()] = element.value().get<int>();
                }
            } catch (...) {}
        }

        void saveLocal() {
            try {
                nlohmann::json j;
                for (const auto& pair : localPlaytimeMap) {
                    j[pair.first] = pair.second;
                }
                std::ofstream o("playtime.json");
                o << j.dump(4);
            } catch (...) {}
        }

        void scanSteam() {
            std::string steamPath = "C:\\Program Files (x86)\\Steam\\userdata";
            if (!std::filesystem::exists(steamPath)) return;

            for (const auto& entry : std::filesystem::directory_iterator(steamPath)) {
                if (entry.is_directory()) {
                    std::filesystem::path configPath = entry.path() / "config" / "localconfig.vdf";
                    if (std::filesystem::exists(configPath)) {
                        parseVDF(configPath.string());
                    }
                }
            }
        }

        // Basic VDF parser for playtime
        void parseVDF(const std::string& path) {
            std::ifstream file(path);
            if (!file.is_open()) return;

            std::string line;
            std::string currentAppId = "";
            bool inApps = false;
            
            while (std::getline(file, line)) {
                
                std::smatch m;
                static std::regex reAppId(R"(\s*\"(\d+)\"\s*)");
                static std::regex rePlaytime(R"(\s*\"Playtime\"\s*\"(\d+)\"\s*)");

                if (std::regex_match(line, m, reAppId)) {
                    currentAppId = m[1].str();
                }
                else if (!currentAppId.empty() && std::regex_match(line, m, rePlaytime)) {
                    try {
                        int mins = std::stoi(m[1].str());
                        steamPlaytimeMap[std::stoi(currentAppId)] = mins;
                    } catch (...) {}
                }
            }
        }
    };

}
