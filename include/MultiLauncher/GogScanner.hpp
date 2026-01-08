#include "IScanner.hpp"

namespace MultiLauncher{
    class GogScanner : public IScanner{
        public:
            std::vector<Game> scan() override {
                std::vector<Game> games;
                // for simplicity we can scan the folder GOG Galaxy\Games
                // each game has its own folder with a .exe inside
                // so we can iterate through each folder, find the .exe and create a Game object
            }
    };
} // namespace MultiLauncher