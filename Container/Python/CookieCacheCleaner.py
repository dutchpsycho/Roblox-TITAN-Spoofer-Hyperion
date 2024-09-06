import os
import sqlite3
import shutil
import platform
import win32crypt
import base64
from Cryptodome.Cipher import AES
import json

browser_cookie_paths = {
    "chrome": os.path.expandvars(r"%LOCALAPPDATA%\Google\Chrome\User Data"),
    "msedge": os.path.expandvars(r"%LOCALAPPDATA%\Microsoft\Edge\User Data"),
    "brave": os.path.expandvars(r"%LOCALAPPDATA%\BraveSoftware\Brave-Browser\User Data"),
    "vivaldi": os.path.expandvars(r"%LOCALAPPDATA%\Vivaldi\User Data"),
}

def mskey(browser_path):
    with open(os.path.join(browser_path, "Local State"), "r", encoding="utf-8") as file:
        local_state = json.loads(file.read())
    encrypted_key = base64.b64decode(local_state["os_crypt"]["encrypted_key"])
    encrypted_key = encrypted_key[5:]
    return win32crypt.CryptUnprotectData(encrypted_key, None, None, None, 0)[1]

def decrypt_cookie(encrypted_value, key):
    try:
        iv = encrypted_value[3:15]
        payload = encrypted_value[15:]
        cipher = AES.new(key, AES.MODE_GCM, iv)
        decrypted_value = cipher.decrypt(payload).decode('utf-8')
        return decrypted_value
    except Exception as e:
        return None

def clear_cookies_from_db(db_path, browser_path, success_callback=None, error_callback=None):
    try:
        key = mskey(browser_path)
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        cursor.execute("SELECT host_key, name, encrypted_value FROM cookies")
        rows = cursor.fetchall()
        for host_key, name, encrypted_value in rows:
            if "roblox.com" in host_key:
                decrypted_value = decrypt_cookie(encrypted_value, key)
                if decrypted_value:
                    cursor.execute("DELETE FROM cookies WHERE host_key=? AND name=?", (host_key, name))
        conn.commit()
        cursor.close()
        conn.close()
        if success_callback:
            success_callback(f"Roblox cookies cleared from {db_path}")
    except sqlite3.Error as e:
        if error_callback:
            error_callback(f"Failed to clear cookies from {db_path}: {e}")

def clear_browser_cookies(browser, success_callback=None, error_callback=None):
    paths = browser_cookie_paths.get(browser.lower())
    if not paths:
        if error_callback:
            error_callback(f"Browser {browser} is not supported.")
        return

    for path in paths:
        cookie_db_path = os.path.join(path, "Default", "Network", "Cookies")
        if os.path.exists(cookie_db_path):
            clear_cookies_from_db(cookie_db_path, path, success_callback, error_callback)
        else:
            if error_callback:
                error_callback(f"Cookie database not found at {cookie_db_path}")

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

    clear_browser_cookies(
        browser, 
        success_callback=lambda message: print(f"SUCCESS: {message}"),
        error_callback=lambda message: print(f"ERROR: {message}")
    )
    print(f"Finished clearing Roblox cookies for {browser}.")

if __name__ == "__main__":
    main()