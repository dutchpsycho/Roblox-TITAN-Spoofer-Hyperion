#include "../Header/Mac.h"

namespace MAC {

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
        static thread_local COM::COMInitializer comInit; // no multicom

        IWbemLocator* pLoc = nullptr;
        IWbemServices* pSvc = nullptr;

        HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, reinterpret_cast<LPVOID*>(&pLoc));
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

        if (pEnumerator->Next(WBEM_INFINITE, 1, &pAdapter, &uReturn) == WBEM_S_NO_ERROR) {
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

        while (RegEnumKeyEx(hKey, index++, subKeyName, &subKeyLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            std::wstring subKeyPath = basePath + L"\\" + subKeyName;

            HKEY subKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subKeyPath.c_str(), 0, KEY_READ, &subKey) == ERROR_SUCCESS) {
                WCHAR guidValue[256];
                DWORD guidLen = sizeof(guidValue);
                if (RegQueryValueEx(subKey, L"NetCfgInstanceId", nullptr, nullptr, reinterpret_cast<LPBYTE>(guidValue), &guidLen) == ERROR_SUCCESS) {
                    if (adapterGUID == guidValue) {
                        RegCloseKey(subKey);
                        RegCloseKey(hKey);
                        return subKeyPath;
                    }
                }
                RegCloseKey(subKey);
            }
            subKeyLen = 256;
        }

        RegCloseKey(hKey);
        return L"";
    }

    void MacSpoofer::spoofMac() {
        auto adapters = getAdapters();
        if (adapters.empty()) return;

        std::mutex outputMutex;
        std::vector<std::thread> threads;

        for (const auto& adapter : adapters) {
            threads.emplace_back([&, adapter]() {
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
                    RegSetValueEx(hKey, L"NetworkAddress", 0, REG_SZ, reinterpret_cast<const BYTE*>(macAddress.c_str()), static_cast<DWORD>(macAddress.size() * sizeof(wchar_t)));
                    RegCloseKey(hKey);
                }
                });
        }

        for (auto& thread : threads) {
            if (thread.joinable()) thread.join();
        }
    }
}