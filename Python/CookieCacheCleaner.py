import os
import sys
import sqlite3
import shutil
import platform

browser_cookie_paths = {
    "chrome": os.path.expandvars(r"%LOCALAPPDATA%\Google\Chrome\User Data"),
    "msedge": os.path.expandvars(r"%LOCALAPPDATA%\Microsoft\Edge\User Data"),
    "firefox": os.path.expandvars(r"%APPDATA%\Mozilla\Firefox\Profiles"),
    "opera": os.path.expandvars(r"%APPDATA%\Opera Software\Opera Stable"),
    "brave": os.path.expandvars(r"%LOCALAPPDATA%\BraveSoftware\Brave-Browser\User Data"),
    "vivaldi": os.path.expandvars(r"%LOCALAPPDATA%\Vivaldi\User Data"),
    "waterfox": os.path.expandvars(r"%APPDATA%\Waterfox\Profiles"),
}

def clear_cookies_from_db(db_path):
    """Clear cookies from the database at the given path."""
    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        cursor.execute("DELETE FROM cookies WHERE host_key LIKE '%roblox.com%'")
        conn.commit()
        cursor.close()
        conn.close()
        print(f"Cookies cleared from {db_path}")
    except sqlite3.Error as e:
        print(f"Failed to clear cookies from {db_path}: {e}")

def delete_cookie_files(path):
    """Recursively delete cookie files from a given directory."""
    for root, dirs, files in os.walk(path):
        for file in files:
            if "cookie" in file.lower() or "cookies.sqlite" in file.lower():
                file_path = os.path.join(root, file)
                try:
                    os.remove(file_path)
                    print(f"Deleted {file_path}")
                except Exception as e:
                    print(f"Failed to delete {file_path}: {e}")

def clear_browser_cookies(browser):
    """Clear cookies for a given browser."""
    paths = browser_cookie_paths.get(browser.lower())
    if not paths:
        print(f"Browser {browser} is not supported.")
        return

    for path in paths:
        if os.path.exists(path):
            if browser.lower() in ["firefox", "waterfox"]:
                for profile in os.listdir(path):
                    profile_path = os.path.join(path, profile)
                    if os.path.isdir(profile_path):
                        clear_cookies_from_db(os.path.join(profile_path, "cookies.sqlite"))
            else:
                for dirpath, dirnames, filenames in os.walk(path):
                    for filename in filenames:
                        if "cookies" in filename.lower():
                            full_path = os.path.join(dirpath, filename)
                            clear_cookies_from_db(full_path)
        else:
            print(f"Path {path} does not exist.")

def main():
    if platform.system() != 'Windows':
        print("This script is designed for Windows systems.")
        return
    
    print("Available browsers:")
    for browser in browser_cookie_paths.keys():
        print(f"- {browser.capitalize()}")

    browser = input("Enter the browser name to clear cookies (e.g., 'chrome', 'firefox'): ").strip().lower()
    if browser not in browser_cookie_paths:
        print("Unsupported browser.")
        return

    clear_browser_cookies(browser)
    print(f"Finished clearing cookies for {browser}.")

if __name__ == "__main__":
    main()