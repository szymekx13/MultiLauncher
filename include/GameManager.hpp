#include "IScanner.hpp"
#include <vector>
#include "Game.hpp"
#include <memory>
#include <iostream>

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
                        games.insert(games.end(), found.begin(), found.end());
                    }catch(const std::exception& e){
                        std::cerr << "Scanner error: " << e.what() << std::endl;
                    }
                }
            }

            const std::vector<Game>& getGames() const{
                return games;
            }
        private:
            std::vector<std::unique_ptr<IScanner> > scanners;
            std::vector<Game> games;
    };
}