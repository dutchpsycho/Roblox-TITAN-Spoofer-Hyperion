// TITAN Spoofer V1.2
// Licensed under CC BY-NC-ND 4.0
// Developed by TITAN
// TITAN Softworks EST. 2024

#include "Container/Services/Services.hpp"
#include "Container/Header/FsCleaner.h"
#include "Container/Header/Mac.h"
#include "Container/Header/Registry.h"
#include "Container/Header/WMI.h"
#include "Container/Header/Installer.h"

int main() {
    Services::TITAN();

    Services::KillRbx();

    FsCleaner::run();
    MAC::MacSpoofer::run();
    Registry::RegSpoofer::run();
    WMI::WmiSpoofer::run();

    Installer::run();

    std::cout << "\nAll done :3 Bye Bye Hyperion :mog: \n";

    // if you want to run this on startup, or dont want to have to close the window, comment below out
    std::cin.get();

    return 0;
}
