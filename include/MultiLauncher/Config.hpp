#pragma once
#include <filesystem>


namespace MultiLauncher{
    class Config{
        public:
            static std::filesystem::path getSettingsPath() {
                const char* appData = std::getenv("APPDATA");
                return std::filesystem::path(appData) / "MultiLauncher" / "settings.json";
            }
    };
}