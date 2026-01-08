#include "../include/MultiLauncher/GameManager.hpp"
#include "../include/MultiLauncher/SteamScanner.hpp"
#include "../include/MultiLauncher/EpicScanner.hpp"
#include "../include/MultiLauncher/Gui.hpp"
#include "../include/external/imgui/imgui.h"
#include <memory>
#include <cctype>
#include <iostream>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <limits>
#ifdef _WIN32
    #include <windows.h>
    #include <tlhelp32.h>
#endif

using namespace std;

string toLower(string s);
bool processExists(const string& exeName);


int main(){
    MultiLauncher::GameManager manager;

    manager.addScanner(make_unique<MultiLauncher::SteamScanner>());
    manager.addScanner(make_unique<MultiLauncher::EpicScanner>());
    //manager.addScanner(make_unique<MultiLauncher::GogScanner>());

    manager.scanAll();

    const auto& games_c = manager.getGames();
    auto games = manager.getGames(); // copy to sort
    for(int i = 0; i < games.size(); i++){
        if(games[i].getName() == "Steamworks Common Redistributables"){
            games.erase(games.end() - (games.size() - i));
        }
    }

    //Sorting by name
    sort(games.begin(), games.end(), [](const MultiLauncher::Game& a, const MultiLauncher::Game& b){
        return toLower(a.getName()) < toLower(b.getName());
    });

    int i = 0;
    
    auto games_sortByLauncher = games;

    //Sorting by launcher
    sort(games_sortByLauncher.begin(), games_sortByLauncher.end(),
        [](const MultiLauncher::Game& a, const MultiLauncher::Game& b){
            // 1️ launcher first
            if (toLower(a.getLauncher()) != toLower(b.getLauncher()))
                return toLower(a.getLauncher()) < toLower(b.getLauncher());

            // 2️ if launcher is the same, sort by name
            return toLower(a.getName()) < toLower(b.getName());
        }
    );
    // filtering by launcher name
    
    cout << "Vector games size: " << games.size() << endl;

    for(auto& game : games_sortByLauncher){
        i++;
        cout << i <<" game: " << setw(20) << game.getName() << " from launcher: "
                    << setw(10) << game.getLauncher() << " at path: " << setw(10) << game.getPath() << endl;
    }
    MultiLauncher::Gui gui;

    while(true){
        cout << "Enter a game number to launch or 0 to exit: ";
        int choice;
        if(!(cin >> choice)){
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input. Try again." << endl;
            continue;
        }
        if(choice == 0){
            break;
        }

        if(choice >= 1){
            if(choice > static_cast<int>(games_sortByLauncher.size())){
                cout << "Invalid choice. Try again." << endl;
                continue;
            }
            try{
                auto &g = games_sortByLauncher[choice - 1];
                g.launch(); 
                cout << "Launching " << g.getName() << "..." << endl;
                while(true){
                    if(processExists(g.getExeName())){
                        cout << "Launched: " << g.getName() << endl;
                        break;
                    }
                    this_thread::sleep_for(chrono::milliseconds(500));
                }
            }catch(const std::exception& e){  
                cout << "Failed to launch game: " << e.what() << endl;
            }
        }else{
            cout << "Invalid choice. Try again." << endl;
        }
    }

    return 0;
}

string toLower(string s){
    transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c){ return tolower(c); });
    return s;
}
bool processExists(const string& exeName){
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    if(snapshot == INVALID_HANDLE_VALUE){
        return false;
    }
    PROCESSENTRY32 processEntry {};
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    if(Process32First(snapshot, &processEntry)){
        do{
#ifdef UNICODE
            // convert exeName (UTF-8/ANSI) to wide string and compare
            int wlen = MultiByteToWideChar(CP_UTF8, 0, exeName.c_str(), -1, nullptr, 0);
            if(wlen > 0){
                std::vector<wchar_t> wbuf(wlen);
                MultiByteToWideChar(CP_UTF8, 0, exeName.c_str(), -1, wbuf.data(), wlen);
                if(_wcsicmp(processEntry.szExeFile, wbuf.data()) == 0){
                    CloseHandle(snapshot);
                    return true;
                }
            }
#else
            if(_stricmp(processEntry.szExeFile, exeName.c_str()) == 0){
                CloseHandle(snapshot);
                return true;
            }
#endif
        }while(Process32Next(snapshot, &processEntry));    
    }
    CloseHandle(snapshot);
    return false;
}