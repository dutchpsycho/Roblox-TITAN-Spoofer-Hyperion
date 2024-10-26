// MAC.cxx

#define WIN32_LEAN_AND_MEAN

#include "../Header/MAC.hxx"
#include "../Util/Utils.hxx"

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

std::string rMAC() {
    std::stringstream newMac;
    newMac << std::hex << std::setw(2) << std::setfill('0') << ((rand() % 0xFF) & 0xFE);
    for (int i = 1; i < 6; i++) {
        newMac << std::hex << std::setw(2) << std::setfill('0') << (rand() % 0xFF);
        if (i < 5) {
            newMac << ":";
        }
    }
    return newMac.str();
}

bool setRegistryValue(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName, const std::wstring& value) {
    return writeReg(hKeyRoot, subKey, valueName, value);
}

bool setNetworkAdapterState(const std::wstring& adapterName, bool enable) {
    HRESULT hres;
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        std::cerr << "Failed to initialize COM library. Error code = 0x" << std::hex << hres << std::endl;
        return false;
    }

    hres = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL);

    if (FAILED(hres)) {
        std::cerr << "Failed to initialize security. Error code = 0x" << std::hex << hres << std::endl;
        CoUninitialize();
        return false;
    }

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID*)&pLoc);

    if (FAILED(hres)) {
        std::cerr << "Failed to create IWbemLocator object. Error code = 0x" << std::hex << hres << std::endl;
        CoUninitialize();
        return false;
    }

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        NULL,
        NULL,
        0,
        NULL,
        0,
        0,
        &pSvc);

    if (FAILED(hres)) {
        std::cerr << "Could not connect. Error code = 0x" << std::hex << hres << std::endl;
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        NULL,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE
    );

    if (FAILED(hres)) {
        std::cerr << "Could not set proxy blanket. Error code = 0x" << std::hex << hres << std::endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    IEnumWbemClassObject* pEnumerator = NULL;
    std::wstring query = L"SELECT * FROM Win32_NetworkAdapter WHERE NetConnectionID = '" + adapterName + L"'";

    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t(query.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres)) {
        std::cerr << "Query for network adapter failed. Error code = 0x" << std::hex << hres << std::endl;
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return false;
    }

    IWbemClassObject* pAdapter = NULL;
    ULONG uReturn = 0;

    while (pEnumerator) {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pAdapter, &uReturn);
        if (0 == uReturn) {
            break;
        }

        VARIANT vtPath;
        hr = pAdapter->Get(L"__PATH", 0, &vtPath, 0, 0);
        if (SUCCEEDED(hr)) {
            IWbemClassObject* pClass = NULL;
            hr = pSvc->GetObject(vtPath.bstrVal, 0, NULL, &pClass, NULL);

            if (SUCCEEDED(hr)) {
                IWbemClassObject* pInParamsDefinition = NULL;
                IWbemClassObject* pOutParams = NULL;

                BSTR methodName = SysAllocString(enable ? L"Enable" : L"Disable");
                hr = pClass->GetMethod(methodName, 0, &pInParamsDefinition, NULL);
                if (SUCCEEDED(hr)) {
                    hr = pSvc->ExecMethod(vtPath.bstrVal, methodName, 0, NULL, NULL, &pOutParams, NULL);
                    if (FAILED(hr)) {
                        std::cerr << "Failed to execute method. Error code = 0x" << std::hex << hr << std::endl;
                    }
                }

                if (pOutParams) pOutParams->Release();
                if (pInParamsDefinition) pInParamsDefinition->Release();
                if (pClass) pClass->Release();

                SysFreeString(methodName);
            }
        }
        VariantClear(&vtPath);
        pAdapter->Release();
    }

    if (pEnumerator) pEnumerator->Release();
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();

    return true;
}

bool disableNetworkAdapter(const std::wstring& adapterName) {
    return setNetworkAdapterState(adapterName, false);
}

bool enableNetworkAdapter(const std::wstring& adapterName) {
    return setNetworkAdapterState(adapterName, true);
}

