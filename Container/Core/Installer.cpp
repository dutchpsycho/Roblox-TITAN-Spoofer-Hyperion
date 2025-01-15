#include "../Header/Installer.h"
#include "../Services/Services.hpp"

namespace fs = std::filesystem;

void Installer::run() {
    Services::SectHeader("Roblox Re-Install", 104);
    Installer installer;
    installer.Initialize();
}

void Installer::Initialize() {
    const std::vector<std::string> drives = getAllDrives();

    if (drives.empty()) {
        None();
        return;
    }

    const std::string user = getUserProfile();
    if (user.empty()) {
        None();
        return;
    }

    bool actionTaken = false;

    for (const auto& drive : drives) {
        const std::string path = drive + "\\Users\\" + user;
        const std::string blxpath = path + "\\AppData\\Local\\Bloxstrap";
        const std::string downloads = path + "\\Downloads";

        if (BloxstrapExists(blxpath)) {
            std::cout << "Found Bloxstrap on " << drive << ", using that to reinstall..." << std::endl;
            openFile(blxpath + "\\Bloxstrap.exe");
            actionTaken = true;
            continue;
        }

        if (InstallerExists(downloads)) {
            std::cout << "Found Roblox Installer on " << drive << ", using that to reinstall..." << std::endl;
            openFile(downloads + "\\RobloxPlayerInstaller.exe");
            actionTaken = true;
            continue;
        }
    }

    if (!actionTaken) {
        None();
    }
}

bool Installer::BloxstrapExists(const std::string& path) {
    return fs::exists(path);
}

bool Installer::InstallerExists(const std::string& path) {
    const std::string installerPath = path + "\\RobloxPlayerInstaller.exe";
    return fs::exists(installerPath);
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

std::vector<std::string> Installer::getAllDrives() {
    std::vector<std::string> drives;
    char driveStrings[256];
    DWORD length = GetLogicalDriveStringsA(sizeof(driveStrings), driveStrings);

    if (length > 0) {
        char* drive = driveStrings;
        while (*drive) {
            if (GetDriveTypeA(drive) == DRIVE_FIXED) {
                drives.emplace_back(std::string(drive).substr(0, 2));
            }
            drive += strlen(drive) + 1;
        }
    }
    return drives;
}

// idk how to open a file w args thru shell so deal w it
void Installer::openFile(const std::string& filePath) {
    if (!fs::exists(filePath)) {
        std::cout << "Couldn't find file: " << filePath << std::endl;
        return;
    }

    std::cout << "Opening file: " << filePath << std::endl;

    std::string command = "\"" + filePath + "\"";
    if (filePath.find("Bloxstrap.exe") != std::string::npos) {
        command += " -player"; // yay auto install
    }

    STARTUPINFOA si = { 0 };
    PROCESS_INFORMATION pi = { 0 };
    si.cb = sizeof(si);

    std::string cmdCommand = "cmd.exe /c " + command;

    if (CreateProcessA(NULL, (LPSTR)cmdCommand.c_str(), NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {

        std::this_thread::sleep_for(std::chrono::seconds(1));
        TerminateProcess(pi.hProcess, 0);

        std::this_thread::sleep_for(std::chrono::seconds(5));

        std::atomic<bool> terminated(false);
        std::thread sniper(&Installer::monrbx, this, std::ref(terminated));

        sniper.join();

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// idk if this works on RobloxInstaller, but it does on Bloxstrap
void Installer::monrbx(std::atomic<bool>& terminated) {
    int attempts = 0;

    while (!terminated && attempts < 20) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        HWND hwnd = FindWindowW(NULL, L"Roblox");
        if (hwnd != NULL) {
            std::cout << "Roblox installed, terminating..." << std::endl;
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            terminated = true;
        }
        attempts++;
    }
}