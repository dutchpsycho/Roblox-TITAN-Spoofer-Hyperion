#ifndef HYPERION_HPP
#define HYPERION_HPP

#include <windows.h>
#include <string>

void HookNtDll();
bool spoofReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName);
bool FWWMIC(const std::string& wmicCommand, const std::string& propertyName);
void spoofHyperion();

#endif