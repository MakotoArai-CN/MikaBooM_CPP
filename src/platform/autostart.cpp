#include "autostart.h"
#include <windows.h>

const char* AutoStart::GetRegistryKey() {
    return "Software\\Microsoft\\Windows\\CurrentVersion\\Run";
}

std::string AutoStart::GetExePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::string(buffer);
}

bool AutoStart::Enable() {
    HKEY hKey;
    std::string exePath = GetExePath() + " -window=false";

    if (RegOpenKeyExA(HKEY_CURRENT_USER, GetRegistryKey(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        LONG result = RegSetValueExA(hKey, "MikaBooM", 0, REG_SZ,
                                      (const BYTE*)exePath.c_str(),
                                      (DWORD)exePath.length() + 1);
        RegCloseKey(hKey);
        return result == ERROR_SUCCESS;
    }

    return false;
}

bool AutoStart::Disable() {
    HKEY hKey;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, GetRegistryKey(), 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        LONG result = RegDeleteValueA(hKey, "MikaBooM");
        RegCloseKey(hKey);
        return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
    }

    return false;
}

bool AutoStart::IsEnabled() {
    HKEY hKey;

    if (RegOpenKeyExA(HKEY_CURRENT_USER, GetRegistryKey(), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        DWORD type;
        DWORD size = 0;
        LONG result = RegQueryValueExA(hKey, "MikaBooM", NULL, &type, NULL, &size);
        RegCloseKey(hKey);
        return result == ERROR_SUCCESS;
    }

    return false;
}