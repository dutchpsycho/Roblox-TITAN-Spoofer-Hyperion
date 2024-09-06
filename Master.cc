/*
* @Author Damon (Swedish.Psych0)
* @License TITAN © 2024 by Damon under CC BY-NC-ND 4.0
* @Namespace TITAN
* @Version 5.0.0
*/

#include "./Container/Header/Hyperion.hxx"
#include "./Container/Header/Fingerprint.hxx"
#include "./Container/Header/Roblox.hxx"
#include "./Container/Header/MAC.hxx"
#include "./Container/Header/CacheClear.hxx"

#include "./Container/Util/Elevate.hxx"
#include "./Container/Util/Utils.hxx"

#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#include <windows.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>

// If you want to use headless mode, you need to do a couple things.
// First, right click the project (right side, TITAN Spoofer)
// Second, click Linker dropdown & click advanced. change entry point to "MainCRTStartup"
// Third, click system and click the dropdown and switch it to "SUBSYSTEM::WINDOWS"
// Finally, uncomment the line below (Remove //)

// #define HEADLESS

#ifdef HEADLESS

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    srand(static_cast<unsigned int>(time(nullptr)));
    Elevate();
    SigFucker();
    ClearRoblox();
    spoofMac();
    spoofHyperion();
    fingerprint = SystemFingerprint::CreateUniqueFingerprint();
    return 0;
}

#else

void Colour(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void TITAN() {
    std::string art = R"(
                              dMb,
                            ,dMMMMb,          ,,
                         ,dMMMMMMMMMb, eeee8888"
                      ,mMMm!!!!XXXXMMMMM"""
                    ,d!!XXMMXX88888888W"
                   `MX88dMM8888WWWMMMMMMb,
                       '""MMMMMMMMMMMMMMMMb
                         MMMMMMMMMMMMMMMMMMb,
                        dMMMMMMMMMMMMMMMMMMMMb,,
            _,dMMMMMMMMMMXXXX!!!!!!!!!!!!!!XXXXXMP
       _,dMMXX!!!!!!!!!!!!!!!!!!XXXXX888888888WWC
   _,dMMX!!!MMMM!!!!!!!!XXXXXX888888888888WWMMMMMb,
  dMMX!!!!!MMM!XXXXXX88888888888888888WWMMMMMMMMMMMb
 dMMXXXXXX8MMMM88888888888888888WWWMMMMMMMMMMMMMMMMMb    ,d8
 MMMMWW888888MMMMM8888888WWMMMMMMMMMMMMMMMMMMMMMMMMMMM,d88P'
  YMMMMMWW888888WWMMMMMMMMMMP"""'    `"YMMMMMMMMMMMXMMMMMP
     `""YMMMMMMMMMMMMMP""'            mMMMm!XXXXX8888888e,
                                    ,d!!XXMM888888888888WW
Swedish.Psycho                     "MX88dMM888888WWWMMMMMMb
TITAN Spoofer V5.0                        """```"YMMMMMMMYMMM
Hyperion: 'New phone, who dis?                    `"YMMMMM
https://github.com/dutchpsycho/TITAN-Spoofer         `"YMP
    )";
    std::cout << art << std::endl;
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
        } else {
            std::cout << "  " << options[i] << std::endl;
        }
    }
}

void _ps(const std::string& message, int color) {
    Colour(color);
    std::cout << message << std::endl;
    Colour(7);
}

std::string rands(size_t length) {
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
    std::string windowTitle = rands(24);
    SetConsoleTitleA(windowTitle.c_str());

    SigFucker();
    TITAN();
    Elevate();

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
            std::unique_ptr<SystemFingerprint> fingerprint;
            switch (choice) {
            case 0: {
                ClearRoblox();
                spoofMac();
                spoofHyperion();
                fingerprint = SystemFingerprint::CreateUniqueFingerprint();
                std::cout << "Spoofed Fingerprint Successfully :: [ " << fingerprint->ToString() << " ]\n" << std::endl;
                _ps("Operation Finished Successfully, no restart required.", 10);
                system("pause");
                break;
            }
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
    return 0;
}

#endif