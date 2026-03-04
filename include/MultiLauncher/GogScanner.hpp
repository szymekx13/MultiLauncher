#pragma once
#include "IScanner.hpp"
#include <filesystem>
#include <string>
#include "../JSON/json.hpp"

using json = nlohmann::json;

namespace MultiLauncher{
    class GogScanner : public IScanner{
        public:
            std::vector<Game> scan() override {
                std::vector<Game> games;
                // TODO:
                // we have to scan C:Program Files (x86)\GOG Galaxy\Games\
                // and find goggame-XXXXXXX.info
#ifdef _WIN32
                std::filesystem::path manifestDir = R"(C:\Program Files (x86)\GOG Galaxy\Games)"
#else
                return games;  // there is no official GOG launcher for linux
#endif
                if(!manifestDir.exists()){
                    return games;
                }
                std::filesystem::path gogFile = "";
                for(const auto& entry : std::filesystem.directory_iterator(manifestDir)){
                    if(entry.path().extension() == '.info'){
                        std::string file = entry.to_string();
                        if(file.substr(0, 8) == "goggame-"){ //goggame-
                            Logger::instance().info("Found gog info file");
                            gogFile = entry;
                            break;
                        }
                    }
                }
                try{
                    auto j = json.parse(gogFile);
                    

                }catch(const std::exception& e){
                    Logger::instance().info("Failed to parse gog file");
                }

            }
    };
} // namespace MultiLauncher