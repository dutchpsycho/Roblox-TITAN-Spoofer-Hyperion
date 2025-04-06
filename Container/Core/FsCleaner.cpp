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

    std::vector<fs::path> startMenuPaths = {
        fs::path(systemDrive) / L"ProgramData/Microsoft/Windows/Start Menu/Programs/Roblox/Roblox Player.lnk",
        fs::path(userProfile) / L"AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Roblox/Roblox Player.lnk"
    };

    for (const auto& rbxSPath : startMenuPaths) {
        if (fs::exists(rbxSPath)) {
            std::wstring rbxTarget = Services::ResolveTarget(rbxSPath);
            fs::path rbxDir = fs::path(rbxTarget).parent_path().parent_path();
            for (const auto& entry : fs::directory_iterator(rbxDir)) {
                if (entry.is_directory() && entry.path().filename().wstring().find(L"version-") == 0) {
                    Services::BulkDelete(entry.path(), { L"RobloxPlayerBeta.exe", L"RobloxPlayerBeta.dll", L"RobloxCrashHandler.exe", L"RobloxPlayerLauncher.exe" });
                }
            }
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

    try {
        std::wstring userProfile = Services::GetUser();
        std::wstring systemDrive = Services::GetSysDrive();

        std::wstring bloxstrapPath = fs::path(userProfile) / L"AppData/Local/Bloxstrap/Bloxstrap.exe";
        std::wstring fishstrapPath = fs::path(userProfile) / L"AppData/Local/Fishstrap/Fishstrap.exe";
        std::wstring rbxPath = fs::path(systemDrive) / L"Program Files (x86)/Roblox/Versions/RobloxPlayerInstaller.exe";

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

        auto tryLnk = [](const std::wstring& name) -> std::optional<std::wstring> {
            std::vector<fs::path> lnkPaths = {
                L"C:\\ProgramData\\Microsoft\\Windows\\Start Menu\\Programs\\" + name,
                Services::GetUser() + L"\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\" + name
            };

            for (const auto& path : lnkPaths) {
                if (fs::exists(path)) {
                    std::wstring resolved = Services::ResolveTarget(path);
                    if (!resolved.empty() && fs::exists(resolved)) {
                        return resolved;
                    }
                }
            }

            return std::nullopt;
            };

        std::optional<std::wstring> exeToRun;

        if (fs::exists(bloxstrapPath)) {
            exeToRun = bloxstrapPath;
        }

        else if (auto fishstrapLnk = tryLnk(L"Fishstrap.lnk"); fishstrapLnk.has_value()) {
            exeToRun = fishstrapLnk.value();
        }

        else if (fs::exists(fishstrapPath)) {
            exeToRun = fishstrapPath;
        }

        if (exeToRun.has_value() && !exeToRun->empty()) {
            ShellExecuteW(nullptr, L"open", exeToRun->c_str(), L"-player", nullptr, SW_HIDE);
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::thread monThread(monRbx);
            monThread.join();
            std::wcout << L"Reinstalled via: " << *exeToRun << L"\n";
        }

        else if (fs::exists(rbxPath)) {
            ShellExecuteW(nullptr, L"open", rbxPath.c_str(), nullptr, nullptr, SW_HIDE);
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::wcout << L"Roblox installer executed.\n";
        }

        else {
            std::wcerr << L"[!] No installer found: Bloxstrap, Fishstrap, or Roblox.\n";
        }

        stopMon = true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Install() Exception: " << ex.what() << "\n";

    }
}