#pragma once
#include "Gui.hpp"
#include "GameManager.hpp"

namespace MultiLauncher{
    class App{
        private:
            Gui gui;
        public:
            void run();
    };
} // namespace MultiLauncher