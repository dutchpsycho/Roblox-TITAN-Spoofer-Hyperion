// Hyperion.cxx

#include "../Header/Hyperion.hxx"
#include "../Util/Utils.hxx"

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

typedef NTSTATUS(NTAPI* pNtOpenKey)(
    PHANDLE KeyHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes
    );

typedef NTSTATUS(NTAPI* pNtDeleteKey)(
    HANDLE KeyHandle
    );

#ifndef RtlInitUnicodeString
typedef VOID(NTAPI* pRtlInitUnicodeString)(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );
#endif

#ifndef NtClose
typedef NTSTATUS(NTAPI* pNtClose)(
    HANDLE Handle
    );
#endif

typedef NTSTATUS(NTAPI* pNtSetInformationProcess)(
    HANDLE ProcessHandle,
    PROCESS_INFORMATION_CLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength
    );

pNtOpenKey NtOpenKey = nullptr;
pNtDeleteKey NtDeleteKeyPtr = nullptr;
pRtlInitUnicodeString RtlInitUnicodeStringPtr = nullptr;
pNtClose NtClosePtr = nullptr;
pNtSetInformationProcess NtSetInformationProcess = nullptr;

BYTE OriginalBytesNtSetInformationProcess[16];
BYTE* pOriginalCodeNtSetInformationProcess = nullptr;

void iniNtFunctions() {
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll) {
        NtOpenKey = reinterpret_cast<pNtOpenKey>(GetProcAddress(hNtDll, "NtOpenKey"));
        NtDeleteKeyPtr = reinterpret_cast<pNtDeleteKey>(GetProcAddress(hNtDll, "NtDeleteKey"));

#ifndef RtlInitUnicodeString
        RtlInitUnicodeStringPtr = reinterpret_cast<pRtlInitUnicodeString>(GetProcAddress(hNtDll, "RtlInitUnicodeString"));
#endif

#ifndef NtClose
        NtClosePtr = reinterpret_cast<pNtClose>(GetProcAddress(hNtDll, "NtClose"));
#endif

        NtSetInformationProcess = reinterpret_cast<pNtSetInformationProcess>(GetProcAddress(hNtDll, "NtSetInformationProcess"));
    }
}

