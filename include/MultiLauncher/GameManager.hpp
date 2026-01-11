#pragma once
#include "IScanner.hpp"
#include <vector>
#include "Game.hpp"
#include <memory>
#include "Logger.hpp"

namespace MultiLauncher{
    class GameManager{
        public:
            void addScanner(std::unique_ptr<IScanner> scanner){
                scanners.push_back(std::move(scanner));
            }
            void scanAll(){
                for(auto& scanner : scanners){
                    try{
                        auto found = scanner->scan();
                        for(auto& g : found){
                            games.emplace_back(std::make_unique<Game>(std::move(g)));
                        }
                    }catch(const std::exception& e){
                        Logger::instance().error(std::string("Scanner error: ") + e.what());
                    }
                }
            }

            // mutable access
            std::vector<std::unique_ptr<Game> >& getGames() {
                return games;
            }
            // const access
            const std::vector<std::unique_ptr<Game> >& getGames() const {
                return games;
            }
        private:
            std::vector<std::unique_ptr<IScanner> > scanners;
            std::vector<std::unique_ptr<Game> >games;
    };
}