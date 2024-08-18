# Requirements and Instructions ⚡

1. **SQL3.dll:**
   - Required for `TITAN_Spoofer.exe` (Library for SQLite3).
   - If you move `TITAN_Spoofer.exe`, ensure you move `SQL3.dll` with it or create a shortcut.

2. **HEADLESS_TITAN_Spoofer.exe:**
   - Does not require `SQL3.dll`.
   - Does not clear the Roblox cookie cache.
   - Can be used in startup.

3. **TITAN_Spoofer.exe:**
   - Includes a Command Line Interface (CLI) navigated by arrow keys.
   - Provides an option to clear your Roblox cookie cache.
   - Use this executable if you need CLI functionality.

4. **HEADLESS_TITAN_Spoofer.exe:**
   - Does not include the CLI.
   - Acts the same as the "Spoof" command in `TITAN_Spoofer.exe`.
   - Controlled by the `#define HEADLESS` directive in `Master.cpp`.

5. **Cache Cleaner:**
   - If the cache cleaner in `TITAN_Spoofer.exe` is not working, use `Python/CookieCacheCleaner.py` to clear the Roblox cookie cache.

6. **Spoofing on Startup:**
   - Press `Windows + R`, type `shell:startup`, and press Enter.
   - Create a shortcut to `HEADLESS_TITAN_Spoofer.exe` in the startup folder, or drag the `.exe` file there.
   - Note: This method does not work for `TITAN_Spoofer.exe` (CLI version).
