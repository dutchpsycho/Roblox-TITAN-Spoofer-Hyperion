#include "../Header/FsCleaner.h"
#include "../Services/Services.hpp"

namespace fs = std::filesystem;

struct PathHelper {
    static std::wstring user() { return Services::GetUser(); }
    static std::wstring sys() { return Services::GetSysDrive(); }

    static std::optional<fs::path> resolveShortcut(const std::wstring& name) {
        std::vector<fs::path> shortcuts = {
            fs::path(sys()) / L"ProgramData" / L"Microsoft" / L"Windows" / L"Start Menu" / L"Programs" / name,
            fs::path(user()) / L"AppData" / L"Roaming" / L"Microsoft" / L"Windows" / L"Start Menu" / L"Programs" / name
        };

        for (const auto& p : shortcuts) {
            if (fs::exists(p)) {
                auto target = Services::ResolveTarget(p);
                if (!target.empty() && fs::exists(target))
                    return fs::path(target);
            }
        }

        return std::nullopt;
    }

    static std::optional<fs::path> findExe(const std::wstring& exeName) {
        std::vector<std::wstring> roots = {
            sys() + L"\\Program Files\\",
            sys() + L"\\Program Files (x86)\\",
            user() + L"\\AppData\\Local\\"
        };

        for (const auto& root : roots) {
            std::error_code ec;
            for (auto it = fs::recursive_directory_iterator(root,
                fs::directory_options::skip_permission_denied,
                ec);
                it != fs::recursive_directory_iterator();
                it.increment(ec)) {
                if (!ec && it->path().filename() == exeName)
                    return it->path();
            }
        }
        return std::nullopt;
    }

    static fs::path fallbackBloxstrap() { return fs::path(user()) / L"AppData" / L"Local" / L"Bloxstrap" / L"Bloxstrap.exe"; }
    static fs::path fallbackFishstrap() { return fs::path(user()) / L"AppData" / L"Local" / L"Fishstrap" / L"Fishstrap.exe"; }
    static fs::path fallbackRoblox() { return fs::path(sys()) / L"Program Files (x86)" / L"Roblox" / L"Versions" / L"RobloxPlayerInstaller.exe"; }
};

static void cleanVers(const fs::path& baseDir) {
    if (!fs::exists(baseDir)) return;
    for (const auto& d : fs::directory_iterator(baseDir)) {
        auto name = d.path().filename().wstring();
        if (d.is_directory() && name.rfind(L"version-", 0) == 0) {
            Services::BulkDelete(d.path(), {
                L"RobloxPlayerBeta.exe",
                L"RobloxPlayerBeta.dll",
                L"RobloxCrashHandler.exe",
                L"RobloxPlayerLauncher.exe"
                });
        }
    }
}

void FsCleaner::RmvReferents(const fs::path& filePath, const std::wstring& itemClass) {
    if (!fs::exists(filePath))
        throw std::runtime_error("File does not exist: " + filePath.string());

    std::wifstream in(filePath);
    if (!in.is_open())
        throw std::runtime_error("Failed to open file: " + filePath.string());

    std::wstringstream buf;
    buf << in.rdbuf();
    in.close();

    std::wstring content = buf.str();
    std::wregex rx(LR"(<Item class=\")" + itemClass + LR"(" referent=\"[^\"]+\">)");
    std::wstring rep = L"<Item class=\"" + itemClass + L"\" referent=\"" + Services::genRand() + L"\">";

    content = std::regex_replace(content, rx, rep);

    std::wofstream out(filePath);

    if (!out.is_open())
        throw std::runtime_error("Failed to write to file -> " + filePath.string());
    out << content;
}

void FsCleaner::CleanRbx() {

    // roblox
    if (auto robLnk = PathHelper::resolveShortcut(L"Roblox Player.lnk")) {
        cleanVers(robLnk->parent_path().parent_path());
    }
    cleanVers(fs::path(PathHelper::sys()) / L"Program Files (x86)/Roblox/Versions");

    // bloxstrap
    if (auto bxLnk = PathHelper::resolveShortcut(L"Bloxstrap.lnk")) {
        cleanVers(bxLnk->parent_path() / L"Versions");
    }
    cleanVers(fs::path(PathHelper::user()) / L"AppData/Local/Bloxstrap/Versions");

    // fishstrap
    if (auto fsLnk = PathHelper::resolveShortcut(L"Fishstrap.lnk")) {
        cleanVers(fsLnk->parent_path() / L"Versions");
    }
    cleanVers(fs::path(PathHelper::user()) / L"AppData/Local/Fishstrap/Versions");

    // temp & logs
    fs::remove_all(fs::path(PathHelper::user()) / L"AppData/Local/Temp/Roblox");
    fs::path logs = fs::path(PathHelper::user()) / L"AppData/Local/Roblox";

    for (const std::wstring& sub : { L"logs", L"LocalStorage", L"Downloads" }) {
        fs::remove_all(logs / sub);
        std::wcout << L"Deleted -> " << (logs / sub) << std::endl;
    }

    RmvReferents(logs / L"AnalysticsSettings.xml", L"GoogleAnalyticsConfiguration");
    RmvReferents(logs / L"GlobalBasicSettings_13.xml", L"UserGameSettings");
}

void FsCleaner::Install() {
    Services::SectHeader("Roblox Installation", 203);

    std::atomic<bool> stopMon(false);
    auto monitor = [&]() {
        while (!stopMon) {
            if (HWND h = FindWindowW(nullptr, L"Roblox")) {
                DWORD pid = 0;
                GetWindowThreadProcessId(h, &pid);
                if (pid) {
                    if (HANDLE p = OpenProcess(PROCESS_TERMINATE, FALSE, pid)) {
                        TerminateProcess(p, 0);
                        CloseHandle(p);
                        stopMon = true;
                        return;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        };

    std::wstring exe;
    // search order
    if (auto p = PathHelper::findExe(L"Bloxstrap.exe"))            exe = p->wstring();

    else if (auto p = PathHelper::findExe(L"Fishstrap.exe"))       exe = p->wstring();
    else if (auto p = PathHelper::resolveShortcut(L"Bloxstrap.lnk")) exe = p->wstring();
    else if (auto p = PathHelper::resolveShortcut(L"Fishstrap.lnk")) exe = p->wstring();
    else if (fs::exists(PathHelper::fallbackBloxstrap()))          exe = PathHelper::fallbackBloxstrap().wstring();
    else if (fs::exists(PathHelper::fallbackFishstrap()))          exe = PathHelper::fallbackFishstrap().wstring();

    if (!exe.empty()) {
        ShellExecuteW(nullptr, L"open", exe.c_str(), L"-player", nullptr, SW_HIDE);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        std::thread(monitor).join();
    }

    else {
        std::wstring rbx;

        if (auto p = PathHelper::findExe(L"RobloxPlayerInstaller.exe")) rbx = p->wstring();
        else if (fs::exists(PathHelper::fallbackRoblox()))              rbx = PathHelper::fallbackRoblox().wstring();

        if (!rbx.empty()) {
            ShellExecuteW(nullptr, L"open", rbx.c_str(), nullptr, nullptr, SW_HIDE);
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }

        else {
            std::wcerr << L"[!] no installer found\n";
        }
    }

    stopMon = true;
}

void FsCleaner::run() {
    Services::SectHeader("File System Cleaning", 202);

    try {
        CleanRbx();
    }

    catch (const std::exception& ex) {
        std::cerr << "Error in FsCleaner::run(): " << ex.what() << '\n';
    }
}