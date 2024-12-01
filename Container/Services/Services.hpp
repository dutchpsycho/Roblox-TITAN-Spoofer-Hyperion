#pragma once

#include "Defs.h"

#include <Windows.h>
#include <TlHelp32.h>

#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <array>

namespace Services {

    // CONVERSION

    inline std::string toUtf8(const std::wstring& wstr) {
        std::string result;
        int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
        if (sizeNeeded > 0) {
            result.resize(sizeNeeded - 1);
            WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], sizeNeeded, nullptr, nullptr);
        }
        return result;
    }

    inline std::wstring stringToWString(const std::string& str) {
        return std::wstring(str.begin(), str.end());
    }


    // GENERATORS

    static thread_local std::mt19937 gen(std::random_device{}());

    inline std::wstring genRand(size_t length = 12) {
        constexpr std::wstring_view chars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);

        std::wstring rUser;
        rUser.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            rUser.push_back(chars[dist(gen)]);
        }
        return rUser;
    }

    inline std::string genMac() {
        std::uniform_int_distribution<int> dist(0, 255);

        std::array<unsigned char, 6> mac = {};
        for (auto& byte : mac) {
            byte = static_cast<unsigned char>(dist(gen));
        }
        mac[0] &= 0xFE;

        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

        return std::string(macStr);
    }

    inline std::string genGUID() {
        std::uniform_int_distribution<uint32_t> dist32(0, 0xFFFFFFFF);
        std::uniform_int_distribution<uint16_t> dist16(0, 0xFFFF);

        uint32_t data1 = dist32(gen);
        uint16_t data2 = dist16(gen);
        uint16_t data3 = (dist16(gen) & 0x0FFF) | 0x4000; // ver 4 GUID
        uint16_t data4 = (dist16(gen) & 0x3FFF) | 0x8000; // var 1 GUID

        char guidStr[37];
        snprintf(guidStr, sizeof(guidStr), "%08X-%04X-%04X-%04X-%012llX",
            data1, data2, data3, data4,
            ((static_cast<uint64_t>(dist32(gen)) << 16) | dist16(gen)));

        return std::string(guidStr);
    }

    inline std::string genSerial() {
        std::uniform_int_distribution<int> dist(0, 9);

        std::string serialNumber;
        serialNumber.reserve(12);
        for (int i = 0; i < 12; ++i) {
            serialNumber.push_back('0' + dist(gen));
        }
        return serialNumber;
    }

    inline std::wstring genEDID() {
        std::uniform_int_distribution<int> dist(0, 0xFFFF);
        std::uniform_int_distribution<int> uidDist(10000, 99999);

        std::wostringstream idStream;
        idStream << L"5&"
            << std::hex << dist(gen)
            << dist(gen)
            << L"&0&UID"
            << uidDist(gen);

        return idStream.str();
    }

    inline std::wstring rndWindName(size_t length = 24) {
        constexpr std::wstring_view chars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);

        std::wstring rndName;
        rndName.reserve(length);
        for (size_t i = 0; i < length; ++i) {
            rndName.push_back(chars[dist(gen)]);
        }
        return rndName;
    }


    // NTDLL

    static HMODULE GetNtdll() {
        static HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        return ntdll;
    }

    static auto GetNtTerminateProcess() {
        static auto NtTerminateProcess = reinterpret_cast<NTSTATUS(WINAPI*)(HANDLE, NTSTATUS)>(
            GetProcAddress(GetNtdll(), "NtTerminateProcess")
            );
        return NtTerminateProcess;
    }

    static auto GetNtOpenKey() {
        static auto NtOpenKey = reinterpret_cast<NTSTATUS(WINAPI*)(PHANDLE, ACCESS_MASK, POBJECT_ATTRIBUTES)>(
            GetProcAddress(GetNtdll(), "NtOpenKey")
            );
        return NtOpenKey;
    }

    static auto GetNtDeleteKey() {
        static auto NtDeleteKey = reinterpret_cast<NTSTATUS(WINAPI*)(HANDLE)>(
            GetProcAddress(GetNtdll(), "NtDeleteKey")
            );
        return NtDeleteKey;
    }

    static auto GetNtClose() {
        static auto NtClose = reinterpret_cast<NTSTATUS(WINAPI*)(HANDLE)>(
            GetProcAddress(GetNtdll(), "NtClose")
            );
        return NtClose;
    }

    inline HANDLE OpenKey(const std::wstring_view& keyPath, ACCESS_MASK desiredAccess) {
        auto NtOpenKey = GetNtOpenKey();
        if (!NtOpenKey) return nullptr;

        UNICODE_STRING unicodeKeyPath = {};
        OBJECT_ATTRIBUTES objectAttributes = {};

        RtlInitUnicodeString(&unicodeKeyPath, keyPath.data());
        InitializeObjectAttributes(&objectAttributes, &unicodeKeyPath, OBJ_CASE_INSENSITIVE, nullptr, nullptr);

        HANDLE keyHandle = nullptr;
        NTSTATUS status = NtOpenKey(&keyHandle, desiredAccess, &objectAttributes);

        return NT_SUCCESS(status) ? keyHandle : nullptr;
    }

    inline void CloseKey(HANDLE keyHandle) {
        auto NtClose = GetNtClose();
        if (!NtClose || !keyHandle) return;

        NtClose(keyHandle);
    }

    inline bool DelKey(const std::wstring_view& keyPath) {
        HANDLE keyHandle = OpenKey(keyPath, DELETE);
        if (!keyHandle) return false;

        auto NtDeleteKey = GetNtDeleteKey();
        if (!NtDeleteKey) {
            CloseKey(keyHandle);
            return false;
        }

        NTSTATUS status = NtDeleteKey(keyHandle);
        CloseKey(keyHandle);

        return NT_SUCCESS(status);
    }

    static auto GetNtQueryKey() {
        static auto NtQueryKey = reinterpret_cast<NTSTATUS(WINAPI*)(
            HANDLE, KEY_INFORMATION_CLASS, PVOID, ULONG, PULONG)>(
                GetProcAddress(GetNtdll(), "NtQueryKey")
                );
        return NtQueryKey;
    }

    static auto GetNtSetValueKey() {
        static auto NtSetValueKey = reinterpret_cast<NTSTATUS(WINAPI*)(
            HANDLE, PUNICODE_STRING, ULONG, ULONG, const void*, ULONG)>(
                GetProcAddress(GetNtdll(), "NtSetValueKey")
                );
        return NtSetValueKey;
    }

    static auto GetNtEnumerateKey() {
        static auto NtEnumerateKey = reinterpret_cast<NTSTATUS(WINAPI*)(
            HANDLE, ULONG, KEY_INFORMATION_CLASS, PVOID, ULONG, PULONG)>(
                GetProcAddress(GetNtdll(), "NtEnumerateKey")
                );
        return NtEnumerateKey;
    }


    // HELPERS

    inline void SetWindow() {
        std::wstring rndName = rndWindName();
        if (!SetConsoleTitleW(rndName.c_str())) {}
    }

    inline void KillRbx() {
        SetWindow();

        const std::wstring_view names[] = { L"RobloxPlayerBeta.exe", L"RobloxCrashHandler.exe", L"Bloxstrap.exe" };
        const std::wstring_view titles = L"Roblox";

        auto NtTerminateProcess = GetNtTerminateProcess();
        if (!NtTerminateProcess) return;

        HWND hwnd = FindWindowW(nullptr, titles.data());
        if (hwnd) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            if (pid) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
                if (hProcess) {
                    NtTerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        }

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32W entry = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(snapshot, &entry)) {
            do {
                for (const auto& target : names) {
                    if (target == entry.szExeFile) {
                        HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
                        if (hProcess) {
                            NtTerminateProcess(hProcess, 0);
                            CloseHandle(hProcess);
                        }
                    }
                }
            } while (Process32NextW(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    

    // VISUALS

    inline void SectHeader(const std::string& sectionName, int colorCode) {
        std::cout << "\033[38;5;" << colorCode << "m"
            << "\n============ " << sectionName << " ============\n"
            << "\033[0m";
    }

#pragma warning(disable : 4566)

    inline void TITAN() {
        std::string art = R"(
       ++x                                             +++x              
        +;++x           TITAN Spoofer V1.1          +++;+x               
         +;;;;++          Swedish.Psycho         +++;;;++                
         +;::::;;++                           ++;;;;;;;+                 
          +:::::::;; ++                   ++x;;:::::::+                  
          +;:::::::;  ++++             x++++ ;;::::::;+                  
           +;::::::;;  ;;;;++       ++;;;;; x;:::::::+                   
           +;::::::;;  +;::::;;   ;;::::;+  ;;::::::;+                   
            +:::::::;  +;:::::;   ;:::::;+  ;;::::::++                   
            +;::::::;;  ;:::::;   ;:::::;+  ;::::::;+                    
             +::::::;;  ;;::::;x  ;:::::;  ;;::::::++                    
             +;:::::;+  +;::::;+  ;::::;;  ;;:::::;+                     
              ;::::::;  +;::::;+  ;::::;+  +::::::;+                     
              +;:::::;  ;;::::;;  ;::::;+  ;:::::++                      
                +;:::;  +;::::;; x;::::;;  ;::::+                        
                 +;::;;  ;::::;; +;::::;;  ;::;+                         
                  +;:;;  ;;:::;; +;::::;  ;;:;+                          
                    +;;  ;;:::;; ;;:::;;  ;;+                            
                     x;+ ;;:::;; ;;:::;;  ;+                             
                         +;:::;; ;;:::;+                                 
                          ;:::;; ;;:::;+                                 
                          ;;::;; ;;:::;                                  
                          ;;::;; ;;:::;                                  
                          ;:::;; ;;:::;+                                 
                          +;:::; ;::::+                                  
                          ++:::;x;:::;+                                  
                           +;:::;;::;+                                   
                            +;::;;;;+                                    
                             ;;;;;;;+                                    
                             +;;;;;+                                     
                              +;;;+                                      
                               +;;x                                      
                               +;+                                       
                                +                                        
    )";
        std::cout << art;
    }
}