#include "../Header/FsCleaner.h"

namespace fs = std::filesystem;

void FsCleaner::run() {
    Services::SectHeader("File System Cleaning", 202);

    bool logsCleaned = delLogs();
    bool cfgReplaced = replaceCfg();
    bool versCleaned = cleanVers();

    if (!logsCleaned && !cfgReplaced && !versCleaned) {
        std::cout << "Nothing to clean :P" << std::endl;
    }
}

std::string FsCleaner::getEnv(const char* var) {
    char* buffer = nullptr;
    size_t size = 0;
    if (_dupenv_s(&buffer, &size, var) != 0 || !buffer) {
        throw std::runtime_error("Failed to retrieve environment variable: " + std::string(var));
    }
    std::string result(buffer);
    free(buffer);
    return result;
}

template <typename Func>
void FsCleaner::retry(Func func, const fs::path& path) {
    int retries = 0;
    while (retries < 3) {
        try {
            func();
            return;
        }
        catch (const fs::filesystem_error&) {
            retries++;
            if (retries >= 5) return; // roblox should die within 5s, if not sums wrong

            std::cout << "Roblox seems to be open, Retrying in 1s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

bool FsCleaner::delLogs() {
    const fs::path tempPath = getEnv("TEMP");
    const std::string_view targets[] = { "Roblox", "RobloxPlayerBeta.pdb", "crashpad_roblox" };
    bool cleaned = false;

    for (const auto& target : targets) {
        const fs::path path = tempPath / target;
        retry([&]() {
            if (fs::exists(path)) {
                fs::is_directory(path) ? fs::remove_all(path) : fs::remove(path);
                std::cout << "Deleted -> " << path << std::endl;
                cleaned = true;
            }
            }, path);
    }
    return cleaned;
}

bool FsCleaner::replaceCfg() {
    const fs::path setsPath = fs::path(getEnv("LOCALAPPDATA")) / "Roblox" / "GlobalBasicSettings_13.xml";
    if (!fs::exists(setsPath)) return false;

    bool cleaned = false;

    if (process(setsPath)) {
        for (const auto& entry : fs::directory_iterator(setsPath.parent_path())) {
            retry([&]() {
                if (entry.path().filename() != "GlobalBasicSettings_13.xml") {
                    fs::remove_all(entry.path());
                    std::cout << "Deleted -> " << entry.path() << std::endl;
                    cleaned = true;
                }
                }, entry.path());
        }
    }
    return cleaned;
}

bool FsCleaner::cleanVers() {
    const std::string_view toDel[] = {
        "RobloxPlayerLauncher.exe", "RobloxPlayerBeta.exe", "RobloxPlayerBeta.dll",
        "WebView2Loader.dll", "RobloxCrashHandler.exe"
    };
    bool cleaned = false;

    auto process = [&](const fs::path& baseDir) {
        if (!fs::exists(baseDir) || !fs::is_directory(baseDir)) return;

        for (const auto& verDir : fs::directory_iterator(baseDir)) {
            if (!fs::is_directory(verDir)) continue;

            const auto& dir = verDir.path().filename().string();
            if (!dir.starts_with("version-")) continue;

            for (const auto& file : toDel) {
                const fs::path filePath = verDir.path() / file;
                retry([&]() {
                    if (fs::exists(filePath)) {
                        fs::remove(filePath);
                        std::cout << "Deleted -> " << filePath << std::endl;
                        cleaned = true;
                    }
                    }, filePath);
            }
        }
        };

    for (const auto& drive : fs::directory_iterator("/")) {
        if (!drive.is_directory()) continue;

        const fs::path path = drive.path() / "Program Files" / "Roblox" / "Versions";
        const fs::path pathx86 = drive.path() / "Program Files (x86)" / "Roblox" / "Versions";

        process(path);
        process(pathx86);
    }
    return cleaned;
}

bool FsCleaner::process(const fs::path& filePath) {
    std::ifstream cfgFile(filePath, std::ios::binary);
    if (!cfgFile) return false;

    std::string content((std::istreambuf_iterator<char>(cfgFile)), std::istreambuf_iterator<char>());
    cfgFile.close();

    const std::string updCont = updCfg(content);
    fs::remove(filePath);

    std::ofstream patchedCfg(filePath, std::ios::binary);
    if (!patchedCfg) return false;

    patchedCfg << updCont;
    return true;
}

std::string FsCleaner::updCfg(const std::string& content) {
    static const std::regex referRegex(R"(<Item class="UserGameSettings" referent="[^"]+">)");
    return std::regex_replace(content, referRegex, R"(<Item class="UserGameSettings" referent="TITAN">)");
}