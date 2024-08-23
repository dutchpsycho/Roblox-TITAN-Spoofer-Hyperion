#include "../Header/CacheClear.hpp"
#include <windows.h>
#include <iostream>
#include <string>
#include <tlhelp32.h>
#include <shlobj.h>
#include <vector>
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

struct sqlite3;
#define SQLITE_OK 0

typedef int (*sqlite3_open_ptr)(const char*, sqlite3**);
typedef int (*sqlite3_close_ptr)(sqlite3*);
typedef const char* (*sqlite3_errmsg_ptr)(sqlite3*);
typedef int (*sqlite3_exec_ptr)(sqlite3*, const char*, int(*)(void*, int, char**, char**), void*, char**);
typedef void (*sqlite3_free_ptr)(void*);

sqlite3_open_ptr sqlite3_open = nullptr;
sqlite3_close_ptr sqlite3_close = nullptr;
sqlite3_errmsg_ptr sqlite3_errmsg = nullptr;
sqlite3_exec_ptr sqlite3_exec = nullptr;
sqlite3_free_ptr sqlite3_free = nullptr;

bool LoadSQLiteFunctions() {
    HMODULE hDLL = LoadLibraryA("SQL3.dll");
    if (!hDLL) {
        std::cerr << "Failed to load SQLite DLL" << std::endl;
        return false;
    }

    sqlite3_open = (sqlite3_open_ptr)GetProcAddress(hDLL, "sqlite3_open");
    sqlite3_close = (sqlite3_close_ptr)GetProcAddress(hDLL, "sqlite3_close");
    sqlite3_errmsg = (sqlite3_errmsg_ptr)GetProcAddress(hDLL, "sqlite3_errmsg");
    sqlite3_exec = (sqlite3_exec_ptr)GetProcAddress(hDLL, "sqlite3_exec");
    sqlite3_free = (sqlite3_free_ptr)GetProcAddress(hDLL, "sqlite3_free");

    if (!sqlite3_open || !sqlite3_close || !sqlite3_errmsg || !sqlite3_exec || !sqlite3_free) {
        std::cerr << "Failed to get SQLite function addresses" << std::endl;
        FreeLibrary(hDLL);
        return false;
    }

    return true;
}

std::vector<std::string> popularBrowsers = {
    "chrome.exe",
    "msedge.exe",
    "firefox.exe",
    "opera.exe",
    "brave.exe",
    "vivaldi.exe",
    "safari.exe",
    "avastbrowser.exe",
    "torch.exe",
    "slimbrowser.exe",
    "maxthon.exe",
    "netscape.exe",
    "seamonkey.exe",
    "comodo_dragon.exe",
    "waterfox.exe"
};

std::map<std::string, std::vector<std::string>> browserCookiePaths = {
    {"chrome.exe", {"%LOCALAPPDATA%\\Google\\Chrome\\User Data"}},
    {"msedge.exe", {"%LOCALAPPDATA%\\Microsoft\\Edge\\User Data"}},
    {"firefox.exe", {"%APPDATA%\\Mozilla\\Firefox\\Profiles"}},
    {"opera.exe", {"%APPDATA%\\Opera Software\\Opera Stable"}},
    {"brave.exe", {"%LOCALAPPDATA%\\BraveSoftware\\Brave-Browser\\User Data"}},
    {"vivaldi.exe", {"%LOCALAPPDATA%\\Vivaldi\\User Data"}},
    {"safari.exe", {"%APPDATA%\\Apple Computer\\Safari"}},
    {"avastbrowser.exe", {"%LOCALAPPDATA%\\AVAST Software\\Browser\\User Data"}},
    {"torch.exe", {"%LOCALAPPDATA%\\Torch\\User Data"}},
    {"waterfox.exe", {"%APPDATA%\\Waterfox\\Profiles"}},
};