std::vector<std::string> getNetworkAdapters() {
    std::vector<std::string> adapters;
    ULONG bufferSize = 0;
    GetAdaptersInfo(NULL, &bufferSize);
    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_INFO pAdapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(&buffer[0]);

    if (GetAdaptersInfo(pAdapterInfo, &bufferSize) == ERROR_SUCCESS) {
        PIP_ADAPTER_INFO pAdapter = pAdapterInfo;
        while (pAdapter) {
            adapters.push_back(pAdapter->Description);
            pAdapter = pAdapter->Next;
        }
    }
    return adapters;
}

std::wstring getCurrentUserSID() {
    HANDLE tokenHandle = NULL;
    DWORD bufferSize = 0;
    std::wstring sidString;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle)) {
        GetTokenInformation(tokenHandle, TokenUser, NULL, 0, &bufferSize);
        if (bufferSize > 0) {
            std::vector<BYTE> buffer(bufferSize);
            if (GetTokenInformation(tokenHandle, TokenUser, buffer.data(), bufferSize, &bufferSize)) {
                SID_AND_ATTRIBUTES* sidAndAttributes = reinterpret_cast<SID_AND_ATTRIBUTES*>(buffer.data());
                LPWSTR sid = NULL;
                if (ConvertSidToStringSidW(sidAndAttributes->Sid, &sid)) {
                    sidString = sid;
                    LocalFree(sid);
                }
            }
        }
        CloseHandle(tokenHandle);
    }

    return sidString;
}

std::wstring getNetworkAdapterClassGUID() {
    GUID NetGUID;
    HRESULT hr = CLSIDFromString(L"{4d36e972-e325-11ce-bfc1-08002be10318}", &NetGUID);
    if (FAILED(hr)) {
        std::cerr << "Failed to parse Network Adapter Class GUID." << std::endl;
        return L"";
    }

    HDEVINFO hDevInfo = SetupDiGetClassDevsW(&NetGUID, NULL, NULL, DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to get device class information." << std::endl;
        return L"";
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    WCHAR classGUID[39];
    if (StringFromGUID2(NetGUID, classGUID, 39) > 0) {
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return classGUID;
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    std::cerr << "Failed to retrieve Network Adapter Class GUID as string." << std::endl;
    return L"";
}

void spoofMac() {
    std::cout << "\033[38;2;139;0;0m" << "\n========== MAC SPOOFING ==========\n" << "\033[0m";

    std::vector<std::string> adapters = getNetworkAdapters();
    std::wstring netAdapterClassGUID = getNetworkAdapterClassGUID();

    if (netAdapterClassGUID.empty()) {
        std::cerr << "Failed to locate Network Adapter Class GUID." << std::endl;
        return;
    }

    for (const auto& adapter : adapters) {
        std::wstring wAdapterName = stringToWstring(adapter);
        std::string randomMac = rMAC();
        std::wstring registryPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\" + netAdapterClassGUID;

        if (!disableNetworkAdapter(wAdapterName)) {
            std::cerr << "Failed to disable network adapter: " << adapter << std::endl;
            continue;
        }

        for (int i = 0; i < 1000; i++) {
            std::wstring subKey = registryPath + L"\\" + std::to_wstring(i);
            std::wstring networkAdapterName;

            HKEY hKey;
            if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subKey.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                WCHAR buffer[256];
                DWORD bufferSize = sizeof(buffer);
                if (RegQueryValueExW(hKey, L"DriverDesc", NULL, NULL, (LPBYTE)buffer, &bufferSize) == ERROR_SUCCESS) {
                    networkAdapterName = buffer;
                }
                RegCloseKey(hKey);

                if (wAdapterName == networkAdapterName) {
                    if (!setRegistryValue(HKEY_LOCAL_MACHINE, subKey, L"NetworkAddress", stringToWstring(randomMac))) {
                        std::cerr << "Failed to set MAC address in registry." << std::endl;
                    }
                    break;
                }
            }
        }

        if (!enableNetworkAdapter(wAdapterName)) {
            std::cerr << "Failed to re-enable network adapter: " << adapter << std::endl;
        }

        std::cout << "Spoofed -> " << adapter << " - New MAC :: [ " << randomMac << " ]" << std::endl;
    }
}