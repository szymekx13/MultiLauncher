#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <cctype>
#include "Game.hpp"
#include "ProcessRunner.hpp"
#include <regex>
#include "../external/JSON/json.hpp"
using json = nlohmann::json;

namespace MultiLauncher {
    struct EpicGameInfo {
        std::string appName;
        std::string title;
    };

    class EpicProvider {
    private:
        static std::string getLegendaryBinary() {
#ifdef _WIN32
            return "tools/legendary/legendary.exe";
#else
            return "tools/legendary/legendary";
#endif
        }

    public:
        static bool isAvailable() {
            return std::filesystem::exists(getLegendaryBinary());
        }

        static void authenticate() {
            ProcessRunner::runAsync(getLegendaryBinary() + " auth", [](int code) {
                Logger::instance().info("Legendary auth process completed with code: " + std::to_string(code));
            });
        }

        static std::vector<EpicGameInfo> listGames(bool forceRefresh = false) {
            std::vector<EpicGameInfo> games;
            std::string fullJson;
            bool jsonStarted = false;

            Logger::instance().info(std::string("Fetching Epic Games list via Legendary") + (forceRefresh ? " (forcing refresh)..." : "..."));

            std::string cmd = getLegendaryBinary() + " --quiet list --json";
            if (forceRefresh) {
                cmd += " --force-refresh";
            }

            int exitCode = ProcessRunner::run(cmd, [&](const std::string& line) {
                if (line.empty()) return;

                // Better detection of JSON vs Log lines
                // Log lines typically start with [tag] LEVEL:
                // But JSON also starts with [ or {
                bool isLogLine = false;
                if (line.size() > 1 && line[0] == '[') {
                    // Check if it matches log pattern like [cli], [Core], [DL], [updater]
                    // Log lines usually have a space after the closing bracket if they are tags
                    size_t closingBracket = line.find(']');
                    if (closingBracket != std::string::npos && closingBracket < 20) {
                        bool onlyAlnum = true;
                        for (size_t i = 1; i < closingBracket; ++i) {
                            if (!isalnum((unsigned char)line[i]) && line[i] != '-') {
                                onlyAlnum = false;
                                break;
                            }
                        }
                        if (onlyAlnum && closingBracket + 1 < line.size()) {
                            if (line[closingBracket + 1] == ' ' || isupper((unsigned char)line[closingBracket + 1])) {
                                 // Likely a log line if it's followed by a space or uppercase (LEVEL)
                                 // e.g., "[cli] INFO:" or "[Core]DEBUG:"
                                 isLogLine = true;
                            }
                        }
                    }
                }

                if (!jsonStarted) {
                    if (!isLogLine && (line[0] == '[' || line[0] == '{')) {
                        jsonStarted = true;
                        fullJson += line;
                    } else if (!isLogLine && line.find("Logging in...") == std::string::npos) {
                        Logger::instance().info("[Legendary Info] " + line);
                    }
                } else {
                    // Once JSON started, we append everything that doesn't look like a new log message
                    if (isLogLine) {
                        Logger::instance().info("[Legendary Info] " + line);
                    } else {
                        fullJson += line;
                    }
                }
            });
            if (exitCode == 1) {
                Logger::instance().info("Legendary: Not logged in. Please connect your Epic Games account.");
                return games;
            } else if (exitCode != 0) {
                Logger::instance().error("Legendary list-games failed with code: " + std::to_string(exitCode));
                return games;
            }

            if (fullJson.empty()) {
                // If we got exit code 0 but no JSON, maybe there's an issue or just no games
                return games;
            }

            try {
                auto j = json::parse(fullJson);
                if (j.is_array()) {
                    for (auto& item : j) {
                        EpicGameInfo info;
                        if (item.contains("app_name")) info.appName = item["app_name"].get<std::string>();
                        if (item.contains("title")) info.title = item["title"].get<std::string>();
                        
                        if (!info.appName.empty() && !info.title.empty()) {
                            games.push_back(info);
                        }
                    }
                }
            } catch (const std::exception& e) {
                Logger::instance().error("Failed to parse Legendary JSON: " + std::string(e.what()));
            }

            if (!games.empty()) {
                Logger::instance().info("Successfully synchronized " + std::to_string(games.size()) + " games from Epic.");
            }
            return games;
        }

        static void loginWithCode(const std::string& code, std::function<void()> onSuccess = nullptr) {
            std::string cmd = getLegendaryBinary() + " auth --code " + code + " --yes";
            Logger::instance().info("Attempting to login to Epic Games with authorization code...");
            ProcessRunner::runAsync(cmd, [onSuccess](int exitCode) {
                if (exitCode == 0) {
                    Logger::instance().info("Successfully logged into Epic Games!");
                    if (onSuccess) onSuccess();
                } else {
                    Logger::instance().error("Epic Games login failed with code: " + std::to_string(exitCode));
                }
            });
        }

        static void logout() {
            std::string cmd = getLegendaryBinary() + " auth --delete --yes";
            ProcessRunner::runAsync(cmd, [](int exitCode) {
                if (exitCode == 0) {
                    Logger::instance().info("Logged out from Epic Games.");
                } else {
                    Logger::instance().error("Logout failed with code: " + std::to_string(exitCode));
                }
            });
        }

        static void installGame(Game& game, const std::string& basePath = "") {
            std::string cmd = getLegendaryBinary() + " install " + game.getName() + " --skip-sdl --repair";
            if (!basePath.empty()) {
                cmd += " --base-path \"" + basePath + "\"";
            }

            game.status = Game::GameStatus::Downloading;
            
            ProcessRunner::runAsync(cmd, [&](int code) {
                if (code == 0) game.status = Game::GameStatus::Idle;
                else game.status = Game::GameStatus::Error;
            }, [&](const std::string& line) {
                std::regex progressRegex(R"(\[DL\]\s+(\d+)%\s+\|\s+([\d\.]+GB\s+/\s+[\d\.]+GB)\s+\|\s+([\d\. ]+\w+/s))");
                std::smatch match;
                if (std::regex_search(line, match, progressRegex)) {
                    game.setProgress(std::stof(match[1].str()) / 100.0f);
                    game.setETA(match[3].str());
                }
                Logger::instance().info("[Legendary] " + line);
            });
        }

        static void launchGame(const std::string& appName) {
            ProcessRunner::runAsync(getLegendaryBinary() + " launch " + appName, [](int code) {
                Logger::instance().info("Legendary launch process completed with code: " + std::to_string(code));
            });
        }
    };
}
