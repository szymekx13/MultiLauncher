#include "IScanner.hpp"

namespace MultiLauncher{
    class GogScanner : public IScanner{
        public:
            std::vector<Game> scan() override {
                std::vector<Game> games;
                // TODO:
                // scanning folder GOG Galaxy\Games,
                // each game have folder in wich is .exe file
            }
    };
} // namespace MultiLauncher