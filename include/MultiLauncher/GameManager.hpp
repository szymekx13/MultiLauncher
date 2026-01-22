#pragma once
#include "IScanner.hpp"
#include <vector>
#include "Game.hpp"
#include <memory>
#include "Logger.hpp"
#include <mutex>

namespace MultiLauncher{
    class GameManager{
        public:
            void addScanner(std::unique_ptr<IScanner> scanner){
                scanners.push_back(std::move(scanner));
            }
            void update(){
                std::lock_guard<std::mutex> lock(gamesMutex);
                for(auto& game : games){
                    game->updateStatus();
                }
            }
            void scanAll(){
                for(auto& scanner : scanners){
                    try{
                        auto found = scanner->scan();
                        std::lock_guard<std::mutex> lock(gamesMutex);
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
            // const access - Note: GUI should ideally use a copy or lock if games can be added mid-frame
            const std::vector<std::unique_ptr<Game> >& getGames() const {
                return games;
            }
            
            std::unique_lock<std::mutex> lockGames() const {
                return std::unique_lock<std::mutex>(gamesMutex);
            }
        private:
            std::vector<std::unique_ptr<IScanner> > scanners;
            std::vector<std::unique_ptr<Game> >games;
            mutable std::mutex gamesMutex;
    };
}