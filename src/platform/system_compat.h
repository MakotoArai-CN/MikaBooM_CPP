#pragma once

#include <stdint.h>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <psapi.h>
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

struct ProcessMemorySnapshot {
    uint64_t workingSetBytes;
    uint64_t privateBytes;
    bool approximate;

    ProcessMemorySnapshot()
        : workingSetBytes(0), privateBytes(0), approximate(true) {}
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

    static bool QueryCurrentProcessMemory(ProcessMemorySnapshot& snapshot) {
#ifdef _WIN32
        return QueryCurrentProcessMemoryWindows(snapshot);
#else
        (void)snapshot;
        return false;
#endif
    }

#ifdef _WIN32
    static bool QueryRegionResidentBytes(void* base, size_t sizeBytes, size_t strideBytes,
                                         uint64_t& residentBytes, bool& approximate) {
        residentBytes = 0;
        approximate = true;
        if (!base || sizeBytes == 0) {
            return false;
        }

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        const size_t pageSize = sysInfo.dwPageSize ? (size_t)sysInfo.dwPageSize : 4096;
        const size_t stride = strideBytes < pageSize ? pageSize : strideBytes;
        const size_t pageCount = (sizeBytes + pageSize - 1) / pageSize;
        if (pageCount == 0) {
            return false;
        }

        typedef BOOL (WINAPI *PQueryWorkingSetEx)(HANDLE, PVOID, DWORD);
        HMODULE hPsapi = GetModuleHandleA("psapi.dll");
        if (!hPsapi) {
            hPsapi = LoadLibraryA("psapi.dll");
        }
        if (!hPsapi) {
            return false;
        }

        PQueryWorkingSetEx pQueryWorkingSetEx =
            (PQueryWorkingSetEx)GetProcAddress(hPsapi, "QueryWorkingSetEx");
        if (!pQueryWorkingSetEx) {
            return false;
        }

        const size_t sampleCount = (sizeBytes + stride - 1) / stride;
        if (sampleCount == 0) {
            return false;
        }

        PSAPI_WORKING_SET_EX_INFORMATION* entries =
            new PSAPI_WORKING_SET_EX_INFORMATION[sampleCount];
        if (!entries) {
            return false;
        }

        size_t index = 0;
        for (size_t offset = 0; offset < sizeBytes; offset += stride) {
            entries[index].VirtualAddress = (PVOID)((char*)base + offset);
            ++index;
        }

        BOOL ok = pQueryWorkingSetEx(GetCurrentProcess(), entries,
                                     (DWORD)(sizeof(PSAPI_WORKING_SET_EX_INFORMATION) * index));
        if (!ok) {
            delete[] entries;
            return false;
        }

        size_t residentPages = 0;
        for (size_t i = 0; i < index; ++i) {
            if (entries[i].VirtualAttributes.Valid) {
                ++residentPages;
            }
        }

        delete[] entries;

        residentBytes = (uint64_t)residentPages * stride;
        if (residentBytes > (uint64_t)sizeBytes) {
            residentBytes = (uint64_t)sizeBytes;
        }
        approximate = (stride != pageSize);
        return true;
    }
#endif

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

    static bool QueryCurrentProcessMemoryWindows(ProcessMemorySnapshot& snapshot) {
#if defined(PROCESS_MEMORY_COUNTERS_EX)
        PROCESS_MEMORY_COUNTERS_EX counters;
        ZeroMemory(&counters, sizeof(counters));
        counters.cb = sizeof(counters);

        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&counters, sizeof(counters))) {
            snapshot.workingSetBytes = static_cast<uint64_t>(counters.WorkingSetSize);
            snapshot.privateBytes = static_cast<uint64_t>(counters.PrivateUsage);
            snapshot.approximate = false;
            return true;
        }
#endif

        PROCESS_MEMORY_COUNTERS legacyCounters;
        ZeroMemory(&legacyCounters, sizeof(legacyCounters));
        legacyCounters.cb = sizeof(legacyCounters);
        if (GetProcessMemoryInfo(GetCurrentProcess(), &legacyCounters, sizeof(legacyCounters))) {
            snapshot.workingSetBytes = static_cast<uint64_t>(legacyCounters.WorkingSetSize);
            snapshot.privateBytes = 0;
            snapshot.approximate = true;
            return true;
        }

        return false;
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