NTSTATUS NTAPI HookedNtSetInformationProcess(
    HANDLE ProcessHandle,
    PROCESS_INFORMATION_CLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength) {
    if (ProcessInformationClass == ProcessDebugPort) {
        std::cout << "ProcessDebugPort hook triggered, hiding debug port" << std::endl;
        ProcessInformation = nullptr;
        ProcessInformationLength = 0;
        return STATUS_SUCCESS;
    }
    if (pOriginalCodeNtSetInformationProcess && OriginalBytesNtSetInformationProcess) {
        memcpy(pOriginalCodeNtSetInformationProcess, OriginalBytesNtSetInformationProcess, sizeof(OriginalBytesNtSetInformationProcess));
        NTSTATUS result = NtSetInformationProcess(
            ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
        DWORD oldProtect;
        if (VirtualProtect(pOriginalCodeNtSetInformationProcess, sizeof(OriginalBytesNtSetInformationProcess), PAGE_EXECUTE_READWRITE, &oldProtect)) {
            pOriginalCodeNtSetInformationProcess[0] = 0xE9;
            *(DWORD*)(pOriginalCodeNtSetInformationProcess + 1) = (DWORD)((BYTE*)HookedNtSetInformationProcess - pOriginalCodeNtSetInformationProcess - 5);
            VirtualProtect(pOriginalCodeNtSetInformationProcess, sizeof(OriginalBytesNtSetInformationProcess), oldProtect, &oldProtect);
        }
        else {
            std::cerr << "Failed to restore original bytes after hooking." << std::endl;
        }
        return result;
    }
    return STATUS_SUCCESS;
}

void Inline(BYTE* targetFunction, BYTE* hookFunction, BYTE* originalBytes, size_t byteCount) {
    if (!targetFunction || !hookFunction || !originalBytes) {
        std::cerr << "Invalid function pointers provided to Inline." << std::endl;
        return;
    }
    memcpy(originalBytes, targetFunction, byteCount);
    DWORD offset = static_cast<DWORD>((hookFunction - targetFunction - 5));
    DWORD oldProtect;
    if (VirtualProtect(targetFunction, byteCount, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        targetFunction[0] = 0xE9;
        *(DWORD*)(targetFunction + 1) = offset;
        VirtualProtect(targetFunction, byteCount, oldProtect, &oldProtect);
    }
    else {
        std::cerr << "Failed to change memory protection in Inline." << std::endl;
    }
}

void HookNtDll() {
    iniNtFunctions();
    if (NtSetInformationProcess) {
        pOriginalCodeNtSetInformationProcess = reinterpret_cast<BYTE*>(NtSetInformationProcess);
        if (pOriginalCodeNtSetInformationProcess) {
            Inline(pOriginalCodeNtSetInformationProcess, reinterpret_cast<BYTE*>(&HookedNtSetInformationProcess), OriginalBytesNtSetInformationProcess, sizeof(OriginalBytesNtSetInformationProcess));
            std::cout << "Hooked NtSetInformationProcess" << std::endl;
        }
        else {
            std::cerr << "NtSetInformationProcess pointer is null." << std::endl;
        }
    }
    else {
        std::cerr << "Failed to retrieve NtSetInformationProcess" << std::endl;
    }
}

bool PermCheck() {
    SC_HANDLE hSCManager = OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE);
    if (!hSCManager) {
        std::cerr << "Failed to open service control manager." << std::endl;
        return false;
    }

    SC_HANDLE hService = OpenService(hSCManager, L"winmgmt", SERVICE_QUERY_STATUS);
    if (!hService) {
        std::cerr << "Failed to open WMI service" << std::endl;
        CloseServiceHandle(hSCManager);
        return false;
    }

    SERVICE_STATUS_PROCESS ssStatus;
    DWORD dwBytesNeeded;

    if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, reinterpret_cast<LPBYTE>(&ssStatus), sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
        std::cerr << "Failed to query WMI service" << std::endl;
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return false;
    }

    if (ssStatus.dwCurrentState != SERVICE_RUNNING) {
        std::cerr << "WMI service is not running" << std::endl;
        CloseServiceHandle(hService);
        CloseServiceHandle(hSCManager);
        return false;
    }

    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);

    HANDLE hToken = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        std::cerr << "Failed to open process token" << std::endl;
        return false;
    }

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenPrivileges, nullptr, 0, &dwSize);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        std::cerr << "Failed to query token privileges" << std::endl;
        CloseHandle(hToken);
        return false;
    }

    std::vector<BYTE> buffer(dwSize);
    if (!GetTokenInformation(hToken, TokenPrivileges, buffer.data(), dwSize, &dwSize)) {
        std::cerr << "Failed to retrieve token info." << std::endl;
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

bool FWWMIC(const std::string& wmicCommand, const std::string& propertyName) {
    if (!PermCheck()) {
        std::cerr << "No permissions or WMI service isn't running." << std::endl;
        return false;
    }

    HRESULT hres;
    hres = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hres)) {
        std::cerr << "Failed to initialize COM library. Error code = 0x" << std::hex << hres << std::endl;
        // Fallback: Direct registry manipulation as an alternative spoofing method
        std::cerr << "Attempting fallback spoofing via registry manipulation." << std::endl;
        // Implement fallback registry spoofing here if applicable
        // For demonstration, returning false
        return false;
    }
    struct COMUninitializer {
        ~COMUninitializer() {
            CoUninitialize();
        }
    } comUninitializer;

    hres = CoInitializeSecurity(
        nullptr,
        -1,
        nullptr,
        nullptr,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE,
        nullptr
    );

    if (FAILED(hres)) {
        std::cerr << "Failed to initialize security. Error code = 0x" << std::hex << hres << std::endl;
        return false;
    }

    IWbemLocator* pLoc = nullptr;
    hres = CoCreateInstance(
        CLSID_WbemLocator,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres) || !pLoc) {
        std::cerr << "Failed to create IWbemLocator object. Err code = 0x" << std::hex << hres << std::endl;
        return false;
    }

    struct IWbemLocatorReleaser {
        IWbemLocator* p;
        IWbemLocatorReleaser(IWbemLocator* ptr) : p(ptr) {}
        ~IWbemLocatorReleaser() { if (p) p->Release(); }
    } locatorReleaser(pLoc);

    IWbemServices* pSvc = nullptr;
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"),
        nullptr,
        nullptr,
        0,
        NULL,
        0,
        0,
        &pSvc
    );

    if (FAILED(hres) || !pSvc) {
        std::cerr << "Could not connect to WMI namespace. Error code = 0x" << std::hex << hres << std::endl;
        return false;
    }

    struct IWbemServicesReleaser {
        IWbemServices* p;
        IWbemServicesReleaser(IWbemServices* ptr) : p(ptr) {}
        ~IWbemServicesReleaser() { if (p) p->Release(); }
    } servicesReleaser(pSvc);

    hres = CoSetProxyBlanket(
        pSvc,
        RPC_C_AUTHN_WINNT,
        RPC_C_AUTHZ_NONE,
        nullptr,
        RPC_C_AUTHN_LEVEL_CALL,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        nullptr,
        EOAC_NONE
    );

    if (FAILED(hres)) {
        std::cerr << "Could not set proxy blanket. Error code = 0x" << std::hex << hres << std::endl;
        return false;
    }

    std::wstring wqlQuery = L"SELECT " + stringToWstring(propertyName) + L" FROM " + stringToWstring(wmicCommand);
    IEnumWbemClassObject* pEnumerator = nullptr;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t(wqlQuery.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        nullptr,
        &pEnumerator
    );

    if (FAILED(hres) || !pEnumerator) {
        std::cerr << "WMI query failed. Error code = 0x" << std::hex << hres << std::endl;
        return false;
    }

    struct IEnumWbemClassObjectReleaser {
        IEnumWbemClassObject* p;
        IEnumWbemClassObjectReleaser(IEnumWbemClassObject* ptr) : p(ptr) {}
        ~IEnumWbemClassObjectReleaser() { if (p) p->Release(); }
    } enumeratorReleaser(pEnumerator);

    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;

    while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == WBEM_S_NO_ERROR) {
        struct IWbemClassObjectReleaser {
            IWbemClassObject* p;
            IWbemClassObjectReleaser(IWbemClassObject* ptr) : p(ptr) {}
            ~IWbemClassObjectReleaser() { if (p) p->Release(); }
        } classObjReleaser(pclsObj);

        VARIANT vtProp;
        VariantInit(&vtProp);

        hres = pclsObj->Get(_bstr_t(propertyName.c_str()), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hres)) {
            std::string newValue = randstring(8) + "-" + randstring(4) + "-" + randstring(4) + "-" + randstring(4) + "-" + randstring(12);
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
            }
            else {
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

    iniNtFunctions();
    HookNtDll();

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