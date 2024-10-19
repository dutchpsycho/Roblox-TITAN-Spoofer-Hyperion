#include "../Header/Roblox.hxx"
#include "../Util/Utils.hxx"

namespace fs = std::filesystem;

bool Term(const std::string& pname);
bool Wait(const std::string& pname);

std::string getSystemDrive() {
    char systemDrive[MAX_PATH];
    if (GetEnvironmentVariableA("SystemDrive", systemDrive, MAX_PATH) > 0) {
        return std::string(systemDrive);
    }
    return "C:";
}

std::string NewSets(const std::string& xmlContent) {
    std::string nXML = xmlContent;
    std::regex referRegex(R"(<Item class="UserGameSettings" referent="[^"]+">)");
    nXML = std::regex_replace(nXML, referRegex, R"(<Item class="UserGameSettings" referent="blew my high">)");
    return nXML;
}

bool CopySets(const fs::path& SetFPath) {
    if (!fs::exists(SetFPath)) {
        return false;
    }

    std::ifstream configFile(SetFPath);
    if (!configFile.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << configFile.rdbuf();
    std::string xmlContent = buffer.str();
    configFile.close();

    std::string nXML = NewSets(xmlContent);

    fs::remove(SetFPath);
    std::ofstream NewSetF(SetFPath);
    if (!NewSetF.is_open()) {
        return false;
    }

    NewSetF << nXML;
    NewSetF.close();

    return true;
}

bool Term(const std::string& pname) {
    std::string command = "conhost.exe /C taskkill /IM " + pname + " /F";
    return system(command.c_str()) == 0;
}

bool Wait(const std::string& pname) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    bool prun = true;

    while (prun) {
        hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hProcessSnap == INVALID_HANDLE_VALUE) {
            std::cerr << "Failed to create process snapshot." << std::endl;
            return false;
        }

        pe32.dwSize = sizeof(PROCESSENTRY32);
        prun = false;

        if (Process32First(hProcessSnap, &pe32)) {
            do {
                char exeFileA[MAX_PATH];
                WideCharToMultiByte(CP_ACP, 0, pe32.szExeFile, -1, exeFileA, MAX_PATH, NULL, NULL);

                if (_stricmp(exeFileA, pname.c_str()) == 0) {
                    prun = true;
                    break;
                }
            } while (Process32Next(hProcessSnap, &pe32));
        }

        CloseHandle(hProcessSnap);

        if (prun) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    return true;
}

void ClearRoblox() {
    std::cout << "\033[38;2;0;0;255m" << "\n========== ROBLOX CLEARING PROCESS ==========\n" << "\033[0m";

    Term("RobloxPlayerBeta.exe");
    Term("RobloxCrashHandler.exe");

    Wait("RobloxPlayerBeta.exe");
    Wait("RobloxCrashHandler.exe");

    char LocalAppDataP[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, LocalAppDataP))) {
        fs::path rbxP = fs::path(LocalAppDataP) / "Roblox";
        fs::path setFPath = rbxP / "GlobalBasicSettings_13.xml";

        std::string originalXMLContent;
        std::string mXML;

        if (fs::exists(setFPath)) {
            std::ifstream configFile(setFPath);
            if (configFile.is_open()) {
                std::stringstream buffer;
                buffer << configFile.rdbuf();
                originalXMLContent = buffer.str();
                configFile.close();

                mXML = NewSets(originalXMLContent);
            } else {
                std::cerr << "Failed to open Roblox's XML file: " << setFPath << std::endl;
            }
        } else {
            std::cerr << "Global XML not found, skipping rollover." << std::endl;
        }

        if (fs::exists(rbxP)) {
            std::cout << "Removing Roblox dir: " << rbxP << std::endl;
            rmDir(rbxP);

            if (!fs::exists(rbxP)) {
                fs::create_directories(rbxP);
            }

            if (!mXML.empty()) {
                std::ofstream newSetF(setFPath);
                if (newSetF.is_open()) {
                    newSetF << mXML;
                    newSetF.close();
                    std::cout << "Cleaned cfg: " << setFPath << std::endl;
                }
            }
        } else {
            std::cerr << "Roblox dir not found, skipping." << std::endl;
        }
    }
}