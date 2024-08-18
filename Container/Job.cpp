#include "./Header/Job.hpp"

#include <ctime>
#include <iostream>

std::string randstring(size_t length) {
    const char* chars = "0123456789ABCDEF";
    std::string result;
    for (size_t i = 0; i < length; ++i) {
        result += chars[rand() % 16];
    }
    return result;
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
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string wstringToString(const std::wstring& wstr) {
    if (wstr.empty()) {
        return std::string();
    }
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}