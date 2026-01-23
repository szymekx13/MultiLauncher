# MultiLauncher
Cross-platform game launcher written in C++ using Dear ImGui.
Supports Epic Games downloads via community tools.

### Features

- Epic Games Store downloads (via community tool)
- ImGui based GUI
- Windows Support
- Linux Support (WIP)

## Building

### Prerequisites

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get install build-essential cmake pkg-config libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev
```

**Windows:**
- Visual Studio with C++ Desktop Development (includes CMake)

### Build with CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```


# Dependencies

- Dear ImGui
- OpenGL (Linux)
- DirectX 11 (Windows)
- Legendary (Epic games CLI)

### Disclaimer

This project is not affiliated with Epic Games.
Epic Games is trademark of Epic Games Inc.