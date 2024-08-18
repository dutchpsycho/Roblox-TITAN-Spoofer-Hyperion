#ifndef ROBLOX_HPP
#define ROBLOX_HPP

#include <windows.h>
#include <string>

bool Freeze(DWORD processId);
bool Terminate(const std::string& processName);
bool ScopeProcess(const std::string& processName);
bool ScopeWindow(const std::string& windowTitle);
void IndexReg(HKEY hKeyRoot, const std::wstring& searchString);
void ClearRoblox();

#endif