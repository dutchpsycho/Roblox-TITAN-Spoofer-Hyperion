// Elevate.hxx

#ifndef ELEVATE_HXX
#define ELEVATE_HXX

#include "Utils.hxx"

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <tlhelp32.h>
#include <iostream>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "advapi32.lib")

void Elevate() {
    BOOL fIsElevated = FALSE;
    HANDLE hToken = NULL;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD cbSize = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize)) {
            fIsElevated = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }

    if (!fIsElevated) {
        WCHAR szPath[MAX_PATH];
        SHELLEXECUTEINFOW sei = { sizeof(sei) };

        if (GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath))) {
            sei.lpVerb = L"runas";
            sei.lpFile = szPath;
            sei.hwnd = NULL;
            sei.nShow = SW_SHOWNORMAL;
            if (ShellExecuteExW(&sei)) {
                exit(0);
            }
        }

        if (GetModuleFileNameW(NULL, szPath, ARRAYSIZE(szPath))) {
            std::wstring regKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
            std::wstring valueName = L"ElevateProcess";
            if (writeReg(HKEY_CURRENT_USER, regKey, valueName, szPath)) {
                exit(0);
            }
        }

        if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
            TOKEN_PRIVILEGES tp;
            LUID luid;

            if (LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid)) {
                tp.PrivilegeCount = 1;
                tp.Privileges[0].Luid = luid;
                tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
                if (AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
                    CloseHandle(hToken);
                    if (GetLastError() == ERROR_SUCCESS) return;
                }
            }
            CloseHandle(hToken);
        }

        std::cout << "Elevation failed.  Restart as an administrator." << std::endl;
        exit(1);
    }
}

#endif