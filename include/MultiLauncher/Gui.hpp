#pragma once
#include "GameManager.hpp"
#include "../external/imgui/imgui.h"

namespace MultiLauncher{
    class Gui{
        public:
            void init();
            void render(GameManager& manager);
            void shutdown();
    };
}