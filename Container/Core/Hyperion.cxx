// Hyperion.cxx

#include "../Header/Hyperion.hxx"
#include "../Util/Utils.hxx"

struct HandleDeleter {
    void operator()(SC_HANDLE h) const {
        if (h) {
            CloseServiceHandle(h);
        }
    }
};

using ServiceHandle = std::unique_ptr<std::remove_pointer_t<SC_HANDLE>, HandleDeleter>;

struct ComInitializer {
    ComInitializer() {
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (FAILED(hr)) {
            throw std::runtime_error("No COM? :P");
        }
    }
    ~ComInitializer() {
        CoUninitialize();
    }
};

constexpr auto WMI_NAMESPACE = L"ROOT\\CIMV2";
constexpr auto WMI_QUERY_LANGUAGE = L"WQL";
constexpr auto DISPLAY_KEY_PATH = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY";
constexpr auto ERROR_MESSAGE_COM_INIT = "Failed to initialize COM library. Error code = 0x";
constexpr auto SERVICE_NAME = L"winmgmt";

std::optional<bool> PermCheck() {
    ServiceHandle hSCManager{ OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE) };
    if (!hSCManager) {
        std::cerr << "Failed to open service control manager." << std::endl;
        return std::nullopt;
    }

    ServiceHandle hService{ OpenService(hSCManager.get(), SERVICE_NAME, SERVICE_QUERY_STATUS) };
    if (!hService) {
        std::cerr << "Failed to open WMI service." << std::endl;
        return std::nullopt;
    }

    SERVICE_STATUS_PROCESS ssStatus{};
    DWORD dwBytesNeeded = 0;

    if (!QueryServiceStatusEx(hService.get(), SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssStatus),
                              sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
        std::cerr << "Failed to query WMI service." << std::endl;
        return std::nullopt;
    }

    if (ssStatus.dwCurrentState != SERVICE_RUNNING) {
        std::cerr << "WMI service is not running." << std::endl;
        return std::nullopt;
    }

    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        std::cerr << "Failed to open process token." << std::endl;
        return std::nullopt;
    }

    std::unique_ptr<void, decltype(&CloseHandle)> tokenHandle{ hToken, CloseHandle };

    DWORD dwSize = 0;
    if (!GetTokenInformation(hToken, TokenPrivileges, nullptr, 0, &dwSize) &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        std::cerr << "Failed to query token privileges." << std::endl;
        return std::nullopt;
    }

    std::vector<BYTE> buffer(dwSize);
    if (!GetTokenInformation(hToken, TokenPrivileges, buffer.data(), dwSize, &dwSize)) {
        std::cerr << "Failed to retrieve token info." << std::endl;
        return std::nullopt;
    }

    return true;
}

