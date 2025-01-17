#include "../Header/FsCleaner.h"
#include "../Services/Services.hpp"

namespace fs = std::filesystem;

void FsCleaner::RmvReferents(const std::filesystem::path& filePath, const std::wstring& itemClass) {
    if (!fs::exists(filePath)) {
        throw std::runtime_error("File does not exist: " + filePath.string());
    }

    std::wifstream inFile(filePath);
    if (!inFile.is_open()) {
        throw std::runtime_error("Failed to open file: " + filePath.string());
    }

    std::wstringstream buffer;
    buffer << inFile.rdbuf();
    inFile.close();
    std::wstring content = buffer.str();

    std::wregex referRegex(LR"(<Item class=\")" + itemClass + LR"(" referent=\"[^\"]+\">)");

    std::wstring replacement = L"<Item class=\"" + itemClass + L"\" referent=\"" + Services::genRand() + L"\">";
    content = std::regex_replace(content, referRegex, replacement);

    std::wofstream outFile(filePath);
    if (!outFile.is_open()) {
        throw std::runtime_error("Failed to write to file -> " + filePath.string());
    }
    outFile << content;
    outFile.close();
}

void FsCleaner::CleanRbx() {
    std::wstring systemDrive = Services::GetSysDrive();
    std::wstring userProfile = Services::GetUser();

    fs::path rbxSPath = fs::path(systemDrive) / L"ProgramData/Microsoft/Windows/Start Menu/Programs/Roblox/Roblox Player.lnk";
    std::wstring rbxTarget = Services::ResolveTarget(rbxSPath);
    fs::path rbxDir = fs::path(rbxTarget).parent_path().parent_path();
    for (const auto& entry : fs::directory_iterator(rbxDir)) {
        if (entry.is_directory() && entry.path().filename().wstring().find(L"version-") == 0) {
            Services::BulkDelete(entry.path(), { L"RobloxPlayerBeta.exe", L"RobloxPlayerBeta.dll", L"RobloxCrashHandler.exe", L"RobloxPlayerLauncher.exe" });
        }
    }

    fs::path bloxstrapSPath = fs::path(userProfile) / L"AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Bloxstrap.lnk";
    if (fs::exists(bloxstrapSPath)) {
        std::wstring bloxstrapTarget = Services::ResolveTarget(bloxstrapSPath);
        fs::path bloxstrapDir = fs::path(bloxstrapTarget).parent_path();
        for (const auto& entry : fs::directory_iterator(bloxstrapDir / L"Versions")) {
            if (entry.is_directory() && entry.path().filename().wstring().find(L"version-") == 0) {
                Services::BulkDelete(entry.path(), { L"RobloxPlayerBeta.exe", L"RobloxPlayerBeta.dll", L"RobloxCrashHandler.exe", L"RobloxPlayerLauncher.exe" });
            }
        }
    }

    fs::path tempDir = fs::path(userProfile) / L"AppData/Local/Temp/Roblox";
    fs::remove_all(tempDir);
    std::wcout << L"Deleted -> " << tempDir << std::endl;

    fs::path robloxLogsDir = fs::path(userProfile) / L"AppData/Local/Roblox";
    for (const auto& subDir : { L"logs", L"LocalStorage", L"Downloads" }) {
        fs::path fullPath = robloxLogsDir / subDir;
        fs::remove_all(fullPath);
        std::wcout << L"Deleted -> " << fullPath << std::endl;
    }

    RmvReferents(robloxLogsDir / L"AnalysticsSettings.xml", L"GoogleAnalyticsConfiguration");
    RmvReferents(robloxLogsDir / L"GlobalBasicSettings_13.xml", L"UserGameSettings");

    std::cout << "Replaced Global & Analytic Referents" << std::endl;
}

void FsCleaner::run() {
    Services::SectHeader("File System Cleaning", 202);
    try {
        CleanRbx();
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << '\n';
    }
}

void FsCleaner::Install() {
    Services::SectHeader("Roblox Installation", 203);

    std::wstring userProfile = Services::GetUser();
    std::wstring bloxstrapPath = fs::path(userProfile) / L"AppData/Local/Bloxstrap/Bloxstrap.exe";
    std::wstring rbxPath = fs::path(Services::GetSysDrive()) / L"Program Files (x86)/Roblox/Versions/RobloxPlayerInstaller.exe";

    std::atomic<bool> stopMon(false);

    auto monRbx = [&stopMon]() {
        while (!stopMon) {
            HWND robloxWindow = FindWindowW(nullptr, L"Roblox");
            if (robloxWindow) {
                DWORD processId = 0;
                GetWindowThreadProcessId(robloxWindow, &processId);

                if (processId != 0) {
                    HANDLE processHandle = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
                    if (processHandle) {
                        TerminateProcess(processHandle, 0);
                        CloseHandle(processHandle);
                        stopMon = true;
                        return;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        };

    if (fs::exists(bloxstrapPath)) {

        ShellExecuteW(nullptr, L"open", bloxstrapPath.c_str(), L"-player", nullptr, SW_HIDE);

        std::this_thread::sleep_for(std::chrono::seconds(3));

        std::thread monThread(monRbx);

        monThread.join();
        std::wcout << L"Reinstalled via Bloxstrap.\n";

    }

    else if (fs::exists(rbxPath)) {

        ShellExecuteW(nullptr, L"open", rbxPath.c_str(), nullptr, nullptr, SW_HIDE);

        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::wcout << L"Roblox installer executed.\n";

    }

    else {
        throw std::runtime_error("Neither Bloxstrap nor Roblox installer found..?");
    }

    stopMon = true;
}