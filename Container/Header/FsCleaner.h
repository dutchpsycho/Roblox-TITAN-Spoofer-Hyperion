#pragma once

#include "../Services/Services.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <regex>
#include <sstream>
#include <thread>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <chrono>

namespace fs = std::filesystem;

class FsCleaner {
public:
    static void run();

private:
    static bool cleanPaths(const std::vector<std::pair<fs::path, std::vector<std::string_view>>>& paths);
    static bool replaceCfg(const fs::path& cfgPath);
    static bool cleanVersions(const std::vector<std::pair<fs::path, bool>>& paths);
    static bool process(const fs::path& filePath);

    static std::string updCfg(const std::string& content);
    static std::string getEnv(const char* var);

    template <typename Func>

    static void retry(Func func, const fs::path& path);
};