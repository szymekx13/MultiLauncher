#include "../include/GameManager.hpp"
#include "../include/SteamScanner.hpp"
#include "../include/EpicScanner.hpp"
#include <iostream>
#include <iomanip>


int main(){
    MultiLauncher::GameManager manager;

    manager.addScanner(std::make_unique<MultiLauncher::SteamScanner>());
    manager.addScanner(std::make_unique<MultiLauncher::EpicScanner>());
    //manager.addScanner(std::make_unique<MultiLauncher::GogScanner>());

    manager.scanAll();

    const auto& games = manager.getGames();
    for(auto& game : games){
        std::cout << std::setw(30) <<"Found game: " << game.getName() << " from Lancher: "
                    << game.getLauncher() << " at path: " << game.getPath() << std::endl;
    }
}