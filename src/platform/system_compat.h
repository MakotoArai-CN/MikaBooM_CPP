#pragma once

#include <stdint.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#elif defined(__linux__)
#include <sys/sysinfo.h>
#endif

struct MemoryStatusSnapshot {
    unsigned long memoryLoad;
    uint64_t totalPhys;
    uint64_t availPhys;

    MemoryStatusSnapshot()
        : memoryLoad(0), totalPhys(0), availPhys(0) {}
};

class SystemCompat {
public:
    static bool QueryMemoryStatus(MemoryStatusSnapshot& status) {
#ifdef _WIN32
        return QueryMemoryStatusWindows(status);
#elif defined(__linux__)
        return QueryMemoryStatusLinux(status);
#else
        (void)status;
        return false;
#endif
    }

private:
#ifdef _WIN32
    static bool QueryMemoryStatusWindows(MemoryStatusSnapshot& status) {
        typedef BOOL (WINAPI *PGlobalMemoryStatusEx)(LPMEMORYSTATUSEX);

        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (hKernel32) {
            PGlobalMemoryStatusEx pGlobalMemoryStatusEx =
                (PGlobalMemoryStatusEx)GetProcAddress(hKernel32, "GlobalMemoryStatusEx");
            if (pGlobalMemoryStatusEx) {
                MEMORYSTATUSEX memInfo;
                ZeroMemory(&memInfo, sizeof(memInfo));
                memInfo.dwLength = sizeof(memInfo);

                if (pGlobalMemoryStatusEx(&memInfo)) {
                    status.memoryLoad = memInfo.dwMemoryLoad;
                    status.totalPhys = static_cast<uint64_t>(memInfo.ullTotalPhys);
                    status.availPhys = static_cast<uint64_t>(memInfo.ullAvailPhys);
                    return true;
                }
            }
        }

        MEMORYSTATUS legacyInfo;
        ZeroMemory(&legacyInfo, sizeof(legacyInfo));
        legacyInfo.dwLength = sizeof(legacyInfo);
        GlobalMemoryStatus(&legacyInfo);

        if (legacyInfo.dwTotalPhys == 0) {
            return false;
        }

        status.memoryLoad = legacyInfo.dwMemoryLoad;
        status.totalPhys = static_cast<uint64_t>(legacyInfo.dwTotalPhys);
        status.availPhys = static_cast<uint64_t>(legacyInfo.dwAvailPhys);
        return true;
    }
#elif defined(__linux__)
    static bool QueryMemoryStatusLinux(MemoryStatusSnapshot& status) {
        struct sysinfo info;
        if (sysinfo(&info) != 0) {
            return false;
        }

        const uint64_t total = static_cast<uint64_t>(info.totalram) * info.mem_unit;
        const uint64_t avail = static_cast<uint64_t>(info.freeram) * info.mem_unit;

        status.totalPhys = total;
        status.availPhys = avail;

        if (total == 0) {
            status.memoryLoad = 0;
        } else {
            const uint64_t used = total - avail;
            status.memoryLoad = static_cast<unsigned long>((used * 100ULL) / total);
        }

        return true;
    }
#endif
};
