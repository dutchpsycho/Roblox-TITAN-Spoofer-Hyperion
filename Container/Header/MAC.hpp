#ifndef MAC_HPP
#define MAC_HPP

#include <string>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")

bool getOGMAC(IP_ADAPTER_INFO* adapter, std::string& originalMac);
bool setMAC(const std::wstring& adapterGUID, const std::string& newMac);
std::string rMAC();
bool resetAdapter(const std::string& adapterName);
bool spoofMac();

#endif