// Roblox.hxx

#ifndef ROBLOX_HXX
#define ROBLOX_HXX

#include <windows.h>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

bool Freeze(DWORD processId);
bool Terminate(const std::string& processName);
bool ScopeProcess(const std::string& processName);
bool ScopeWindow(const std::string& windowTitle);
void IndexReg(HKEY hKeyRoot, const std::wstring& searchString);
bool CopySets(const fs::path& SetFPath);
std::string NewSets(const std::string& xmlContent);
void ClearRoblox();

#endif