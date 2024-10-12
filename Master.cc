/*
* @Author Damon (Swedish.Psych0)
* @License TITAN © 2024 by Damon under CC BY-NC-ND 4.0
* @Namespace TITAN
* @Version 6.1.0
*/

/*
* If you want to use headless mode, you need to do a couple things.
* First, right click the project (right side, TITAN Spoofer)
* Second, click Linker dropdown & click advanced. change entry point to "MainCRTStartup"
* Third, click system and click the dropdown and switch it to "SUBSYSTEM::WINDOWS"
* Finally, uncomment "//#define HEADLESS"
*/

// #define HEADLESS

#include "./Container/Header/Hyperion.hxx"
#include "./Container/Header/Fingerprint.hxx"
#include "./Container/Header/Roblox.hxx"
#include "./Container/Header/MAC.hxx"
#include "./Container/Util/Elevate.hxx"
#include "./Container/Util/Utils.hxx"

#ifdef HEADLESS

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Elevate();
    SigFucker();
    ClearRoblox();
    spoofMac();
    spoofHyperion();
    return 0;
}

#else

int main() {
    try {
        std::string windowTitle = randstring(24);
        SetConsoleTitleA(windowTitle.c_str());
        SigFucker();
        TITAN();
        Elevate();
        int choice = 0;
        while (true) {
            system("CLS");
            TITAN();
            Menu(choice);
            int key = _getch();
            if (key == 13) {
                ClearRoblox();
                spoofMac();
                spoofHyperion();
                std::cout << std::endl;
                Colour(10); std::cout << "Op finished, sry Byfron not today ;3" << std::endl;std::cout << std::endl;Colour(7);
                system("pause");
                break;
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        system("pause");
    }
    system("pause");
    return 0;
}

#endif

// WITH FINGERPRINT SPOOFING, THIS FUCKS WITH UR BIOS AND SYSTEM. DO NOT USE IF YOU USE WAVE/SYNZ
/*
int main() {
    try {
        srand(static_cast<unsigned int>(time(nullptr)));
        std::string windowTitle = rands(24);
        SetConsoleTitleA(windowTitle.c_str());
        SigFucker();
        TITAN();
        Elevate();
        int choice = 0;
        while (true) {
            system("CLS");
            TITAN();
            Menu(choice);
            int key = _getch();
            if (key == 13) {
                std::unique_ptr<SystemFingerprint> fingerprint;
                ClearRoblox();
                spoofMac();
                spoofHyperion();
                fingerprint = SystemFingerprint::CreateUniqueFingerprint();
                std::cout << "Spoofed Fingerprint Successfully :: [ " << fingerprint->ToString() << " ]\n" << std::endl;
                _ps("Operation Finished Successfully, no restart required.", 10);
                system("pause");
                break;
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        system("pause");
    }
    system("pause");
    return 0;
}
#endif
*/