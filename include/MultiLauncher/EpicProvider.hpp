#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "Game.hpp"
#include "ProcessRunner.hpp"
#include <regex>

namespace MultiLauncher {
    struct EpicGameInfo {
        std::string appName;
        std::string title;
    };

    class EpicProvider {
    public:
        static bool isAvailable() {
            return std::filesystem::exists("tools/legendary/legendary.exe");
        }

        static void authenticate() {
            ProcessRunner::runAsync("tools/legendary/legendary.exe auth", [](int code) {
                Logger::instance().info("Legendary auth process completed with code: " + std::to_string(code));
            });
        }

        static std::vector<EpicGameInfo> listGames() {
            std::vector<EpicGameInfo> games;
            ProcessRunner::run("tools/legendary/legendary.exe list-games", [&](const std::string& line) {
                // Skip headers and separators
                if (line.empty() || line.find("---") != std::string::npos || line.find("AppName") != std::string::npos) {
                    return;
                }

                // Split by '|'
                std::vector<std::string> columns;
                size_t start = 0;
                size_t end = line.find('|');
                while (end != std::string::npos) {
                    columns.push_back(line.substr(start, end - start));
                    start = end + 1;
                    end = line.find('|', start);
                }
                columns.push_back(line.substr(start));

                if (columns.size() >= 2) {
                    std::string appName = columns[0];
                    std::string title = columns[1];
                    
                    // Trim
                    auto trim = [](std::string& s) {
                        s.erase(0, s.find_first_not_of(" \t\r\n"));
                        s.erase(s.find_last_not_of(" \t\r\n") + 1);
                    };
                    trim(appName);
                    trim(title);

                    if (!appName.empty() && !title.empty()) {
                        games.push_back({appName, title});
                    }
                }
            });
            return games;
        }

        static void installGame(Game& game, const std::string& basePath = "") {
            std::string cmd = "tools/legendary/legendary.exe install " + game.getName() + " --skip-sdl --repair";
            if (!basePath.empty()) {
                cmd += " --base-path \"" + basePath + "\"";
            }

            game.status = Game::GameStatus::Downloading;
            
            ProcessRunner::runAsync(cmd, [&](int code) {
                if (code == 0) game.status = Game::GameStatus::Idle;
                else game.status = Game::GameStatus::Error;
            }, [&](const std::string& line) {
                // Regex for progress: [DL]  42% | 8.2GB / 19.3GB | 12MB/s
                std::regex progressRegex(R"(\[DL\]\s+(\d+)%\s+\|\s+([\d\.]+GB\s+/\s+[\d\.]+GB)\s+\|\s+([\d\.]+\w+/s))");
                std::smatch match;
                if (std::regex_search(line, match, progressRegex)) {
                    game.setProgress(std::stof(match[1].str()) / 100.0f);
                    game.setETA(match[3].str()); // Using speed as ETA info for now or we can parse ETA if legendary provides it
                }
                Logger::instance().info("[Legendary] " + line);
            });
        }

        static void launchGame(const std::string& appName) {
            ProcessRunner::runAsync("tools/legendary/legendary.exe launch " + appName, [](int code) {
                Logger::instance().info("Legendary launch process completed for " + std::to_string(code));
            });
        }
    };
}
