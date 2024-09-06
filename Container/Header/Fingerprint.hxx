#ifndef SYSTEMFINGERPRINT_H
#define SYSTEMFINGERPRINT_H

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

#pragma comment(lib, "ntdll.lib")

class SystemFingerprint {
private:
    static constexpr int FingerprintSize = 16;
    unsigned char UniqueFingerprint[FingerprintSize];

    template <typename I>
    std::string n2hexstr(I w, size_t hex_len = sizeof(I) << 1) const;

    void InitializeFingerprint();

    void Interleave(unsigned long Data);

    static std::string Pad4Byte(const std::string& str);

    static std::string GetPhysicalDriveId(DWORD Id);

public:
    SystemFingerprint();

    std::string ToString() const;

    static bool IsNumber(const std::string& s);

    static std::unique_ptr<SystemFingerprint> CreateUniqueFingerprint();
};

#endif