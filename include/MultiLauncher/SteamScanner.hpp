#pragma once
#include "IScanner.hpp"
#include "../external/ValveFileVDF/vdf_parser.hpp"
#include <fstream>
#include <iostream>
#include "Logger.hpp"
#include <vector>
#include <string>
#include <filesystem>
#include <optional> // added

namespace MultiLauncher {

class SteamScanner : public IScanner {
public:
    std::vector<Game> scan() override {
        std::vector<Game> games;

        // 1. Otwórz libraryfolders.vdf
        std::ifstream file(R"(C:\Program Files (x86)\Steam\steamapps\libraryfolders.vdf)");
        if (!file) {
            Logger::instance().error("Could not open libraryfolders.vdf");
            return games;
        }

        tyti::vdf::object root;
        try {
            root = tyti::vdf::read(file);
        } catch (const std::exception& e) {
            Logger::instance().error(std::string("Failed to parse libraryfolders.vdf: ") + e.what());
            return games;
        }

        Logger::instance().info(std::string("Root name: ") + root.name);

        // helper: attempt to get "path" from a library object (robust for different VDF shapes)
        auto get_library_path = [](const tyti::vdf::object& lib) -> std::optional<std::filesystem::path> {
            // common case: path is an attribute
            auto it = lib.attribs.find("path");
            if (it != lib.attribs.end()) return std::filesystem::path(it->second);

            // fallback: path might be represented as a child with an attrib
            auto cit = lib.childs.find("path");
            if (cit != lib.childs.end()) {
                const auto& child = *cit->second;
                if (!child.attribs.empty()) return std::filesystem::path(child.attribs.begin()->second);
            }

            return std::nullopt;
        };

        // 2. Iteruj po wszystkich bibliotekach Steam (root.childs contains "0", "1", ...)
        for (const auto& [id, lib_ptr] : root.childs) {
            const auto& lib = *lib_ptr;

            // Ścieżka do biblioteki (robust)
            auto opt_path = get_library_path(lib);
            if (!opt_path.has_value()) {
                Logger::instance().error(std::string("No path for library id: ") + id);
                continue;
            }

            std::filesystem::path steamapps = *opt_path;
            steamapps /= "steamapps";

            if (!std::filesystem::exists(steamapps)) continue;

            Logger::instance().info(std::string("Steam library found: ") + steamapps.string());

            // 3. Iteruj po plikach appmanifest.acf
            for (const auto& entry : std::filesystem::directory_iterator(steamapps)) {
                if (!entry.is_regular_file()) continue;
                std::string filename = entry.path().filename().string();
                if (filename.rfind("appmanifest_", 0) != 0 || entry.path().extension() != ".acf")
                    continue;

                std::ifstream manifest(entry.path());
                if (!manifest) continue;

                tyti::vdf::object app;
                try {
                    app = tyti::vdf::read(manifest);
                } catch (...) {
                    Logger::instance().error(std::string("Failed to parse ") + filename);
                    continue;
                }

                // 4. AppState może być root'em (app.name == "AppState") albo child'em
                const tyti::vdf::object* state = nullptr;
                if (app.name == "AppState") {
                    state = &app;
                } else {
                    auto st_it = app.childs.find("AppState");
                    if (st_it != app.childs.end()) state = st_it->second.get();
                }

                if (!state) {
                    Logger::instance().error(std::string("No AppState in appmanifest: ") + filename);
                    continue;
                }

                // 5. Pobierz name, installdir i appid z atrybutów state (używamy bezpośrednio attribs)
                auto name_it = state->attribs.find("name");
                auto installdir_it = state->attribs.find("installdir");
                auto appid_it = state->attribs.find("appid");

                if (name_it == state->attribs.end() ||
                    installdir_it == state->attribs.end() ||
                    appid_it == state->attribs.end()) {
                    Logger::instance().error(std::string("Missing attribute in appmanifest: ") + filename);
                    continue;
                }

                std::string name = name_it->second;
                std::string appid = appid_it->second;

                if(name == "Steamworks Common Redistributables"){
                    continue;
                }

                // 6. Dodajemy do wektora gier
                games.emplace_back(
                    name,
                    Game::STEAM,
                    std::filesystem::path("steam://run/" + appid),
                    std::stoi(appid)
                );
            }
        }

        Logger::instance().info(std::string("Total Steam games found: ") + std::to_string(games.size()));
        return games;
    }
};

} // namespace MultiLauncher