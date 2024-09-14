import os
import sqlite3
import shutil
import platform
import win32crypt
import base64
import json
import logging

from Cryptodome.Cipher import AES

def titan():
    x = """
████████╗██╗████████╗ █████╗ ███╗   ██╗
╚══██╔══╝██║╚══██╔══╝██╔══██╗████╗  ██║
   ██║   ██║   ██║   ███████║██╔██╗ ██║
   ██║   ██║   ██║   ██╔══██║██║╚██╗██║
   ██║   ██║   ██║   ██║  ██║██║ ╚████║
   ╚═╝   ╚═╝   ╚═╝   ╚═╝  ╚═╝╚═╝  ╚═══╝                                      
    """
    logging.info(x)

logging.basicConfig(level=logging.INFO, format='%(levelname)s - %(message)s')

bpaths = {
    "chrome": os.path.expandvars(r"%LOCALAPPDATA%\Google\Chrome\User Data"),
    "msedge": os.path.expandvars(r"%LOCALAPPDATA%\Microsoft\Edge\User Data"),
    "brave": os.path.expandvars(r"%LOCALAPPDATA%\BraveSoftware\Brave-Browser\User Data"),
    "vivaldi": os.path.expandvars(r"%LOCALAPPDATA%\Vivaldi\User Data"),
    "opera": os.path.expandvars(r"%APPDATA%\Opera Software\Opera Stable"),
    "firefox": os.path.expandvars(r"%APPDATA%\Mozilla\Firefox\Profiles"),
    "yandex": os.path.expandvars(r"%LOCALAPPDATA%\Yandex\YandexBrowser\User Data"),
    "comodo": os.path.expandvars(r"%LOCALAPPDATA%\Comodo\Dragon\User Data"),
    "torch": os.path.expandvars(r"%LOCALAPPDATA%\Torch\User Data"),
    "epic": os.path.expandvars(r"%LOCALAPPDATA%\Epic Privacy Browser\User Data"),
    "cent": os.path.expandvars(r"%LOCALAPPDATA%\CentBrowser\User Data"),
    "iridium": os.path.expandvars(r"%LOCALAPPDATA%\Iridium\User Data"),
}

def masterkey(browser_path):
    try:
        with open(os.path.join(browser_path, "Local State"), "r", encoding="utf-8") as file:
            local_state = json.loads(file.read())
        encrypted_key = base64.b64decode(local_state["os_crypt"]["encrypted_key"])
        encrypted_key = encrypted_key[5:]
        return win32crypt.CryptUnprotectData(encrypted_key, None, None, None, 0)[1]
    except Exception as e:
        exit

def decrypt(encrypted_value, key):
    try:
        iv = encrypted_value[3:15]
        payload = encrypted_value[15:]
        cipher = AES.new(key, AES.MODE_GCM, iv)
        return cipher.decrypt(payload).decode('utf-8')
    except Exception as e:
        exit

def clear_db(db_path, browser_path, success_callback=None, error_callback=None):
    try:
        key = masterkey(browser_path)
        if not key:
            if error_callback:
                error_callback(f"Could not retrieve encryption key for {browser_path}")
            return
        
        with sqlite3.connect(db_path) as conn:
            cursor = conn.cursor()
            cursor.execute("SELECT host_key, name, encrypted_value FROM cookies WHERE host_key LIKE '%roblox.com%'")
            rows = cursor.fetchall()

            for host_key, name, encrypted_value in rows:
                decrypted_value = decrypt(encrypted_value, key)
                if decrypted_value:
                    cursor.execute("DELETE FROM cookies WHERE host_key=? AND name=?", (host_key, name))
            conn.commit()

        if success_callback:
            success_callback(f"Roblox cookies cleared from {db_path}")
    except sqlite3.Error as e:
        if error_callback:
            error_callback(f"Failed to clear cookies for {db_path}: {e}")
    except Exception as e:
        if error_callback:
            error_callback(f"? > {e}")

def db_path(browser):
    paths = bpaths.get(browser)
    if not paths:
        return None
    return os.path.join(paths, "Default", "Network", "Cookies")

def actv(browser, success_callback=None, error_callback=None):
    cookie_db_path = db_path(browser)
    if not cookie_db_path or not os.path.exists(cookie_db_path):
        if error_callback:
            error_callback(f"Cookie database not found for {browser}")
        return

    clear_db(cookie_db_path, bpaths[browser], success_callback, error_callback)

def main():
    if platform.system() != 'Windows':
        logging.error("This script is designed for Windows systems.")
        return

    titan()

    logging.info("Available browsers:")
    for browser in bpaths.keys():
        logging.info(f"- {browser.capitalize()}")

    browser = input("Enter the browser name to clear cookies (e.g., 'chrome', 'msedge'): ").strip().lower()
    if browser not in bpaths:
        logging.error("Unsupported browser.")
        return

    actv(
        browser, 
        success_callback=lambda message: logging.info(f"SUCCESS: {message}"),
        error_callback=lambda message: logging.error(f"ERROR: {message}")
    )
    logging.info(f"Finished clearing Roblox cookies for {browser}.")

if __name__ == "__main__":
    main()