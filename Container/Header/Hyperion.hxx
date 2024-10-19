// Hyperion.hxx

#ifndef HYPERION_HXX
#define HYPERION_HXX

#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)

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

void HookNtDll();
bool spoofReg(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName);
bool FWWMIC(const std::string& wmicCommand, const std::string& propertyName);
void spoofHyperion();

#endif