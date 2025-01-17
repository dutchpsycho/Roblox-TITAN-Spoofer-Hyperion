#ifndef FS_CLEANER_H
#define FS_CLEANER_H

#include <Windows.h>

#include <filesystem>
#include <regex>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>

class FsCleaner {
public:
    static void run();
    static void Install();

private:
    static void RmvReferents(const std::filesystem::path& filePath, const std::wstring& itemClass);
    static void CleanRbx();
};

#endif