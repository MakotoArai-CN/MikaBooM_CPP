#pragma once
#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef LONG (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);

class SystemInfo {
public:
    static bool IsWindows7OrLater() {
        DWORD major = 0, minor = 0;
        GetRealWindowsVersion(major, minor);
        if (major > 6) {
            return true;
        }
        if (major == 6 && minor >= 1) {
            return true;
        }
        return false;
    }
    
    static const char* GetOSName() {
        static char osName[256] = {0};
        if (osName[0] != '\0') return osName;
        
        DWORD major = 0, minor = 0, build = 0;
        GetRealWindowsVersion(major, minor, &build);
        
        if (major == 10) {
            if (build >= 22000) {
                strcpy(osName, "Windows 11");
            } else {
                strcpy(osName, "Windows 10");
            }
        } else if (major == 6) {
            if (minor == 3) {
                strcpy(osName, "Windows 8.1");
            } else if (minor == 2) {
                strcpy(osName, "Windows 8");
            } else if (minor == 1) {
                strcpy(osName, "Windows 7");
            } else if (minor == 0) {
                strcpy(osName, "Windows Vista");
            }
        } else if (major == 5) {
            if (minor == 2) {
                strcpy(osName, "Windows Server 2003");
            } else if (minor == 1) {
                strcpy(osName, "Windows XP");
            } else if (minor == 0) {
                strcpy(osName, "Windows 2000");
            }
        } else {
            sprintf(osName, "Windows (Version %lu.%lu)", major, minor);
        }
        
        return osName;
    }
    
    // 公开这个函数供其他模块使用
    static void GetRealWindowsVersion(DWORD& major, DWORD& minor, DWORD* build = NULL) {
        major = 0;
        minor = 0;
        if (build) *build = 0;
        
        HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
        if (hNtdll) {
            RtlGetVersionPtr pRtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
            if (pRtlGetVersion) {
                OSVERSIONINFOEXW osvi;
                ZeroMemory(&osvi, sizeof(osvi));
                osvi.dwOSVersionInfoSize = sizeof(osvi);
                if (pRtlGetVersion((PRTL_OSVERSIONINFOEXW)&osvi) == 0) {
                    major = osvi.dwMajorVersion;
                    minor = osvi.dwMinorVersion;
                    if (build) *build = osvi.dwBuildNumber;
                    return;
                }
            }
        }
        
        OSVERSIONINFOEXA osvi;
        ZeroMemory(&osvi, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        if (GetVersionExA((LPOSVERSIONINFOA)&osvi)) {
            major = osvi.dwMajorVersion;
            minor = osvi.dwMinorVersion;
            if (build) *build = osvi.dwBuildNumber;
            return;
        }
        
        OSVERSIONINFOA basicOsvi;
        ZeroMemory(&basicOsvi, sizeof(basicOsvi));
        basicOsvi.dwOSVersionInfoSize = sizeof(basicOsvi);
        if (GetVersionExA(&basicOsvi)) {
            major = basicOsvi.dwMajorVersion;
            minor = basicOsvi.dwMinorVersion;
            if (build) *build = basicOsvi.dwBuildNumber;
        }
    }
};