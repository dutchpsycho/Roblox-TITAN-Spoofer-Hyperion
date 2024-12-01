#include "../Header/Installer.h"
#include "../Services/Services.hpp"

namespace fs = std::filesystem;

void Installer::run() {
    Services::SectHeader("Roblox Re-Install", 104);
    Installer installer;
    installer.Initialize();
}

void Installer::Initialize() {
    const std::string user = getUserProfile();
    const std::string drive = getDrive();

    if (user.empty()) {
        None();
        return;
    }

    const std::string path = drive + "\\Users\\" + user;
    const std::string blxpath = path + "\\AppData\\Local\\Bloxstrap";
    const std::string downloads = path + "\\Downloads";

    if (BloxstrapExists(blxpath)) {
        std::cout << "Found Bloxstrap, using that to reinstall..." << std::endl;
        openFile(blxpath + "\\Bloxstrap.exe");
        return;
    }

    if (InstallerExists(downloads)) {
        std::cout << "Found Roblox Installer, using that to reinstall..." << std::endl;
        openFile(downloads + "\\RobloxPlayerInstaller.exe");
        return;
    }

    None();
}

bool Installer::BloxstrapExists(const std::string& path) {
    return fs::exists(path);
}

bool Installer::InstallerExists(const std::string& path) {
    const std::string installerPath = path + "\\RobloxPlayerInstaller.exe";
    return fs::exists(installerPath);
}

// idk how to open a file w args thru shell so deal w it
void Installer::openFile(const std::string& filePath) {
    if (fs::exists(filePath)) {
        std::cout << "Opening file: " << filePath << std::endl;
        std::string command;
        if (filePath.find("Bloxstrap.exe") != std::string::npos) {
            command = "\"" + filePath + "\" -player"; // invokes player directly
        }
        else {
            command = "\"" + filePath + "\"";
        }

        STARTUPINFOA si = { 0 };
        PROCESS_INFORMATION pi = { 0 };
        si.cb = sizeof(si);

        std::string cmdCommand = "cmd.exe /c " + command;

        if (CreateProcessA(NULL, (LPSTR)cmdCommand.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
            Sleep(1000);
            TerminateProcess(pi.hProcess, 0);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        return;
    }

    std::cout << "Couldn't find file: " << filePath << std::endl;
}

void Installer::None() {
    std::cout << "Install Bloxstrap for ease of use, or if you don't want to use Bloxstrap, place RobloxPlayerInstaller.exe in your Downloads folder." << std::endl;
    exit(1);
}

std::string Installer::getUserProfile() {
    char* user = nullptr;
    size_t len = 0;
    if (_dupenv_s(&user, &len, "USERPROFILE") == 0 && user != nullptr) {
        std::string userProfile(user);
        free(user);
        size_t pos = userProfile.find_last_of("\\");
        if (pos != std::string::npos) {
            return userProfile.substr(pos + 1);
        }
    }
    return "";
}

std::string Installer::getDrive() {
    char drive[4] = { 0 };
    if (GetEnvironmentVariableA("SystemDrive", drive, sizeof(drive)) == 0) {
        return "C:";
    }
    return std::string(drive);
}