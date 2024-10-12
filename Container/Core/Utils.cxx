#include "../Util/Utils.hxx"

#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <iostream>

bool IsAdministrator(HANDLE proc) {
    bool IsAdmin = false;
    HANDLE hToken = NULL;
    if (OpenProcessToken(proc, TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION TE;
        DWORD rSize = 0;
        if (GetTokenInformation(hToken, TokenElevation, &TE, sizeof(TOKEN_ELEVATION), &rSize))
            IsAdmin = TE.TokenIsElevated;
        CloseHandle(hToken);
    }
    return IsAdmin;
}

std::string randstring(size_t length) {
    const char* chars = "0123456789ABCDEF";
    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += chars[rand() % 16];
    }
    return result;
}

void WindowName() {
    HWND hwnd = GetConsoleWindow();
    if (hwnd != NULL) {
        std::string randomName = std::to_string(rand());
        SetWindowTextA(hwnd, randomName.c_str());
    }
}

void SigFucker() {
    srand(static_cast<unsigned int>(time(0)));
    WindowName();
}

void Colour(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void TITAN() {
    std::string art = R"(
                              dMb,
                            ,dMMMMb,          ,,
                         ,dMMMMMMMMMb, eeee8888"
                      ,mMMm!!!!XXXXMMMMM"""
                    ,d!!XXMMXX88888888W"
                   `MX88dMM8888WWWMMMMMMb,
                       '""MMMMMMMMMMMMMMMMb
                         MMMMMMMMMMMMMMMMMMb,
                        dMMMMMMMMMMMMMMMMMMMMb,,
            _,dMMMMMMMMMMXXXX!!!!!!!!!!!!!!XXXXXMP
       _,dMMXX!!!!!!!!!!!!!!!!!!XXXXX888888888WWC
   _,dMMX!!!MMMM!!!!!!!!XXXXXX888888888888WWMMMMMb,
  dMMX!!!!!MMM!XXXXXX88888888888888888WWMMMMMMMMMMMb
 dMMXXXXXX8MMMM88888888888888888WWWMMMMMMMMMMMMMMMMMb    ,d8
 MMMMWW888888MMMMM8888888WWMMMMMMMMMMMMMMMMMMMMMMMMMMM,d88P'
  YMMMMMWW888888WWMMMMMMMMMMP"""'    `"YMMMMMMMMMMMXMMMMMP
     `""YMMMMMMMMMMMMMP""'            mMMMm!XXXXX8888888e,
                                    ,d!!XXMM888888888888WW
Swedish.Psycho                     "MX88dMM888888WWWMMMMMMb
TITAN Spoofer V6.1                        """```"YMMMMMMMYMMM
Hyperion: 'New phone, who dis?                    `"YMMMMM
https://github.com/dutchpsycho/TITAN-Spoofer         `"YMP
    )";
    std::cout << art << std::endl;
}

void Menu(int selected) {
    std::string options[] = {
        "Spoof"
    };

    for (int i = 0; i < 1; ++i) {
        if (i == selected) {
            std::cout << "> " << options[i] << " <" << std::endl;
        }
        else {
            std::cout << "  " << options[i] << std::endl;
        }
    }
}

void _ps(const std::string& message, int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
    std::cout << message << std::endl;
    SetConsoleTextAttribute(hConsole, 7);
}

bool readReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName, std::wstring& value) {
    HKEY hKey;
    if (RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    wchar_t buffer[256];
    DWORD bufferSize = sizeof(buffer);
    DWORD type = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, &type, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS && type == REG_SZ) {
        value = buffer;
        RegCloseKey(hKey);
        return true;
    }
    RegCloseKey(hKey);
    return false;
}

bool writeReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& value) {
    HKEY hKey;
    if (RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    if (RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, (const BYTE*)value.c_str(), static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return true;
    }
    RegCloseKey(hKey);
    return false;
}

void delReg(HKEY hKeyRoot, const std::wstring& subKey) {
    HKEY hKey;
    if (RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        wchar_t keyName[256];
        DWORD keyNameSize = sizeof(keyName) / sizeof(keyName[0]);
        while (RegEnumKeyExW(hKey, 0, keyName, &keyNameSize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            RegDeleteKeyW(hKeyRoot, (subKey + L"\\" + keyName).c_str());
            keyNameSize = sizeof(keyName) / sizeof(keyName[0]);
        }
        RegCloseKey(hKey);
        RegDeleteKeyW(hKeyRoot, subKey.c_str());
    }
}

void rmDir(const fs::path& dirPath) {
    try {
        if (fs::exists(dirPath) && fs::is_directory(dirPath)) {
            for (const auto& entry : fs::directory_iterator(dirPath)) {
                fs::remove_all(entry.path());
            }
        }
    }
    catch (const fs::filesystem_error& err) {
        std::cerr << "Error: " << err.what() << std::endl;
    }
}

void rmFile(const fs::path& filePath) {
    try {
        if (fs::exists(filePath)) {
            fs::remove(filePath);
        }
    }
    catch (const fs::filesystem_error& err) {
        std::cerr << "Error: " << err.what() << std::endl;
    }
}

std::wstring stringToWstring(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    if (size_needed <= 0) throw std::runtime_error("MultiByteToWideChar failed");
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string wstringToString(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    if (size_needed <= 0) throw std::runtime_error("WideCharToMultiByte failed");
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}