bool FWWMIC(const std::string& wmicCommand, const std::string& propertyName) {
    if (!PermCheck()) {
        std::cerr << "No permissions or WMI service isn't running." << std::endl;
        return false;
    }

    ComInitializer comInitializer;
    IWbemLocator* pLoc = nullptr;
    HRESULT hres = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres) || !pLoc) {
        std::cerr << ERROR_MESSAGE_COM_INIT << std::hex << hres << std::endl;
        return false;
    }

    std::unique_ptr<IWbemLocator, decltype(&IWbemLocator::Release)> locator{ pLoc, &IWbemLocator::Release };

    IWbemServices* pSvc = nullptr;
    hres = pLoc->ConnectServer(_bstr_t(WMI_NAMESPACE), nullptr, nullptr, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres) || !pSvc) {
        std::cerr << "Failed to connect to WMI namespace. Error code = 0x" << std::hex << hres << std::endl;
        return false;
    }

    std::unique_ptr<IWbemServices, decltype(&IWbemServices::Release)> services{ pSvc, &IWbemServices::Release };

    hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                             RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (FAILED(hres)) {
        std::cerr << "Failed to set proxy blanket. Error code = 0x" << std::hex << hres << std::endl;
        return false;
    }

    std::wstring wqlQuery = L"SELECT " + stringToWstring(propertyName) + L" FROM " + stringToWstring(wmicCommand);
    IEnumWbemClassObject* pEnumerator = nullptr;
    hres = pSvc->ExecQuery(bstr_t(WMI_QUERY_LANGUAGE), bstr_t(wqlQuery.c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                           nullptr, &pEnumerator);
    if (FAILED(hres) || !pEnumerator) {
        std::cerr << "WMI query failed. Error code = 0x" << std::hex << hres << std::endl;
        return false;
    }

    std::unique_ptr<IEnumWbemClassObject, decltype(&IEnumWbemClassObject::Release)> enumerator{ pEnumerator,
                                                                                               &IEnumWbemClassObject::Release };

    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;

    while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == WBEM_S_NO_ERROR) {
        std::unique_ptr<IWbemClassObject, decltype(&IWbemClassObject::Release)> classObj{ pclsObj,
                                                                                         &IWbemClassObject::Release };

        VARIANT vtProp;
        VariantInit(&vtProp);

        hres = pclsObj->Get(_bstr_t(propertyName.c_str()), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hres)) {
            std::string newValue = randstring(8) + "-" + randstring(4) + "-" + randstring(4) + "-" + randstring(4) +
                                   "-" + randstring(12);
            std::wstring newValueW = stringToWstring(newValue);

            VARIANT vtNewVal;
            VariantInit(&vtNewVal);
            vtNewVal.vt = VT_BSTR;
            vtNewVal.bstrVal = SysAllocString(newValueW.c_str());

            if (vtNewVal.bstrVal) {
                hres = pclsObj->Put(_bstr_t(propertyName.c_str()), 0, &vtNewVal, 0);
                if (SUCCEEDED(hres)) {
                    std::cout << "Spoofed -> " << propertyName << " :: [ " << newValue << " ]" << std::endl;
                }
                SysFreeString(vtNewVal.bstrVal);
            } else {
                std::cerr << "Failed to allocate BSTR for new value." << std::endl;
            }
        }

        VariantClear(&vtProp);
    }

    return true;
}

bool spoofReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    DWORD dataSize = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr, nullptr, &dataSize) != ERROR_SUCCESS || dataSize == 0) {
        std::cerr << "Failed to query Registry value: " << wstringToString(subKey) << "\\" << wstringToString(valueName) << std::endl;
        RegCloseKey(hKey);
        return false;
    }

    std::wstring originalValue(dataSize / sizeof(wchar_t), L'\0');
    if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr, reinterpret_cast<LPBYTE>(&originalValue[0]), &dataSize) != ERROR_SUCCESS) {
        std::cerr << "Failed to read Registry value: " << wstringToString(subKey) << "\\" << wstringToString(valueName) << std::endl;
        RegCloseKey(hKey);
        return false;
    }

    std::string randomString = randstring(8) + "-" + randstring(4) + "-" + randstring(4) + "-" + randstring(4) + "-" + randstring(12);
    std::wstring newValue = stringToWstring(randomString);

    if (RegSetValueExW(hKey, valueName.c_str(), 0, REG_SZ, reinterpret_cast<const BYTE*>(newValue.c_str()), static_cast<DWORD>((newValue.size() + 1) * sizeof(wchar_t))) != ERROR_SUCCESS) {
        std::cerr << "Failed to set Registry value: " << wstringToString(subKey) << "\\" << wstringToString(valueName) << std::endl;
        RegCloseKey(hKey);
        return false;
    }

    std::cout << "Spoofed -> " << wstringToString(subKey) << "\\" << wstringToString(valueName) << " :: [ " << randomString << " ]" << std::endl;
    RegCloseKey(hKey);
    return true;
}

