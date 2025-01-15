#include "../Header/FsCleaner.h"

namespace fs = std::filesystem;

std::string FsCleaner::getEnv(const char* var) {
    char* buffer = nullptr;
    size_t size = 0;
    if (_dupenv_s(&buffer, &size, var) != 0 || !buffer) {
        throw std::runtime_error("failed to retrieve environment variable: " + std::string(var));
    }
    std::string result(buffer);
    free(buffer);
    return result;
}

template <typename Func>
void FsCleaner::retry(Func func, const fs::path& path) {
    for (int retries = 0; retries < 3; ++retries) {
        try {
            func();
            return;
        }
        catch (const fs::filesystem_error&) {
            std::cout << "operation failed on: " << path << ", retrying in 1s..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void FsCleaner::run() {
    Services::SectHeader("File System Cleaning", 202);

    bool logsCleaned = cleanPaths({
        { getEnv("TEMP"), { "Roblox", "RobloxPlayerBeta.pdb", "crashpad_roblox" } }
        });

    bool cfgReplaced = replaceCfg(getEnv("LOCALAPPDATA") + "/Roblox/GlobalBasicSettings_13.xml");

    bool versionsCleaned = cleanVersions({
        { "C:/Program Files/Roblox/Versions", false },
        { "C:/Program Files (x86)/Roblox/Versions", false },
        { getEnv("USERPROFILE") + "/AppData/Local/Bloxstrap/Versions", false },
        { getEnv("LOCALAPPDATA") + "/Fishstrap/Versions", false },
        { getEnv("LOCALAPPDATA") + "/Fishstrap/Roblox/Versions", false }
        });

    if (!logsCleaned && !cfgReplaced && !versionsCleaned) {
        std::cout << "nothing to clean :p" << std::endl;
    }
}

bool FsCleaner::cleanPaths(const std::vector<std::pair<fs::path, std::vector<std::string_view>>>& paths) {
    bool cleaned = false;
    for (const auto& [basePath, targets] : paths) {
        for (const auto& target : targets) {
            const fs::path path = basePath / target;
            retry([&]() {
                if (fs::exists(path)) {
                    fs::is_directory(path) ? fs::remove_all(path) : fs::remove(path);
                    std::cout << "deleted -> " << path << std::endl;
                    cleaned = true;
                }
                }, path);
        }
    }
    return cleaned;
}

bool FsCleaner::replaceCfg(const fs::path& cfgPath) {
    if (!fs::exists(cfgPath)) return false;

    bool cleaned = process(cfgPath);

    for (const auto& entry : fs::directory_iterator(cfgPath.parent_path())) {
        if (entry.path().filename() != cfgPath.filename()) {
            retry([&]() {
                fs::remove_all(entry.path());
                std::cout << "deleted -> " << entry.path() << std::endl;
                cleaned = true;
                }, entry.path());
        }
    }
    return cleaned;
}

bool FsCleaner::cleanVersions(const std::vector<std::pair<fs::path, bool>>& paths) {
    const std::vector<std::string> filesToDelete = {
        "RobloxPlayerLauncher.exe", "RobloxPlayerBeta.exe", "RobloxPlayerBeta.dll",
        "WebView2Loader.dll", "RobloxCrashHandler.exe"
    };

    bool cleaned = false;
    for (const auto& [baseDir, _] : paths) {
        if (!fs::exists(baseDir) || !fs::is_directory(baseDir)) continue;

        for (const auto& versionDir : fs::directory_iterator(baseDir)) {
            if (!fs::is_directory(versionDir)) continue;

            if (versionDir.path().filename().string().starts_with("version-")) {
                for (const auto& file : filesToDelete) {
                    retry([&]() {
                        fs::remove(versionDir.path() / file);
                        std::cout << "deleted -> " << versionDir.path() / file << std::endl;
                        cleaned = true;
                        }, versionDir.path() / file);
                }
            }
        }
    }
    return cleaned;
}

bool FsCleaner::process(const fs::path& filePath) {
    std::ifstream cfgFile(filePath, std::ios::binary);
    if (!cfgFile) return false;

    std::string content((std::istreambuf_iterator<char>(cfgFile)), std::istreambuf_iterator<char>());
    cfgFile.close();

    const std::string updatedContent = updCfg(content);
    fs::remove(filePath);

    std::ofstream patchedCfg(filePath, std::ios::binary);
    if (!patchedCfg) return false;

    patchedCfg << updatedContent;
    return true;
}

std::string FsCleaner::updCfg(const std::string& content) {
    static const std::regex referRegex(R"(<Item class="UserGameSettings" referent="[^"]+">)");

    auto b4bv = []() {
        const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);

        std::string result;
        result.reserve(24);
        for (int i = 0; i < 24; ++i) {
            result += chars[dist(gen)];
        }
        return result;
        };

    return std::regex_replace(content, referRegex,
        "<Item class=\"UserGameSettings\" referent=\"" + b4bv() + "\">");
}