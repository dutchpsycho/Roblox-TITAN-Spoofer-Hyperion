#pragma once

#include "Container/Services/Services.hpp"
#include "Container/Header/FsCleaner.h"
#include "Container/Header/Mac.h"
#include "Container/Header/Registry.h"
#include "Container/Header/WMI.h"


/**
 * runs the `runTasks` function in a separate thread
 * this ensures that logs for the main application are not suppressed if logs r set to false
 *
 * @param logs - boolean indicating whether logs should be enabled (true) or suppressed (false)
 * @return a `std::thread` object that runs `runTasks` in the background
 */

namespace TitanSpoofer {

    inline void noOut() {
        std::cout.setstate(std::ios_base::failbit);
    }

    inline void reOut() {
        std::cout.clear();
    }

    inline bool runTasks(bool logs) {
        try {
            if (!logs) {
                noOut();
            }

            Services::KillRbx(); // this will obviously... kill roblox
            FsCleaner::run();   // it ties in with the file cleaner, if u dont want roblox killed then dont run this

            // these do not require roblox to be closed, but would b good
            MAC::MacSpoofer::run();
            Registry::RegSpoofer::run();
            WMI::WmiSpoofer::run();
            FsCleaner::Install();

            if (!logs) {
                reOut();
            }

            return true; // success
        }
        catch (const std::exception& ex) {
            if (!logs) {
                reOut();
            }
            std::cerr << "Titan Spoofer error -> " << ex.what() << std::endl;
            return false; // failure
        }
    }

    inline std::thread run(bool logs) {
        return std::thread([logs]() {
            runTasks(logs);
            });
    }
}