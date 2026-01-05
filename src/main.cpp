#include "../include/GameManager.hpp"
#include "../include/SteamScanner.hpp"
#include "../include/EpicScanner.hpp"
#include <iostream>
#include <algorithm>
#include <iomanip>

int main(){
    MultiLauncher::GameManager manager;

    manager.addScanner(std::make_unique<MultiLauncher::SteamScanner>());
    manager.addScanner(std::make_unique<MultiLauncher::EpicScanner>());
    //manager.addScanner(std::make_unique<MultiLauncher::GogScanner>());

    manager.scanAll();

    const auto& games = manager.getGames();
    sort(games.begin(), games.end());

    int licznik = 0;
    for(auto& game : games){
        licznik++;
        std::cout << licznik <<" game: " << std::setw(20) << game.getName() << " from lancher: "
                    << std::setw(10) <<game.getLauncher() << " at path: " << std::setw(10) <<game.getPath() << std::endl;
    }
}