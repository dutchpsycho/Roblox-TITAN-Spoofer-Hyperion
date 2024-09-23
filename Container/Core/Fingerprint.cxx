// @cred to Synapse X 2018 Injection.cpp (base)

/*
* This file is currently not in use
* It hasn't been confirmed that hyperion uses these vectors, this is merely a pre-caution
* This collides with SYNZ's key system
*/

#include "../Header/Fingerprint.hxx"
#include "../Util/Utils.hxx"
#include "../Util/StringEncryption.hxx"

#include <Windows.h>
#include <string>
#include <intrin.h>
#include <cctype>
#include <Lmcons.h>
#include <map>
#include <random>
#include <fstream>
#include <filesystem>
#include <memory>
#include <iostream>

#pragma comment(lib, "ntdll.lib")

template <typename I>
std::string SystemFingerprint::n2hexstr(I w, size_t hex_len) const {
    static const char* digits = "0123456789ABCDEF";
    std::string rc(hex_len, '0');
    for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4) {
        rc[i] = digits[(w >> j) & 0x0f];
    }
    return rc;
}

void SystemFingerprint::InitializeFingerprint() {
    for (int i = 0; i < FingerprintSize - 1; i++) {
        UniqueFingerprint[i] = ~i & 255;
    }
}

void SystemFingerprint::Interleave(unsigned long Data) {
    *(unsigned long*)UniqueFingerprint ^= Data + 0x2EF35C3D;
    *(unsigned long*)(UniqueFingerprint + 4) ^= Data + 0x6E50D365;
    *(unsigned long*)(UniqueFingerprint + 8) ^= Data + 0x73B3E4F9;
    *(unsigned long*)(UniqueFingerprint + 12) ^= Data + 0x1A044581;

    unsigned long OriginalValue = *(unsigned long*)(UniqueFingerprint);
    *(unsigned long*)UniqueFingerprint ^= *(unsigned long*)(UniqueFingerprint + 12);
    *(unsigned long*)(UniqueFingerprint + 12) ^= OriginalValue * 0x3D05F7D1 + *(unsigned long*)UniqueFingerprint;

    UniqueFingerprint[0] = UniqueFingerprint[15] + UniqueFingerprint[14];
    UniqueFingerprint[14] = UniqueFingerprint[0] + UniqueFingerprint[15];
}

std::string SystemFingerprint::Pad4Byte(const std::string& str) {
    const auto s = str.size() + (4 - str.size() % 4) % 4;
    return str.size() < s ? str + std::string(s - str.size(), ' ') : str;
}

