#include "../Header/Hyperion.hxx"

#include "../Util/Utils.hxx"

#include <windows.h>
#include <winternl.h>
#include <Wbemidl.h>
#include <comdef.h>
#include <sddl.h>
#include <vector>
#include <iostream>
#include <string>
#include <fstream>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "wbemuuid.lib")

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
pNtDeleteKey NtDeleteKey = nullptr;
pRtlInitUnicodeString RtlInitUnicodeStringPtr = nullptr;
pNtClose NtClosePtr = nullptr;
pNtSetInformationProcess NtSetInformationProcess = nullptr;

BYTE OriginalBytesNtSetInformationProcess[16];
BYTE* pOriginalCodeNtSetInformationProcess = nullptr;

void iniNtFunctions() {
    HMODULE hNtDll = GetModuleHandleA("ntdll.dll");
    if (hNtDll) {
        NtOpenKey = (pNtOpenKey)GetProcAddress(hNtDll, "NtOpenKey");
        NtDeleteKey = (pNtDeleteKey)GetProcAddress(hNtDll, "NtDeleteKey");

#ifndef RtlInitUnicodeString
        RtlInitUnicodeStringPtr = (pRtlInitUnicodeString)GetProcAddress(hNtDll, "RtlInitUnicodeString");
#endif

#ifndef NtClose
        NtClosePtr = (pNtClose)GetProcAddress(hNtDll, "NtClose");
#endif

        NtSetInformationProcess = (pNtSetInformationProcess)GetProcAddress(hNtDll, "NtSetInformationProcess");
    }
}

NTSTATUS NTAPI HookedNtSetInformationProcess(
    HANDLE ProcessHandle,
    PROCESS_INFORMATION_CLASS ProcessInformationClass,
    PVOID ProcessInformation,
    ULONG ProcessInformationLength) {
    if (ProcessInformationClass == ProcessDebugPort) {
        std::cout << "ProcessDebugPort hook triggered, hiding debug port" << std::endl;
        ProcessInformation = NULL;
        ProcessInformationLength = 0;
        return STATUS_SUCCESS;
    }
    memcpy(pOriginalCodeNtSetInformationProcess, OriginalBytesNtSetInformationProcess, sizeof(OriginalBytesNtSetInformationProcess));
    NTSTATUS result = ((pNtSetInformationProcess)pOriginalCodeNtSetInformationProcess)(
        ProcessHandle, ProcessInformationClass, ProcessInformation, ProcessInformationLength);
    DWORD oldProtect;
    VirtualProtect(pOriginalCodeNtSetInformationProcess, sizeof(OriginalBytesNtSetInformationProcess), PAGE_EXECUTE_READWRITE, &oldProtect);
    pOriginalCodeNtSetInformationProcess[0] = 0xE9;
    *(DWORD*)(pOriginalCodeNtSetInformationProcess + 1) = (DWORD)((BYTE*)HookedNtSetInformationProcess - pOriginalCodeNtSetInformationProcess - 5);
    VirtualProtect(pOriginalCodeNtSetInformationProcess, sizeof(OriginalBytesNtSetInformationProcess), oldProtect, &oldProtect);
    return result;
}

void Inline(BYTE* targetFunction, BYTE* hookFunction, BYTE* originalBytes, size_t byteCount) {
    memcpy(originalBytes, targetFunction, byteCount);
    DWORD offset = (DWORD)(hookFunction - targetFunction - 5);
    DWORD oldProtect;
    VirtualProtect(targetFunction, byteCount, PAGE_EXECUTE_READWRITE, &oldProtect);
    targetFunction[0] = 0xE9;
    *(DWORD*)(targetFunction + 1) = offset;
    VirtualProtect(targetFunction, byteCount, oldProtect, &oldProtect);
}

void HookNtDll() {
    iniNtFunctions();
    if (NtSetInformationProcess) {
        pOriginalCodeNtSetInformationProcess = (BYTE*)NtSetInformationProcess;
        Inline(pOriginalCodeNtSetInformationProcess, (BYTE*)HookedNtSetInformationProcess, OriginalBytesNtSetInformationProcess, sizeof(OriginalBytesNtSetInformationProcess));
        std::cout << "Hooked NtSetInformationProcess" << std::endl;
    }
    else {
        std::cerr << "Failed to retrieve NtSetInformationProcess" << std::endl;
    }
}

bool PermCheck() {
    SC_HANDLE hSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
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

    if (!QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)&ssStatus, sizeof(SERVICE_STATUS_PROCESS), &dwBytesNeeded)) {
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

    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        std::cerr << "Failed to open process token" << std::endl;
        return false;
    }

    DWORD dwSize = 0;
    GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwSize);

    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        std::cerr << "Failed to query token privs" << std::endl;
        CloseHandle(hToken);
        return false;
    }

    std::vector<BYTE> buffer(dwSize);
    if (!GetTokenInformation(hToken, TokenPrivileges, &buffer[0], dwSize, &dwSize)) {
        std::cerr << "Failed to retrieve token info" << std::endl;
        CloseHandle(hToken);
        return false;
    }

    CloseHandle(hToken);
    return true;
}