bool spoofRegBinary(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName) {
    HKEY hKey = nullptr;
    if (RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    DWORD dataSize = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr, nullptr, &dataSize) != ERROR_SUCCESS || dataSize == 0) {
        std::cerr << "Failed to query Registry value: " << wstringToString(subKey) << "\\" << wstringToString(valueName) << std::endl;
        RegCloseKey(hKey);
        return false;
    }

    std::vector<BYTE> data(dataSize);
    if (RegQueryValueExW(hKey, valueName.c_str(), nullptr, nullptr, data.data(), &dataSize) != ERROR_SUCCESS) {
        std::cerr << "Failed to read Registry value: " << wstringToString(subKey) << "\\" << wstringToString(valueName) << std::endl;
        RegCloseKey(hKey);
        return false;
    }

    for (auto& byte : data) {
        byte = static_cast<BYTE>(rand() % 256);
    }

    if (RegSetValueExW(hKey, valueName.c_str(), 0, REG_BINARY, data.data(), dataSize) != ERROR_SUCCESS) {
        std::cerr << "Failed to set Registry value: " << wstringToString(subKey) << "\\" << wstringToString(valueName) << std::endl;
        RegCloseKey(hKey);
        return false;
    }

    std::cout << "Spoofed -> " << wstringToString(subKey) << "\\" << wstringToString(valueName) << " :: [Binary Data]" << std::endl;
    RegCloseKey(hKey);
    return true;
}

