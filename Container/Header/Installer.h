#ifndef INSTALLER_H
#define INSTALLER_H

#include <Windows.h>
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>

class Installer {

public:
    static void run();
    void Initialize();
    std::string getUserProfile();
    std::string getDrive();
    void openFile(const std::string& filePath);
    void None();
    void closeWindowByTitle(const std::wstring& title);
    void monrbx(std::atomic<bool>& terminated);

private:
    bool BloxstrapExists(const std::string& path);
    bool InstallerExists(const std::string& path);
};

#endif