bool FWWMIC(const std::string& wmicCommand, const std::string& propertyName) {
    if (!PermCheck()) {
        std::cerr << "no permissions or WMI service isnt on" << std::endl;
        return false;
    }

    HRESULT hres;
    IWbemLocator* pLoc = nullptr;
    IWbemServices* pSvc = nullptr;
    IEnumWbemClassObject* pEnumerator = nullptr;
    IWbemClassObject* pclsObj = nullptr;
    ULONG uReturn = 0;

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres)) return false;

    hres = CoInitializeSecurity(NULL, -1, NULL, NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
    if (FAILED(hres)) { CoUninitialize(); return false; }

    hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
    if (FAILED(hres) || !pLoc) {
        CoUninitialize();
        return false;
    }

    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
    if (FAILED(hres) || !pSvc) {
        if (pLoc) pLoc->Release();
        CoUninitialize();
        return false;
    }

    hres = pSvc->ExecQuery(
        bstr_t("WQL"), bstr_t("SELECT * FROM __NAMESPACE WHERE Name = 'CIMV2'"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres) || !pEnumerator) {
        std::cerr << "ROOT\\CIMV2 namespace doesnt exist or I cant access it" << std::endl;
        if (pSvc) pSvc->Release();
        if (pLoc) pLoc->Release();
        CoUninitialize();
        return false;
    }

    if (pEnumerator) pEnumerator->Release();
    pEnumerator = nullptr;

    hres = CoSetProxyBlanket(
        pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL,
        RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
    if (FAILED(hres)) {
        if (pSvc) pSvc->Release();
        if (pLoc) pLoc->Release();
        CoUninitialize();
        return false;
    }

    hres = pSvc->ExecQuery(
        bstr_t("WQL"), bstr_t(wmicCommand.c_str()),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
    if (FAILED(hres) || !pEnumerator) {
        if (pSvc) pSvc->Release();
        if (pLoc) pLoc->Release();
        CoUninitialize();
        return false;
    }

    while (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == WBEM_S_NO_ERROR) {
        VARIANT vtProp;
        VariantInit(&vtProp);

        hres = pclsObj->Get(_bstr_t(propertyName.c_str()), 0, &vtProp, 0, 0);
        if (SUCCEEDED(hres)) {
            std::string newValue = randstring(8) + "-" + randstring(4) + "-" + randstring(4) + "-" + randstring(4) + "-" + randstring(12);
            VARIANT vtNewVal;
            VariantInit(&vtNewVal);

            vtNewVal.vt = VT_BSTR;
            vtNewVal.bstrVal = _bstr_t(newValue.c_str());

            hres = pclsObj->Put(_bstr_t(propertyName.c_str()), 0, &vtNewVal, 0);
            if (SUCCEEDED(hres)) {
                std::cout << "Spoofed -> " << propertyName << " :: [ " << newValue << " ]" << std::endl;
            }

            VariantClear(&vtNewVal);
        }

        VariantClear(&vtProp);
        if (pclsObj) pclsObj->Release();
        pclsObj = nullptr;
    }

    if (pEnumerator) pEnumerator->Release();
    if (pSvc) pSvc->Release();
    if (pLoc) pLoc->Release();

    CoUninitialize();
    return true;
}

bool spoofReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName) {
    HKEY hKey;
    if (RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        std::cerr << "Failed to open Registry key: " << wstringToString(subKey) << std::endl;
        return false;
    }

    DWORD dataSize = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), NULL, NULL, NULL, reinterpret_cast<DWORD*>(&dataSize)) != ERROR_SUCCESS || dataSize == 0) {
        RegCloseKey(hKey);
        return false;
    }

    std::wstring originalValue(dataSize / sizeof(wchar_t), L'\0');
    if (RegQueryValueExW(hKey, valueName.c_str(), NULL, NULL, reinterpret_cast<LPBYTE>(&originalValue[0]), &dataSize) != ERROR_SUCCESS) {
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
    HKEY hKey;
    if (RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_READ | KEY_WRITE, &hKey) != ERROR_SUCCESS) {
        std::cerr << "Failed to open Registry key: " << wstringToString(subKey) << std::endl;
        return false;
    }

    DWORD dataSize = 0;
    if (RegQueryValueExW(hKey, valueName.c_str(), NULL, NULL, NULL, reinterpret_cast<DWORD*>(&dataSize)) != ERROR_SUCCESS || dataSize == 0) {
        std::cerr << "Failed to query Registry value: " << wstringToString(subKey) << "\\" << wstringToString(valueName) << std::endl;
        RegCloseKey(hKey);
        return false;
    }

    std::vector<BYTE> data(dataSize);
    if (RegQueryValueExW(hKey, valueName.c_str(), NULL, NULL, data.data(), &dataSize) != ERROR_SUCCESS) {
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
    if (!NtOpenKey || !NtDeleteKey || !RtlInitUnicodeStringPtr || !NtClosePtr) {
        std::cerr << "Failed to load Nt functions" << std::endl;
        return;
    }

    HKEY hKey;
    std::wstring keyPath = L"\\Registry\\User\\" + sid + L"\\System\\CurrentControlSet\\Control\\" + subKeyName;

    UNICODE_STRING unicodeKey;
    RtlInitUnicodeStringPtr(&unicodeKey, keyPath.c_str());

    OBJECT_ATTRIBUTES objectAttributes;
    InitializeObjectAttributes(&objectAttributes, &unicodeKey, OBJ_CASE_INSENSITIVE, NULL, NULL);

    NTSTATUS status = NtOpenKey(reinterpret_cast<PHANDLE>(&hKey), KEY_ALL_ACCESS, &objectAttributes);

    if (status == STATUS_SUCCESS) {
        status = NtDeleteKey(hKey);
        if (status == STATUS_SUCCESS) {
            std::wcout << L"Removed Registry Key -> " << keyPath << std::endl;
        }
        NtClosePtr(hKey);
    }
}

