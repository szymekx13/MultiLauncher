#include "IScanner.hpp"
#include "vdf_parser.hpp"
#include <sstream>
#include <iostream>
#include <vector>
#include <filesystem>

namespace MultiLauncher{
    class SteamScanner : public IScanner{
        public:
            std::vector<Game> scan() override {
                std::vector<Game> games;

                std::ifstream file(R"(C:\Program Files (x86)\Steam\steamapps\libraryfolders.vdf)");
                if (!file) {
                    std::cerr << "Could not open libraryfolders.vdf\n";
                    return games;
                }

                tyti::vdf::object root = tyti::vdf::read(file);
                std::cout << "Root name: " << root.name << std::endl;

                for (const auto& [id, lib] : root.childs) {
                    auto it = lib->attribs.find("path");
                    if (it == lib->attribs.end()) {
                        continue;
                    }

                    // get string value from the "path" node and construct filesystem path
                    std::filesystem::path libraryPath = it->second;
                    std::filesystem::path steamapps = libraryPath / "steamapps";

                    if (!std::filesystem::exists(steamapps))
                        continue;

                    std::cout << "Steam library: " << steamapps << std::endl;

                    for (const auto& entry : std::filesystem::directory_iterator(steamapps)) {
                        if (!entry.is_regular_file())
                            continue;

                        auto filename = entry.path().filename().string();
                        if (filename.rfind("appmanifest_", 0) != 0 || entry.path().extension() != ".acf")
                            continue;

                        std::ifstream manifest(entry.path());
                        if (!manifest)
                            continue;

                        tyti::vdf::object app = tyti::vdf::read(manifest);
                        const auto& state = *app.childs.at("AppState");

                        std::string name = state.attribs.at("name");
                        std::string installdir = state.attribs.at("installdir");

                        std::string appid = state.attribs.at("appid");

                        games.emplace_back(
                            name,
                            Game::STEAM, 
                            std::filesystem::path("steam://run/" + appid)
                        );
                    }
                }

                return games;
            }
    };
}