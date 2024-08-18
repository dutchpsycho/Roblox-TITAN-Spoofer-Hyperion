#include "./Header/SigFuck.hpp"

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
        std::string randomName = "titan_" + std::to_string(rand());
        SetWindowTextA(hwnd, randomName.c_str());
    }
}

void ExcSig() {
    char filePath[MAX_PATH];
    GetModuleFileNameA(NULL, filePath, MAX_PATH);

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for reading: " << filePath << std::endl;
        return;
    }

    std::vector<char> buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    if (!buffer.empty()) {
        size_t modifyIndex = buffer.size() / 2;
        buffer[modifyIndex] ^= static_cast<char>(rand() % 256);
        std::cout << "Modified file byte at index " << modifyIndex << " to spoof hash." << std::endl;
    }

    std::ofstream outFile(filePath, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open file for writing: " << filePath << std::endl;
        return;
    }
    outFile.write(buffer.data(), buffer.size());
    outFile.close();
    std::cout << "Executable hash spoofed successfully." << std::endl;
}

void SigFucker() {
    srand(static_cast<unsigned int>(time(0)));
    WindowName();
    ExcSig();
}