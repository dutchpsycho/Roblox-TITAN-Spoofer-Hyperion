#include "../Header/SigFuck.hpp"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>

#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>

void WindowName() {
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL) {
        std::string randomName = std::to_string(rand());
        SetWindowTextA(hwnd, randomName.c_str());
    }
}

void SigFucker() {
    srand(static_cast<unsigned int>(time(0)));
    WindowName();
}