bool spoofEDID(HKEY hKeyRoot) {
    HKEY hDisplayKey;
    std::wstring displayKeyPath = L"SYSTEM\\CurrentControlSet\\Enum\\DISPLAY";

    if (RegOpenKeyExW(hKeyRoot, displayKeyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hDisplayKey) != ERROR_SUCCESS) {
        std::cerr << "Failed to open DISPLAY reg key." << std::endl;
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
        DWORD subKeyNameLen = 256;
        if (RegEnumKeyExW(hDisplayKey, i, subKeyName, &subKeyNameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            std::wstring subKeyPath = displayKeyPath + L"\\" + subKeyName;

            HKEY hDeviceKey;
            if (RegOpenKeyExW(hKeyRoot, subKeyPath.c_str(), 0, KEY_READ | KEY_WRITE, &hDeviceKey) == ERROR_SUCCESS) {
                DWORD deviceSubKeysCount = 0;
                if (RegQueryInfoKeyW(hDeviceKey, nullptr, nullptr, nullptr, &deviceSubKeysCount, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                    for (DWORD j = 0; j < deviceSubKeysCount; j++) {
                        WCHAR deviceSubKeyName[256];
                        DWORD deviceSubKeyNameLen = 256;
                        if (RegEnumKeyExW(hDeviceKey, j, deviceSubKeyName, &deviceSubKeyNameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                            std::wstring deviceSubKeyPath = subKeyPath + L"\\" + deviceSubKeyName;
                            std::vector<std::wstring> possiblePaths = {
                                deviceSubKeyPath + L"\\Device Parameters",
                                deviceSubKeyPath + L"\\Control\\Device Parameters",
                                deviceSubKeyPath + L"\\Monitor\\Device Parameters"
                            };
                            for (const auto& path : possiblePaths) {
                                HKEY hEDIDKey;
                                if (RegOpenKeyExW(hKeyRoot, (path + L"\\EDID").c_str(), 0, KEY_READ | KEY_WRITE, &hEDIDKey) == ERROR_SUCCESS) {
                                    if (spoofRegBinary(hEDIDKey, L"EDID", L"EDID")) {
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
    spoofReg(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control", L"SystemReg"); // SysReg
    FWWMIC("SELECT UUID FROM Win32_ComputerSystemProduct", "UUID"); // UUID
    FWWMIC("SELECT SerialNumber FROM Win32_PhysicalMemory", "SerialNumber"); // Physical Serial Num
    spoofEDID(HKEY_LOCAL_MACHINE); // E-DID
    spoofReg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Cryptography", L"MachineGuid"); // Machine GUID
    spoofReg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Authentication\\LogonUI", L"LastLoggedOnUser"); // Current User
    spoofReg(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"RegisteredOwner"); // Current User 2 

    HANDLE tokenHandle = NULL;
    DWORD bufferSize = 0;
    std::vector<SID_AND_ATTRIBUTES> sidList;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle)) {
        std::cerr << "Failed to open process token" << std::endl;
        return;
    }

    GetTokenInformation(tokenHandle, TokenUser, NULL, 0, &bufferSize);
    if (bufferSize > 0) {
        std::vector<BYTE> buffer(bufferSize);
        if (GetTokenInformation(tokenHandle, TokenUser, buffer.data(), bufferSize, &bufferSize)) {
            SID_AND_ATTRIBUTES* sidAndAttributes = reinterpret_cast<SID_AND_ATTRIBUTES*>(buffer.data());
            LPWSTR sidString = NULL;

            if (ConvertSidToStringSidW(sidAndAttributes->Sid, &sidString)) {
                std::wstring sid(sidString);
                deleteRegistryKey(sid, L"Roblox");
                deleteRegistryKey(sid, L"Hyperion");
                deleteRegistryKey(sid, L"Byfron");
                deleteRegistryKey(sid, L"0SystemReg");
                LocalFree(sidString);
            }
            else {
                std::cerr << "Failed to convert SID to string" << std::endl;
            }
        }
        else {
            std::cerr << "Failed to get token information" << std::endl;
        }
    }
    else {
        std::cerr << "Where is your SID?" << std::endl;
    }
    CloseHandle(tokenHandle);
}