bool ClearCookiesFromDb(const std::string& cookieDbPath) {
    if (!LoadSQLiteFunctions()) {
        return false;
    }

    sqlite3* db;
    char* errMsg = nullptr;
    int rc = sqlite3_open(cookieDbPath.c_str(), &db);

    if (rc) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    std::string sql = "DELETE FROM cookies WHERE host_key LIKE '%roblox.com%';";

    rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "SQL err: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(db);
        return false;
    }

    sqlite3_close(db);
    return true;
}

void CacheClear() {
    std::cout << "\033[33m" << "\n========== CACHE CLEARING PROCESS ==========\n" << "\033[0m";

    HWND hwnd = GetForegroundWindow();
    char windowTitle[256];
    GetWindowTextA(hwnd, windowTitle, sizeof(windowTitle));

    std::string runningBrowser;

    for (const auto& browser : popularBrowsers) {
        if (std::string(windowTitle).find(browser.substr(0, browser.find('.'))) != std::string::npos) {
            runningBrowser = browser;
            break;
        }
    }

    if (runningBrowser.empty()) {
        HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            return;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hProcessSnap, &pe32)) {
            do {
                char szExeFileA[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, szExeFileA, MAX_PATH, NULL, NULL);

                for (const auto& browser : popularBrowsers) {
                    if (_stricmp(szExeFileA, browser.c_str()) == 0) {
                        runningBrowser = browser;
                        break;
                    }
                }
                if (!runningBrowser.empty()) break;
            } while (Process32Next(hProcessSnap, &pe32));
        }
        CloseHandle(hProcessSnap);
    }

    if (!runningBrowser.empty()) {
        std::cout << "Found -> " << runningBrowser << std::endl;
        std::cout << "This will be killed: " << runningBrowser << std::endl;

        HANDLE hProcessSnap;
        PROCESSENTRY32 pe32;
        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            return;
        }
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hProcessSnap, &pe32)) {
            do {
                char szExeFileA[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, szExeFileA, MAX_PATH, NULL, NULL);

                if (_stricmp(szExeFileA, runningBrowser.c_str()) == 0) {
                    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                    if (hProcess != NULL) {
                        TerminateProcess(hProcess, 0);
                        CloseHandle(hProcess);
                    }
                }
            } while (Process32Next(hProcessSnap, &pe32));
        }
        CloseHandle(hProcessSnap);

        std::vector<std::string> cookiePaths = browserCookiePaths[runningBrowser];
        for (const auto& path : cookiePaths) {
            char expandedPath[MAX_PATH];
            ExpandEnvironmentStringsA(path.c_str(), expandedPath, MAX_PATH);
            if (runningBrowser == "firefox.exe") {
                for (const auto& profileEntry : fs::directory_iterator(expandedPath)) {
                    if (profileEntry.is_directory()) {
                        for (const auto& entry : fs::recursive_directory_iterator(profileEntry.path().string())) {
                            if (entry.is_regular_file()) {
                                std::string cookiePath = entry.path().string();
                                if (cookiePath.find("Cookies") != std::string::npos || cookiePath.find("cookies.sqlite") != std::string::npos) {
                                    std::cout << "Clearing cookies & trackers for " << runningBrowser << " @ " << cookiePath << std::endl;
                                    ClearCookiesFromDb(cookiePath);
                                }
                            }
                        }
                    }
                }
            }
            else {
                for (const auto& entry : fs::recursive_directory_iterator(expandedPath)) {
                    if (entry.is_regular_file()) {
                        std::string cookiePath = entry.path().string();
                        if (cookiePath.find("Cookies") != std::string::npos || cookiePath.find("cookies.sqlite") != std::string::npos) {
                            std::cout << "Clearing cookies & trackers for " << runningBrowser << " @ " << cookiePath << std::endl;
                            ClearCookiesFromDb(cookiePath);
                        }
                    }
                }
            }
        }
    }
    else {
        std::cerr << "No supported browser is running." << std::endl;
    }
}