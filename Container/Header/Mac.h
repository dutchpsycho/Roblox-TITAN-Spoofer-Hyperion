#pragma once

#include "../Header/COM.h"
#include "../Services/Services.hpp"

#include <windows.h>
#include <comdef.h>
#include <wbemidl.h>
#include <iphlpapi.h>

#include <string>
#include <vector>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <thread>
#include <mutex>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace MAC {

    class MacSpoofer {
    public:
        static void run();

    private:
        static std::vector<std::wstring> getAdapters();
        static std::optional<std::wstring> resAdapter(const std::wstring& adapterName);
        static std::wstring getAdapterRegPath(const std::wstring& adapterGUID);
        static void spoofMac();
    };
}