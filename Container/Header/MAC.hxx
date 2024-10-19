// MAC.hxx

#ifndef MAC_HXX
#define MAC_HXX

#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <setupapi.h>
#include <comdef.h>
#include <Wbemidl.h>
#include <sddl.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "wbemuuid.lib")

void spoofMac();
std::wstring stringToWstring(const std::string& s);
std::string wstringToString(const std::wstring& ws);

#endif