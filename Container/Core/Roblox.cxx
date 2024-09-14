#include "../Header/Roblox.hxx"

#include "../Util/Utils.hxx"

#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <filesystem>
#include <vector>
#include <string>
#include <stack>
#include <iostream>
#include <regex>
#include <fstream>
#include <sstream>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

typedef struct _THREAD_START_INFORMATION {
    PVOID StartAddress;
} THREAD_START_INFORMATION, * PTHREAD_START_INFORMATION;

#ifndef ThreadQuerySetWin32StartAddress
#define ThreadQuerySetWin32StartAddress 9
#endif

typedef NTSTATUS(WINAPI* pNtQueryInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
typedef NTSTATUS(WINAPI* pNtFreezeThread)(HANDLE, PULONG);
namespace fs = std::filesystem;

bool Freeze(DWORD processId);
bool Terminate(const std::string& processName);
bool ScopeProcess(const std::string& processName);
bool ScopeWindow(const std::string& windowTitle);
void rmDir(const fs::path& path);
void rmFile(const fs::path& filePath);

std::string getSystemDrive() {
    char systemDrive[MAX_PATH];
    if (GetEnvironmentVariableA("SystemDrive", systemDrive, MAX_PATH) > 0) {
        return std::string(systemDrive);
    }
    return "C:";
}

std::string NewSets(const std::string& xmlContent) {
    std::string nXML = xmlContent;
    std::regex referRegex(R"(<Item class="UserGameSettings" referent="[^"]+">)");
    nXML = std::regex_replace(nXML, referRegex, R"(<Item class="UserGameSettings" referent="TITAN">)");
    return nXML;
}

bool CopySets(const fs::path& SetFPath) {
    if (!fs::exists(SetFPath)) {
        return false;
    }

    std::ifstream configFile(SetFPath);

    if (!configFile.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << configFile.rdbuf();
    std::string xmlContent = buffer.str();
    configFile.close();
    std::string nXML = NewSets(xmlContent);
    fs::remove(SetFPath);
    std::ofstream NewSetF(SetFPath);
    
    if (!NewSetF.is_open()) {
        return false;
    }

    NewSetF << nXML;
    NewSetF.close();

    return true;
}

bool ScopeProcess(const std::string& processName) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    bool processFound = false;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create process snapshot." << std::endl;
        return false;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (Process32First(hProcessSnap, &pe32)) {
        do {
            char exeFileA[MAX_PATH];
            WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, exeFileA, MAX_PATH, NULL, NULL);

            if (_stricmp(exeFileA, processName.c_str()) == 0) {
                processFound = true;
                break;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    else {
        std::cerr << "Failed to iterate through processes." << std::endl;
    }

    CloseHandle(hProcessSnap);
    return processFound;
}

bool ScopeWindow(const std::string& windowTitle) {
    HWND hwnd = FindWindowA(NULL, windowTitle.c_str());
    if (hwnd == NULL) {
        return false;
    }
    return true;
}

bool Freeze(DWORD processId) {
    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create thread snapshot." << std::endl;
        return false;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    pNtQueryInformationThread NtQueryInformationThread = nullptr;
    pNtFreezeThread NtFreezeThread = nullptr;

    if (Thread32First(hThreadSnap, &te32)) {
        do {
            if (te32.th32OwnerProcessID == processId) {
                HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION, FALSE, te32.th32ThreadID);
                if (hThread != NULL) {
                    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
                    if (hNtDll != NULL) {
                        NtQueryInformationThread =
                            (pNtQueryInformationThread)GetProcAddress(hNtDll, "NtQueryInformationThread");

                        NtFreezeThread =
                            (pNtFreezeThread)GetProcAddress(hNtDll, "NtFreezeThread");

                        if (NtQueryInformationThread && NtFreezeThread) {
                            THREAD_START_INFORMATION tsi;
                            NTSTATUS status = NtQueryInformationThread(hThread, (THREADINFOCLASS)ThreadQuerySetWin32StartAddress, &tsi, sizeof(tsi), NULL);
                            if (NT_SUCCESS(status) && tsi.StartAddress) {
                                MEMORY_BASIC_INFORMATION mbi;
                                if (VirtualQueryEx(GetCurrentProcess(), tsi.StartAddress, &mbi, sizeof(mbi))) {
                                    if (mbi.AllocationBase == hNtDll) {
                                        std::cout << "Freezing thread ID: " << te32.th32ThreadID << std::endl;
                                        NtFreezeThread(hThread, NULL);
                                    }
                                }
                            }
                        }
                    }
                    CloseHandle(hThread);
                }
                else {
                    std::cerr << "Failed to open thread for process ID: " << processId << std::endl;
                }
            }
        } while (Thread32Next(hThreadSnap, &te32));
    }
    else {
        std::cerr << "Failed to iterate through threads." << std::endl;
    }

    CloseHandle(hThreadSnap);
    return true;
}

bool Terminate(const std::string& processName) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create process snapshot." << std::endl;
        return false;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            char szExeFileA[MAX_PATH];
            WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, szExeFileA, MAX_PATH, NULL, NULL);

            if (_stricmp(szExeFileA, processName.c_str()) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess != NULL) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    CloseHandle(hProcessSnap);
    return true;
}

void ClearRoblox() {
    std::cout << "\033[38;2;0;0;255m" << "\n========== ROBLOX CLEARING PROCESS ==========\n" << "\033[0m";

    if (ScopeWindow("Roblox") && (ScopeProcess("RobloxPlayerBeta.exe") || ScopeProcess("Roblox Client"))) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create process snapshot." << std::endl;
            return;
        }

        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe32)) {
            do {
                char szExeFileA[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, szExeFileA, MAX_PATH, NULL, NULL);

                if (_stricmp(szExeFileA, "RobloxPlayerBeta.exe") == 0 || _stricmp(szExeFileA, "Roblox Client") == 0) {
                    std::cout << "Freezing process: " << szExeFileA << std::endl;
                    if (!Freeze(pe32.th32ProcessID)) {
                        std::cerr << "Failed to freeze process: " << szExeFileA << std::endl;
                    }
                }
            } while (Process32Next(hSnapshot, &pe32));
        }
        else {
            std::cerr << "Failed to iterate through processes." << std::endl;
        }
        CloseHandle(hSnapshot);

        Sleep(1000);

        if (!Terminate("RobloxPlayerBeta.exe")) {
            std::cerr << "Failed to terminate RobloxPlayerBeta.exe." << std::endl;
        }

        if (!Terminate("Roblox Client")) {
            std::cerr << "Failed to terminate Roblox Client." << std::endl;
        }
        Sleep(500);
    }

    char LocalAppDataP[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, LocalAppDataP))) {
        fs::path rbxP = fs::path(LocalAppDataP) / "Roblox";
        fs::path setFPath = rbxP / "GlobalBasicSettings_13.xml";

        std::string originalXMLContent;
        std::string mXML;

        if (fs::exists(setFPath)) {
            std::ifstream configFile(setFPath);
            if (configFile.is_open()) {
                std::stringstream buffer;
                buffer << configFile.rdbuf();
                originalXMLContent = buffer.str();
                configFile.close();

                mXML = NewSets(originalXMLContent);
            }
            else {
                std::cerr << "Failed to open Roblox's XML file: " << setFPath << std::endl;
            }
        }
        else {
            std::cerr << "Global XML not found, skipping rollover." << std::endl;
        }

        if (fs::exists(rbxP)) {
            std::cout << "Removing Roblox dir: " << rbxP << std::endl;
            rmDir(rbxP);

            if (!fs::exists(rbxP)) {
                fs::create_directories(rbxP);
            }

            if (!mXML.empty()) {
                std::ofstream newSetF(setFPath);
                if (newSetF.is_open()) {
                    newSetF << mXML;
                    newSetF.close();
                    std::cout << "Recreated the cleaned config file: " << setFPath << std::endl;
                }
                else {
                    std::cerr << "Failed to recreate the config file: " << setFPath << std::endl;
                }
            }
        }
        else {
            std::cerr << "Roblox dir not found, skipping." << std::endl;
        }
    }
    else {
        std::cerr << "Failed to get Local AppData path." << std::endl;
    }

    std::string systemDrive = getSystemDrive();
    fs::path crashHandlerPath = fs::path(systemDrive) / "Program Files (x86)" / "Roblox" / "Versions" / "version-3243b6d003cf4642" / "RobloxCrashHandler.exe";
    std::cout << "Removing Crash Handler: " << crashHandlerPath << std::endl;
    rmFile(crashHandlerPath);

    char tempPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempPath)) {
        fs::path tempDir(tempPath);
        fs::path robloxTempPath = tempDir / "Roblox";
        if (fs::exists(robloxTempPath)) {
            std::cout << "Clearing Roblox %TEMP%: " << robloxTempPath << std::endl;
            for (const auto& entry : fs::directory_iterator(robloxTempPath)) {
                if (entry.is_regular_file()) {
                    fs::remove(entry.path());
                }
            }
        }
    }
    else {
        std::cerr << "Failed to get %TEMP% path." << std::endl;
    }

    fs::path cTempPath = fs::path(systemDrive) / "Temp";
    if (fs::exists(cTempPath)) {
        fs::path robloxTempPath = cTempPath / "Roblox";
        if (fs::exists(robloxTempPath)) {
            std::cout << "Clearing: " << robloxTempPath << std::endl;
            for (const auto& entry : fs::directory_iterator(robloxTempPath)) {
                if (entry.is_regular_file()) {
                    fs::remove(entry.path());
                }
            }
        }
    }
}