std::string SystemFingerprint::GetPhysicalDriveId(DWORD Id) {
    HANDLE H = CreateFileA((std::string("\\\\.\\PhysicalDrive") + std::to_string(Id)).c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (H == INVALID_HANDLE_VALUE) {
        return "Wtf?";
    }

    std::unique_ptr<void, void(*)(HANDLE)> HDevice(H, [](HANDLE handle) { CloseHandle(handle); });

    STORAGE_PROPERTY_QUERY StoragePropQuery{};
    StoragePropQuery.PropertyId = StorageDeviceProperty;
    StoragePropQuery.QueryType = PropertyStandardQuery;

    STORAGE_DESCRIPTOR_HEADER StorageDescHeader{};
    DWORD dwBytesReturned = 0;
    if (!DeviceIoControl(HDevice.get(), IOCTL_STORAGE_QUERY_PROPERTY, &StoragePropQuery, sizeof(STORAGE_PROPERTY_QUERY),
        &StorageDescHeader, sizeof(STORAGE_DESCRIPTOR_HEADER), &dwBytesReturned, NULL)) {
        return "Wtf?";
    }

    const auto OutBufferSize = StorageDescHeader.Size;
    std::unique_ptr<BYTE[]> OutBuffer{ new BYTE[OutBufferSize]{} };
    SecureZeroMemory(OutBuffer.get(), OutBufferSize);

    if (!DeviceIoControl(HDevice.get(), IOCTL_STORAGE_QUERY_PROPERTY, &StoragePropQuery, sizeof(STORAGE_PROPERTY_QUERY),
        OutBuffer.get(), OutBufferSize, &dwBytesReturned, NULL)) {
        return "Wtf?";
    }

    const auto DeviceDescriptor = reinterpret_cast<STORAGE_DEVICE_DESCRIPTOR*>(OutBuffer.get());
    const auto DwSerialNumber = DeviceDescriptor->SerialNumberOffset;
    std::string serial = DwSerialNumber == 0 ? "Unknown" : reinterpret_cast<const char*>(OutBuffer.get() + DwSerialNumber);

    StringObfuscator obfuscator;
    std::string obfuscatedSerial = obfuscator.dynamicObfuscate(serial);

    std::cout << "Spoofed -> Physical Drive Serial :: [ " << obfuscatedSerial << " ]" << std::endl;

    return obfuscatedSerial;
}

SystemFingerprint::SystemFingerprint() {
    ZeroMemory(UniqueFingerprint, FingerprintSize);
}

std::string SystemFingerprint::ToString() const {
    std::string OutString;
    for (int i = 0; i < FingerprintSize - 1; i++) {
        OutString += n2hexstr(UniqueFingerprint[i]);
    }
    return OutString;
}

bool SystemFingerprint::IsNumber(const std::string& s) {
    auto it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

std::unique_ptr<SystemFingerprint> SystemFingerprint::CreateUniqueFingerprint() {
    std::cout << "\033[38;2;255;165;0m" << "\n========== TITAN SUBROUTINE ==========\n" << "\033[0m";

    auto Fingerprint = std::make_unique<SystemFingerprint>();
    Fingerprint->InitializeFingerprint();

    int cpuid_int[4] = { 0, 0, 0, 0 };
    __cpuid(cpuid_int, static_cast<int>(0x80000001));
    int cpuid_sum = cpuid_int[0] + cpuid_int[1] + (cpuid_int[2] | 0x8000) + cpuid_int[3];
    Fingerprint->Interleave(cpuid_sum);
    std::cout << "Spoofed -> CPUID :: [ " << cpuid_sum << " ]" << std::endl;

    DWORD HddNumber = 0;
    GetVolumeInformationA("C://", NULL, NULL, &HddNumber, NULL, NULL, NULL, NULL);
    Fingerprint->Interleave(HddNumber);
    std::cout << "Spoofed -> HDD Serial :: [ " << HddNumber << " ]" << std::endl;

    char ComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD ComputerNameLength = sizeof(ComputerName);
    SecureZeroMemory(ComputerName, ComputerNameLength);
    GetComputerNameA(ComputerName, &ComputerNameLength);
    for (DWORD i = 0; i < ComputerNameLength; i += 4) {
        Fingerprint->Interleave(*reinterpret_cast<unsigned long*>(&ComputerName[i]));
    }

    std::wstring CompHwid;
    if (readReg(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control\\SystemInformation", L"ComputerHardwareId", CompHwid)) {
        std::string CompHwidStr = wstringToString(CompHwid);
        CompHwidStr = Pad4Byte(CompHwidStr);
        for (size_t i = 0; i < CompHwidStr.size(); i += 4) {
            Fingerprint->Interleave(*reinterpret_cast<unsigned long*>(&CompHwidStr[i]));
        }
        std::cout << "Spoofed -> Computer HWID :: [ " << CompHwidStr << " ]" << std::endl;
    }

    auto processRegKey = [&](const std::wstring& subKey, const std::wstring& valueName) {
        std::wstring value;
        if (readReg(HKEY_LOCAL_MACHINE, subKey, valueName, value)) {
            std::string valueStr = wstringToString(value);
            valueStr = Pad4Byte(valueStr);
            for (size_t i = 0; i < valueStr.size(); i += 4) {
                Fingerprint->Interleave(*reinterpret_cast<unsigned long*>(&valueStr[i]));
            }
            std::cout << "Spoofed -> " << wstringToString(valueName) << " :: [ " << valueStr << " ]" << std::endl;
        }
    };

    processRegKey(L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSVendor");
    processRegKey(L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"BIOSReleaseDate");
    processRegKey(L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemManufacturer");
    processRegKey(L"HARDWARE\\DESCRIPTION\\System\\BIOS", L"SystemProductName");

    std::wstring ProcessorName;
    if (readReg(HKEY_LOCAL_MACHINE, L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", L"ProcessorNameString", ProcessorName)) {
        std::string ProcessorNameStr = wstringToString(ProcessorName);
        ProcessorNameStr = Pad4Byte(ProcessorNameStr);
        for (size_t i = 0; i < ProcessorNameStr.size(); i += 4) {
            Fingerprint->Interleave(*reinterpret_cast<unsigned long*>(&ProcessorNameStr[i]));
        }
        std::cout << "Spoofed -> CPU Name :: [ " << ProcessorNameStr << " ]" << std::endl;
    }

    std::string Id = Pad4Byte(GetPhysicalDriveId(0));
    for (size_t i = 0; i < Id.size(); i += 4) {
        Fingerprint->Interleave(*reinterpret_cast<unsigned long*>(&Id[i]));
    }
    std::cout << "Spoofed -> Physical Driver ID :: [ " << Id << " ]" << std::endl;

    return Fingerprint;
}