void deleteRegistryKey(const std::wstring& sid, const std::wstring& subKeyName) {
    if (!NtOpenKey || !NtDeleteKeyPtr || !RtlInitUnicodeStringPtr || !NtClosePtr) {
        std::cerr << "Failed to load necessary Nt functions." << std::endl;
        return;
    }

    HKEY hKey = nullptr;
    std::wstring keyPath = L"\\Registry\\User\\" + sid + L"\\System\\CurrentControlSet\\Control\\" + subKeyName;

    UNICODE_STRING unicodeKey;
    RtlInitUnicodeStringPtr(&unicodeKey, keyPath.c_str());

    OBJECT_ATTRIBUTES objectAttributes;
    InitializeObjectAttributes(&objectAttributes, &unicodeKey, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

    NTSTATUS status = NtOpenKey(reinterpret_cast<PHANDLE>(&hKey), KEY_ALL_ACCESS, &objectAttributes);

    if (status == STATUS_SUCCESS && hKey) {
        status = NtDeleteKeyPtr(hKey);
        if (status == STATUS_SUCCESS) {
            std::wcout << L"Removed Registry Key -> " << keyPath << std::endl;
        }
        else {
            std::wcerr << L"Failed to delete Registry Key -> " << keyPath << L". NTSTATUS: " << status << std::endl;
        }
        NtClosePtr(hKey);
    }
}

bool spoofEDID(HKEY hKeyRoot) {
    HKEY hDisplayKey = nullptr;
    std::wstring displayKeyPath = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY";

    if (RegOpenKeyExW(hKeyRoot, displayKeyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hDisplayKey) != ERROR_SUCCESS) {
        std::cerr << "Failed to open DISPLAY registry key." << std::endl;
        return false;
    }

    DWORD subKeysCount = 0;
    if (RegQueryInfoKeyW(hDisplayKey, nullptr, nullptr, nullptr, &subKeysCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) {
        std::cerr << "Failed to query DISPLAY key info." << std::endl;
        RegCloseKey(hDisplayKey);
        return false;
    }

    for (DWORD i = 0; i < subKeysCount; i++) {
        WCHAR subKeyName[256];
        DWORD subKeyNameLen = sizeof(subKeyName) / sizeof(WCHAR);
        if (RegEnumKeyExW(hDisplayKey, i, subKeyName, &subKeyNameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            std::wstring subKeyPath = displayKeyPath + L"\\" + subKeyName;

            HKEY hDeviceKey = nullptr;
            if (RegOpenKeyExW(hKeyRoot, subKeyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hDeviceKey) == ERROR_SUCCESS) {
                DWORD deviceSubKeysCount = 0;
                if (RegQueryInfoKeyW(hDeviceKey, nullptr, nullptr, nullptr, &deviceSubKeysCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                    for (DWORD j = 0; j < deviceSubKeysCount; j++) {
                        WCHAR deviceSubKeyName[256];
                        DWORD deviceSubKeyNameLen = sizeof(deviceSubKeyName) / sizeof(WCHAR);
                        if (RegEnumKeyExW(hDeviceKey, j, deviceSubKeyName, &deviceSubKeyNameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                            std::wstring deviceSubKeyPath = subKeyPath + L"\\" + deviceSubKeyName;
                            std::vector<std::wstring> possiblePaths = {
                                deviceSubKeyPath + L"\\Device Parameters",
                                deviceSubKeyPath + L"\\Control\\Device Parameters",
                                deviceSubKeyPath + L"\\Monitor\\Device Parameters"
                            };
                            for (const auto& path : possiblePaths) {
                                HKEY hEDIDKey = nullptr;
                                std::wstring edidPath = path + L"\\EDID";
                                if (RegOpenKeyExW(hKeyRoot, edidPath.c_str(), 0, KEY_READ | KEY_WRITE, &hEDIDKey) == ERROR_SUCCESS) {
                                    if (spoofRegBinary(hEDIDKey, edidPath, L"EDID")) {
                                        std::cout << "Spoofed -> EDID :: " << wstringToString(path) << std::endl;
                                    }
                                    RegCloseKey(hEDIDKey);
                                }
                            }
                        }
                    }
                }
                RegCloseKey(hDeviceKey);
            }
        }
    }
    RegCloseKey(hDisplayKey);
    return true;
}

void spoofHyperion() {
    std::cout << "\033[38;2;128;0;128m" << "\n========== HYPERION SPOOFING ==========\n" << "\033[0m";

    spoofReg(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control", L"SystemReg");
    FWWMIC("Win32_ComputerSystemProduct", "UUID");
    FWWMIC("Win32_PhysicalMemory", "SerialNumber");
    spoofEDID(HKEY_LOCAL_MACHINE);
    spoofReg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid");
    spoofReg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI", L"LastLoggedOnUser");
    spoofReg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"RegisteredOwner");

    HANDLE tokenHandle = nullptr;
    DWORD bufferSize = 0;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle)) {
        std::cerr << "Failed to open process token." << std::endl;
        return;
    }

    struct HandleReleaser {
        HANDLE handle;
        HandleReleaser(HANDLE h) : handle(h) {}
        ~HandleReleaser() { if (handle) CloseHandle(handle); }
    } handleReleaser(tokenHandle);

    GetTokenInformation(tokenHandle, TokenUser, nullptr, 0, &bufferSize);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        std::cerr << "Failed to query token privileges." << std::endl;
        return;
    }

    std::vector<BYTE> buffer(bufferSize);
    if (!GetTokenInformation(tokenHandle, TokenUser, buffer.data(), bufferSize, &bufferSize)) {
        std::cerr << "Failed to retrieve token info." << std::endl;
        return;
    }

    SID_AND_ATTRIBUTES* sidAndAttributes = reinterpret_cast<SID_AND_ATTRIBUTES*>(buffer.data());
    LPWSTR sidString = nullptr;

    if (ConvertSidToStringSidW(sidAndAttributes->Sid, &sidString)) {
        std::wstring sid(sidString);
        deleteRegistryKey(sid, L"Roblox");
        deleteRegistryKey(sid, L"Hyperion");
        deleteRegistryKey(sid, L"Byfron");
        deleteRegistryKey(sid, L"0SystemReg");
        LocalFree(sidString);
    }
    else {
        std::cerr << "Failed to convert SID to string." << std::endl;
    }
}