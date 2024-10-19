#ifndef ROBLOX_HXX
#define ROBLOX_HXX

#include <windows.h>
#include <tlhelp32.h>
#include <shlobj.h>

#include <filesystem>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <regex>

namespace fs = std::filesystem;

bool TerminateProcessSafely(const std::string& processName);
bool WaitForProcessTermination(const std::string& processName);
std::string getSystemDrive();
void ClearRoblox();

#endif