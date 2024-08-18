/*
* @Author Damon (Swedish.Psych0)
* @License GNU Public Use
* @Namespace TITAN
* @Version 3.6.0
*/

#include "./Container/Header/Job.hpp"
#include "./Container/Header/Hyperion.hpp"
#include "./Container/Header/Roblox.hpp"
#include "./Container/Header/MAC.hpp"
#include "./Container/Header/CacheClear.hpp"
#include "./Container/Header/SigFuck.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#include <windows.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>

// #define HEADLESS

#ifndef HEADLESS
void Colour(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void TITAN() {
    std::string art = R"(
  /###           /       #####  # /###           /    ##            ##### #     ##    
 /  ############/     ######  /  /  ############/  /####         ######  /#    #### / 
/     #########      /#   /  /  /     #########   /  ###        /#   /  / ##    ###/  
#     /  #          /    /  /   #     /  #           /##       /    /  /  ##    # #   
 ##  /  ##              /  /     ##  /  ##          /  ##          /  /    ##   #     
    /  ###             ## ##        /  ###          /  ##         ## ##    ##   #     
   ##   ##             ## ##       ##   ##         /    ##        ## ##     ##  #     
   ##   ##           /### ##       ##   ##         /    ##        ## ##     ##  #     
   ##   ##          / ### ##       ##   ##        /      ##       ## ##      ## #     
   ##   ##             ## ##       ##   ##        /########       ## ##      ## #     
    ##  ##        ##   ## ##        ##  ##       /        ##      #  ##       ###     
     ## #      / ###   #  /          ## #      / #        ##         /        ###     
      ###     /   ###    /            ###     / /####      ##    /##/          ##     
       ######/     #####/              ######/ /   ####    ## / /  #####              
         ###         ###                 ###  /     ##      #/ /     ##               
                                              #                #                      
                                               ##               ##          
Made by HATEDAMON bitch
https://github.com/dutchpsycho/TITAN-Spoofer
Hyperion: 'New phone, who dis?'
TITAN Spoofer V3.6

)";
    std::string white = "\033[37m";
    std::string resetColor = "\033[0m";
    std::cout << white << art << resetColor << std::endl;
}

void Menu(int selected) {
    std::string options[] = {
        "Spoof",
        "Clear Cookies (Ban API)"
    };
    std::string subtext = "   [ This will log you out of all your accounts. Refer to antiban-guide in TITAN's Discord. ]";

    for (int i = 0; i < 2; ++i) {
        if (i == selected) {
            std::cout << "> " << options[i] << " <" << std::endl;
            if (i == 1) {
                std::cout << subtext << std::endl;
            }
        }
        else {
            std::cout << "  " << options[i] << std::endl;
        }
    }
}

void _ps(const std::string& message, int color) {
    Colour(color);
    std::cout << message << std::endl;
    Colour(7);
}

void ShowProgress(const std::string& step) {
    static const char* progressChars = "|/-\\";
    for (int i = 0; i < 4; ++i) {
        std::cout << "\r" << step << " " << progressChars[i] << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout << "\r" << step << " done!" << std::endl;
}
#endif

std::string generateRandomString(size_t length) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    std::string result;
    result.resize(length);

    for (size_t i = 0; i < length; i++) {
        result[i] = charset[rand() % (sizeof(charset) - 1)];
    }

    return result;
}

int main() {
    srand(static_cast<unsigned int>(time(nullptr)));
    std::string windowTitle = generateRandomString(24);
    SetConsoleTitleA(windowTitle.c_str());

    SigFucker();

#ifdef HEADLESS
    ClearRoblox();
    spoofMac();
    spoofHyperion();
#else
    TITAN();

    int choice = 0;
    while (true) {
        system("CLS");
        TITAN();
        Menu(choice);

        int key = _getch();
        if (key == 224) {
            key = _getch();
            switch (key) {
            case 72: // up
                choice = (choice == 0) ? 1 : choice - 1;
                break;
            case 80: // down
                choice = (choice == 1) ? 0 : choice + 1;
                break;
            default:
                break;
            }
        }
        else if (key == 13) { // enter
            switch (choice) {
            case 0:
                ClearRoblox();
                std::cout << "Clearing Roblox data... Done." << std::endl;

                spoofMac();
                std::cout << "Spoofing MAC addresses... Done." << std::endl;

                spoofHyperion();
                std::cout << "Spoofing Hyperion... Done." << std::endl;

                _ps("System spoofed.", 10);
                system("pause");
                break;
            case 1:
                ClearRoblox();
                CacheClear();
                _ps("Cookie cache cleared.", 10);
                system("pause");
                break;
            default:
                break;
            }
        }
    }
#endif
    return 0;
}