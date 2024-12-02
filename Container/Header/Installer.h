#ifndef INSTALLER_H
#define INSTALLER_H

#include <Windows.h>
#include <string>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>

class Installer {

public:
    static void run();
    void Initialize();

private:
    std::string getUserProfile();
    std::vector<std::string> getAllDrives();
    void openFile(const std::string& filePath);
    void None();
    void monrbx(std::atomic<bool>& terminated);

    bool BloxstrapExists(const std::string& path);
    bool InstallerExists(const std::string& path);
};

#endif