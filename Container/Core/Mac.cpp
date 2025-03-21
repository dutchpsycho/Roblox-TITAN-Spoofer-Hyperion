#include "../Header/Mac.h"

namespace MAC {

    std::wstring GetCurrentSSID() {
        FILE* pipe = _wpopen(L"netsh wlan show interfaces", L"r");
        if (!pipe) return L"";

        wchar_t buffer[512];
        std::wstring output;

        while (fgetws(buffer, sizeof(buffer) / sizeof(wchar_t), pipe)) {
            output += buffer;
        }

        _pclose(pipe);

        size_t pos = output.find(L"SSID                   : ");
        if (pos != std::wstring::npos) {
            size_t start = pos + wcslen(L"SSID                   : ");
            size_t end = output.find(L"\n", start);
            if (end != std::wstring::npos)
                return output.substr(start, end - start);
        }

        return L"";
    }

    void bounceAdapter(const std::wstring& adapterName) {
        std::wstring disableCmd = L"netsh interface set interface name=\"" + adapterName + L"\" admin=disable";
        std::wstring enableCmd = L"netsh interface set interface name=\"" + adapterName + L"\" admin=enable";

        _wsystem((disableCmd + L" >nul 2>&1").c_str());
        Sleep(1000);
        _wsystem((enableCmd + L" >nul 2>&1").c_str());
        Sleep(2000);

        if (adapterName.find(L"Wi-Fi") != std::wstring::npos || adapterName.find(L"Wireless") != std::wstring::npos) {
            std::wstring ssid = GetCurrentSSID();
            if (!ssid.empty()) {
                std::wstring connectCmd = L"netsh wlan disconnect >nul 2>&1 && netsh wlan connect name=\"" + ssid + L"\" >nul 2>&1";
                _wsystem(connectCmd.c_str());
            }
        }
    }

    void MacSpoofer::run() {
        Services::SectHeader("MAC Spoofing", 196);
        spoofMac();
    }

    std::vector<std::wstring> MacSpoofer::getAdapters() {
        ULONG bufferSize = 0;
        GetAdaptersInfo(nullptr, &bufferSize);
        std::vector<BYTE> buffer(bufferSize);

        auto pAdapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());
        if (GetAdaptersInfo(pAdapterInfo, &bufferSize) != ERROR_SUCCESS) {
            return {};
        }

        std::vector<std::wstring> adapters;
        adapters.reserve(16); // if u have more than 16 adapters im concerned
        for (auto pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
            adapters.emplace_back(pAdapter->Description, pAdapter->Description + strlen(pAdapter->Description));
        }
        return adapters;
    }

    std::optional<std::wstring> MacSpoofer::resAdapter(const std::wstring& adapterName) {

        COM::COMInitializer comInit;

        IWbemLocator* pLoc = nullptr;
        IWbemServices* pSvc = nullptr;

        HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
            IID_IWbemLocator, reinterpret_cast<LPVOID*>(&pLoc));
        if (FAILED(hr)) return std::nullopt;

        BSTR namespacePath = SysAllocString(L"ROOT\\CIMV2");
        hr = pLoc->ConnectServer(namespacePath, nullptr, nullptr, 0, 0, nullptr, nullptr, &pSvc);
        SysFreeString(namespacePath);
        if (FAILED(hr)) {
            pLoc->Release();
            return std::nullopt;
        }

        IEnumWbemClassObject* pEnumerator = nullptr;
        std::wstring query = L"SELECT * FROM Win32_NetworkAdapter WHERE Name = '" + adapterName + L"'";
        BSTR queryLang = SysAllocString(L"WQL");
        BSTR queryString = SysAllocString(query.c_str());

        hr = pSvc->ExecQuery(queryLang, queryString,
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            nullptr, &pEnumerator);

        SysFreeString(queryLang);
        SysFreeString(queryString);

        if (FAILED(hr)) {
            pSvc->Release();
            pLoc->Release();
            return std::nullopt;
        }

        IWbemClassObject* pAdapter = nullptr;
        ULONG uReturn = 0;
        std::optional<std::wstring> result;

        HRESULT nextRes = pEnumerator->Next(WBEM_INFINITE, 1, &pAdapter, &uReturn);
        if (SUCCEEDED(nextRes) && uReturn != 0) {
            VARIANT vtGUID;
            VariantInit(&vtGUID);
            if (SUCCEEDED(pAdapter->Get(L"GUID", 0, &vtGUID, nullptr, nullptr)) && vtGUID.bstrVal != nullptr) {
                result = vtGUID.bstrVal;
                VariantClear(&vtGUID);
            }
            pAdapter->Release();
        }

        pEnumerator->Release();
        pSvc->Release();
        pLoc->Release();

        return result;
    }

    std::wstring MacSpoofer::getAdapterRegPath(const std::wstring& adapterGUID) {
        static const std::wstring basePath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
        HKEY hKey;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, basePath.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            return L"";
        }

        DWORD index = 0;
        WCHAR subKeyName[256];
        DWORD subKeyLen = 256;

        LONG result;
        while ((result = RegEnumKeyEx(hKey, index, subKeyName, &subKeyLen, nullptr, nullptr, nullptr, nullptr)) != ERROR_NO_MORE_ITEMS) {
            if (result != ERROR_SUCCESS) {
                subKeyLen = 256;
                index++;
                continue;
            }

            std::wstring subKeyPath = basePath + L"\\" + subKeyName;
            HKEY subKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKeyPath.c_str(), 0, KEY_READ, &subKey) == ERROR_SUCCESS) {
                WCHAR guidValue[256];
                DWORD guidLen = sizeof(guidValue);
                if (RegQueryValueEx(subKey, L"NetCfgInstanceId", nullptr, nullptr,
                    reinterpret_cast<LPBYTE>(guidValue), &guidLen) == ERROR_SUCCESS) {
                    if (adapterGUID == guidValue) {
                        RegCloseKey(subKey);
                        RegCloseKey(hKey);
                        return subKeyPath;
                    }
                }
                RegCloseKey(subKey);
            }
            subKeyLen = 256;
            index++;
        }

        RegCloseKey(hKey);
        return L"";
    }

    void MacSpoofer::spoofMac() {
        auto adapters = getAdapters();
        if (adapters.empty()) return;

        std::mutex outputMutex;
        std::vector<std::thread> spoofThreads;

        for (const auto& adapter : adapters) {
            spoofThreads.emplace_back([&, adapter]() {
                COM::COMInitializer comInit;
                auto adapterGUID = resAdapter(adapter);
                if (!adapterGUID) return;

                auto regPath = getAdapterRegPath(*adapterGUID);
                if (regPath.empty()) return;

                auto newMac = Services::genMac();
                std::wstring macAddress(newMac.begin(), newMac.end());

                {
                    std::lock_guard<std::mutex> lock(outputMutex);
                    std::wcout << L"Spoofed -> " << adapter << L", New MAC -> " << macAddress << L"\n";
                }

                HKEY hKey;
                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, regPath.c_str(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
                    RegSetValueEx(hKey, L"NetworkAddress", 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(macAddress.c_str()),
                        static_cast<DWORD>((macAddress.size() + 1) * sizeof(wchar_t)));
                    RegCloseKey(hKey);
                }

                std::thread bounceThread([adapter]() {
                    bounceAdapter(adapter);
                    });

                bounceThread.detach();
                });
        }

        for (auto& t : spoofThreads) {
            if (t.joinable())
                t.join();

        }
    }
}