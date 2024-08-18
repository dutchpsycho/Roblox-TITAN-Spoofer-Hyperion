#include "../Header/Roblox.hpp"
#include "../Header/Job.hpp"

#include <windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <filesystem>
#include <vector>
#include <string>
#include <stack>
#include <iostream>

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

void ClearRoblox() {
    std::cout << "Starting Roblox clearing process..." << std::endl;

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
    else {
        std::cerr << "No Roblox window or process found to terminate." << std::endl;
    }

    char localAppDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath))) {
        fs::path robloxPath = fs::path(localAppDataPath) / "Roblox";
        std::cout << "Removing Roblox directory: " << robloxPath << std::endl;
        rmDir(robloxPath);
    }
    else {
        std::cerr << "Failed to get Local AppData path." << std::endl;
    }

    std::string systemDrive = getSystemDrive();
    fs::path crashHandlerPath = fs::path(systemDrive) / "Program Files (x86)" / "Roblox" / "Versions" / "version-3243b6d003cf4642" / "RobloxCrashHandler.exe";
    std::cout << "Removing Roblox Crash Handler: " << crashHandlerPath << std::endl;
    rmFile(crashHandlerPath);

    char tempPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tempPath)) {
        fs::path tempDir(tempPath);
        fs::path robloxTempPath = tempDir / "Roblox";
        if (fs::exists(robloxTempPath)) {
            std::cout << "Clearing Roblox temp files from: " << robloxTempPath << std::endl;
            for (const auto& entry : fs::directory_iterator(robloxTempPath)) {
                if (entry.is_regular_file()) {
                    fs::remove(entry.path());
                }
            }
        }
    }
    else {
        std::cerr << "Failed to get Temp path." << std::endl;
    }

    fs::path cTempPath = fs::path(systemDrive) / "Temp";
    if (fs::exists(cTempPath)) {
        fs::path robloxTempPath = cTempPath / "Roblox";
        if (fs::exists(robloxTempPath)) {
            std::cout << "Clearing Roblox temp files from: " << robloxTempPath << std::endl;
            for (const auto& entry : fs::directory_iterator(robloxTempPath)) {
                if (entry.is_regular_file()) {
                    fs::remove(entry.path());
                }
            }
        }
    }

    std::cout << "Roblox clearing process completed." << std::endl;
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
            char szExeFileA[MAX_PATH];
            WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, szExeFileA, MAX_PATH, NULL, NULL);

            if (_stricmp(szExeFileA, processName.c_str()) == 0) {
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
        std::cerr << "Window not found: " << windowTitle << std::endl;
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
    else {
        std::cerr << "Failed to iterate through processes." << std::endl;
    }
    CloseHandle(hProcessSnap);
    return true;
}