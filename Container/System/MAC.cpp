#include "../Header/Job.hpp"

#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iphlpapi.h>
#include <windows.h>

#pragma comment(lib, "iphlpapi.lib")

std::string rMAC() {
    std::stringstream newMac;
    newMac << std::hex << std::setw(2) << std::setfill('0') << ((rand() % 0xFF) & 0xFE) << ":";
    for (int i = 1; i < 6; i++) {
        newMac << std::hex << std::setw(2) << std::setfill('0') << (rand() % 0xFF);
        if (i != 5) newMac << ":";
    }
    return newMac.str();
}

std::vector<std::pair<std::string, std::string>> getAdapterInfo() {
    std::vector<std::pair<std::string, std::string>> adapters;

    IP_ADAPTER_INFO adapterInfo[16];
    DWORD dwBufLen = sizeof(adapterInfo);

    DWORD dwStatus = GetAdaptersInfo(adapterInfo, &dwBufLen);
    if (dwStatus != ERROR_SUCCESS) {
        std::cerr << "GetAdaptersInfo failed with error: " << dwStatus << std::endl;
        return adapters;
    }

    PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
    while (pAdapterInfo) {
        std::string adapterName = pAdapterInfo->AdapterName;
        std::string adapterDescription = pAdapterInfo->Description;

        if (adapterDescription.find("VMware") == std::string::npos &&
            adapterDescription.find("Virtual") == std::string::npos) {
            adapters.push_back({ adapterDescription, adapterName });
        }

        pAdapterInfo = pAdapterInfo->Next;
    }

    return adapters;
}

bool setMAC(const std::string& adapterName, const std::string& newMac) {
    std::wstring adapterNameW = stringToWstring(adapterName);
    std::wstring newMacW = stringToWstring(newMac);

    for (int i = 0; i < 100; i++) {
        std::wstringstream regPath;
        regPath << L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4d36e972-e325-11ce-bfc1-08002be10318}\\"
            << std::setw(4) << std::setfill(L'0') << i;

        std::wstring instanceId;
        if (readReg(HKEY_LOCAL_MACHINE, regPath.str(), L"NetCfgInstanceId", instanceId)) {
            if (wstringToString(instanceId) == adapterName) {
                if (writeReg(HKEY_LOCAL_MACHINE, regPath.str(), L"NetworkAddress", newMacW)) {
                    return true;
                }
                else {
                    std::cerr << "Failed to spoof: " << adapterName << std::endl;
                }
            }
        }
    }
    return false;
}

bool resetAdapter(const std::string& adapterName) {
    std::string disableCmd = "netsh interface set interface \"" + adapterName + "\" admin=disable";
    std::string enableCmd = "netsh interface set interface \"" + adapterName + "\" admin=enable";

    int result = system(disableCmd.c_str());
    if (result != 0) {
        return false;
    }
    Sleep(1000);
    result = system(enableCmd.c_str());
    if (result != 0) {
    }
    return (result == 0);
}

bool spoofMac() {
    std::vector<std::pair<std::string, std::string>> adapterInfo = getAdapterInfo();

    for (const auto& [adapterDescription, adapterName] : adapterInfo) {
        std::string newMac = rMAC();

        if (!setMAC(adapterName, newMac)) {
        }
        else {
            std::cout << "Spoofed MAC: " << adapterDescription << std::endl;

            if (!resetAdapter(adapterDescription)) {
            }
            else {
                std::cout << "Reset adapter -> " << adapterDescription << std::endl;
            }
        }
    }
    return true;
}