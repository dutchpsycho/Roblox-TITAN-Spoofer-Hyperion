#ifndef INSTALLER_H
#define INSTALLER_H

#include <Windows.h>

#include <string>
#include <filesystem>

class Installer {
public:
    static void run();
    void Initialize();
    std::string getUserProfile();
    std::string getDrive();
    void openFile(const std::string& filePath);
    void None();

private:
    bool BloxstrapExists(const std::string& path);
    bool InstallerExists(const std::string& path);
};

#endif