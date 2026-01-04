#pragma once
#include <vector>
#include "Game.hpp"

namespace MultiLauncher{
    class IScanner{
        public:
            virtual std::vector<Game> scan() = 0;
            virtual ~IScanner() = default;
    };
}