#ifndef JOB_HPP
#define JOB_HPP

#include <windows.h>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

std::string randstring(size_t length);

bool readReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName, std::wstring& value);
bool writeReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& value);
void delReg(HKEY hKeyRoot, const std::wstring& subKey);

void rmDir(const fs::path& dirPath);
void rmFile(const fs::path& filePath);

std::wstring stringToWstring(const std::string& str);
std::string wstringToString(const std::wstring& wstr);

#endif