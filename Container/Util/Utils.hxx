// Utils.hxx

#ifndef JOB_HXX
#define JOB_HXX

#include <iostream>
#include <fstream>
#include <string>
#include <conio.h>
#include <windows.h>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>

namespace fs = std::filesystem;

std::string randstring(size_t length);

void WindowName();
void SigFucker();
void Colour(int color);
void TITAN();

void Menu(int selected);
void _ps(const std::string& message, int color);

bool readReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName, std::wstring& value);
bool writeReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& value);
void delReg(HKEY hKeyRoot, const std::wstring& subKey);

void rmDir(const fs::path& dirPath);
void rmFile(const fs::path& filePath);

std::wstring stringToWstring(const std::string& str);
std::string wstringToString(const std::wstring& wstr);

#endif