#pragma once
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "compat_string.h"

typedef LONG (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
typedef BOOL (WINAPI* GetVersionExAPtr)(LPOSVERSIONINFOA);

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
                strcpy_s(osName, sizeof(osName), "Windows 11");
            } else {
                strcpy_s(osName, sizeof(osName), "Windows 10");
            }
        } else if (major == 6) {
            if (minor == 3) {
                strcpy_s(osName, sizeof(osName), "Windows 8.1");
            } else if (minor == 2) {
                strcpy_s(osName, sizeof(osName), "Windows 8");
            } else if (minor == 1) {
                strcpy_s(osName, sizeof(osName), "Windows 7");
            } else if (minor == 0) {
                strcpy_s(osName, sizeof(osName), "Windows Vista");
            }
        } else if (major == 5) {
            if (minor == 2) {
                strcpy_s(osName, sizeof(osName), "Windows Server 2003");
            } else if (minor == 1) {
                strcpy_s(osName, sizeof(osName), "Windows XP");
            } else if (minor == 0) {
                strcpy_s(osName, sizeof(osName), "Windows 2000");
            }
        } else {
            sprintf_s(osName, sizeof(osName), "Windows (Version %lu.%lu)", major, minor);
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
        
        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        GetVersionExAPtr pGetVersionExA = hKernel32
            ? (GetVersionExAPtr)GetProcAddress(hKernel32, "GetVersionExA")
            : NULL;
        if (!pGetVersionExA) {
            return;
        }

        OSVERSIONINFOEXA osvi;
        ZeroMemory(&osvi, sizeof(osvi));
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        if (pGetVersionExA((LPOSVERSIONINFOA)&osvi)) {
            major = osvi.dwMajorVersion;
            minor = osvi.dwMinorVersion;
            if (build) *build = osvi.dwBuildNumber;
            return;
        }

        OSVERSIONINFOA basicOsvi;
        ZeroMemory(&basicOsvi, sizeof(basicOsvi));
        basicOsvi.dwOSVersionInfoSize = sizeof(basicOsvi);
        if (pGetVersionExA(&basicOsvi)) {
            major = basicOsvi.dwMajorVersion;
            minor = basicOsvi.dwMinorVersion;
            if (build) *build = basicOsvi.dwBuildNumber;
        }
    }
};