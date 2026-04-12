#include "memory_worker.h"
#include "../utils/system_info.h"
#include "../utils/anti_detect.h"
#include "../platform/system_compat.h"
#include <algorithm>
#include <stdlib.h>

MemoryWorker::MemoryWorker(int thresh, uint64_t totalMemory)
    : running(0), targetSizeMB(0),
      totalMemoryBytes(totalMemory), workerThread(NULL), lastAdjustTime(0),
      optimalChunkSize(0), maxAdjustPerCycle(0),
      randomMinMB(thresh > 0 ? thresh * 128 / 100 : 256),
      randomMaxMB(thresh > 0 ? thresh * 256 / 100 : 512),
      randomIntervalMinSec(30), randomIntervalMaxSec(90),
      refreshEnabled(true), refreshAfterSec(60), refreshIntervalSec(30), refreshStrideKB(4),
      nextRandomizeTick(0), lastRefreshTick(0), randomTargetBytes(0), refreshCursor(0),
      residentBytesCache(0), residentApproximate(true) {
    InitializeCriticalSection(&allocLock);
    CalculateOptimalParameters();
}

MemoryWorker::~MemoryWorker() {
    Stop();
    DeleteCriticalSection(&allocLock);
}

void MemoryWorker::CalculateOptimalParameters() {
    double totalGB = totalMemoryBytes / (1024.0 * 1024.0 * 1024.0);

    if (totalGB < 16.0) {
        optimalChunkSize = 10LL * 1024 * 1024;
    } else if (totalGB < 64.0) {
        optimalChunkSize = 20LL * 1024 * 1024;
    } else if (totalGB < 256.0) {
        optimalChunkSize = 50LL * 1024 * 1024;
    } else {
        optimalChunkSize = 100LL * 1024 * 1024;
    }

    maxAdjustPerCycle = (int64_t)(totalMemoryBytes * 0.02);
    if (maxAdjustPerCycle < 256LL * 1024 * 1024) {
        maxAdjustPerCycle = 256LL * 1024 * 1024;
    }
    if (maxAdjustPerCycle > 8LL * 1024 * 1024 * 1024) {
        maxAdjustPerCycle = 8LL * 1024 * 1024 * 1024;
    }
}

void MemoryWorker::ConfigureRandomRange(int minMB, int maxMB, int intervalMinSec, int intervalMaxSec) {
    if (minMB < 0) minMB = 0;
    if (maxMB < minMB) maxMB = minMB;
    if (intervalMinSec < 1) intervalMinSec = 1;
    if (intervalMaxSec < intervalMinSec) intervalMaxSec = intervalMinSec;

    randomMinMB = minMB;
    randomMaxMB = maxMB;
    randomIntervalMinSec = intervalMinSec;
    randomIntervalMaxSec = intervalMaxSec;
}

void MemoryWorker::ConfigureRefresh(bool enabled, int afterSec, int intervalSec, int strideKB) {
    refreshEnabled = enabled;
    refreshAfterSec = afterSec < 0 ? 0 : afterSec;
    refreshIntervalSec = intervalSec < 1 ? 1 : intervalSec;
    refreshStrideKB = strideKB < 4 ? 4 : strideKB;
}

void MemoryWorker::Start() {
    if (InterlockedCompareExchange(&running, 1, 0) != 0) return;

    InterlockedExchange(&targetSizeMB, 0);
    lastAdjustTime = GetTickCount();
    nextRandomizeTick = lastAdjustTime;
    lastRefreshTick = 0;
    randomTargetBytes = 0;
    refreshCursor = 0;
    residentBytesCache = 0;
    residentApproximate = true;

    RandomDelay(100, 300);

    workerThread = CreateThread(NULL, 0, WorkerThreadProc, this, 0, NULL);
}

void MemoryWorker::Stop() {
    if (InterlockedCompareExchange(&running, 0, 1) != 1) return;

    if (workerThread) {
        WaitForSingleObject(workerThread, 5000);
        CloseHandle(workerThread);
        workerThread = NULL;
    }

    EnterCriticalSection(&allocLock);

    for (size_t i = 0; i < allocatedMemory.size(); i++) {
        VirtualFree(allocatedMemory[i].ptr, 0, MEM_RELEASE);
    }
    allocatedMemory.clear();
    residentBytesCache = 0;
    refreshCursor = 0;

    LeaveCriticalSection(&allocLock);

    InterlockedExchange(&targetSizeMB, 0);
}

DWORD WINAPI MemoryWorker::WorkerThreadProc(LPVOID lpParam) {
    MemoryWorker* self = (MemoryWorker*)lpParam;
    self->WorkerLoop();
    return 0;
}

DWORD MemoryWorker::PickNextRandomTick(DWORD nowTick) const {
    int interval = randomIntervalMinSec;
    if (randomIntervalMaxSec > randomIntervalMinSec) {
        interval += rand() % (randomIntervalMaxSec - randomIntervalMinSec + 1);
    }
    if (interval < 1) interval = 1;
    return nowTick + (DWORD)(interval * 1000UL);
}

int64_t MemoryWorker::PickRandomTargetBytes(int64_t maxAllowedBytes, DWORD nowTick) {
    int64_t minBytes = (int64_t)randomMinMB * 1024 * 1024;
    int64_t maxBytes = (int64_t)randomMaxMB * 1024 * 1024;

    if (maxBytes < minBytes) {
        maxBytes = minBytes;
    }
    if (maxAllowedBytes < 0) {
        maxAllowedBytes = 0;
    }
    if (maxBytes > maxAllowedBytes) {
        maxBytes = maxAllowedBytes;
    }
    if (minBytes > maxBytes) {
        minBytes = maxBytes;
    }

    int64_t picked = maxBytes;
    if (maxBytes > minBytes) {
        const int64_t range = maxBytes - minBytes;
        picked = minBytes + (rand() % (int)(range / (1024 * 1024) + 1)) * 1024LL * 1024LL;
        if (picked > maxBytes) picked = maxBytes;
    }

    randomTargetBytes = picked;
    nextRandomizeTick = PickNextRandomTick(nowTick);
    return picked;
}

void MemoryWorker::WorkerLoop() {
    while (running) {
        DWORD now = GetTickCount();
        int64_t targetBytes = (int64_t)targetSizeMB * 1024 * 1024;
        int64_t currentBytes = GetAllocatedSize();
        int64_t diff = targetBytes - currentBytes;

        if (diff > 0) {
            AllocateMemory(diff);
        } else if (diff < 0) {
            FreeMemory(-diff);
        }

        if (refreshEnabled && now - lastRefreshTick >= (DWORD)(refreshIntervalSec * 1000UL)) {
            RefreshAllocatedPages(now);
        }

        UpdateResidentStats();
        Sleep(1000);
    }
}

void MemoryWorker::AllocateMemory(int64_t sizeBytes) {
    DWORD major, minor;
    SystemInfo::GetRealWindowsVersion(major, minor);

    int64_t actualChunkSize = optimalChunkSize;

    if (major < 6) {
        actualChunkSize = actualChunkSize / 2;
        if (actualChunkSize < 5LL * 1024 * 1024) {
            actualChunkSize = 5LL * 1024 * 1024;
        }
    }

    int64_t chunks = sizeBytes / actualChunkSize;

    EnterCriticalSection(&allocLock);

    for (int64_t i = 0; i < chunks && running; i++) {
        int variation = (rand() % 40) - 20;
        int64_t variedSize = actualChunkSize + (actualChunkSize * variation / 100);
        if (variedSize < 1024 * 1024) variedSize = 1024 * 1024;

        RandomDelay(5, 50);

        void* chunk = VirtualAlloc(NULL, (SIZE_T)variedSize,
                                   MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

        if (chunk) {
            size_t writeSize = (size_t)variedSize;
            for (size_t j = 0; j < writeSize; j += 4096) {
                DWORD tick = GetTickCount();
                ((char*)chunk)[j] = (char)((tick + j) % 256);
            }
            allocatedMemory.push_back(MemoryBlockInfo(chunk, variedSize, GetTickCount()));

            if (i % 5 == 0 && i > 0) {
                Sleep(100 + (rand() % 100));
            }
        } else {
            break;
        }

        if (major < 6) {
            Sleep(10 + (rand() % 20));
        }
    }

    int64_t remainder = sizeBytes % actualChunkSize;
    if (remainder > 0 && running) {
        RandomDelay(10, 30);

        void* chunk = VirtualAlloc(NULL, (SIZE_T)remainder,
                                   MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (chunk) {
            for (size_t j = 0; j < (size_t)remainder; j += 4096) {
                DWORD tick = GetTickCount();
                ((char*)chunk)[j] = (char)((tick + j) % 256);
            }
            allocatedMemory.push_back(MemoryBlockInfo(chunk, remainder, GetTickCount()));
        }
    }

    LeaveCriticalSection(&allocLock);
}

void MemoryWorker::FreeMemory(int64_t sizeBytes) {
    int64_t freed = 0;

    EnterCriticalSection(&allocLock);

    while (freed < sizeBytes && !allocatedMemory.empty()) {
        MemoryBlockInfo block = allocatedMemory.back();
        allocatedMemory.pop_back();

        freed += block.sizeBytes;
        if (block.ptr) {
            VirtualFree(block.ptr, 0, MEM_RELEASE);
        }

        if (refreshCursor >= allocatedMemory.size()) {
            refreshCursor = allocatedMemory.empty() ? 0 : allocatedMemory.size() - 1;
        }

        RandomDelay(1, 10);
    }

    LeaveCriticalSection(&allocLock);
}

int64_t MemoryWorker::GetAllocatedSize() const {
    int64_t total = 0;

    EnterCriticalSection((LPCRITICAL_SECTION)&allocLock);
    for (size_t i = 0; i < allocatedMemory.size(); i++) {
        total += allocatedMemory[i].sizeBytes;
    }
    LeaveCriticalSection((LPCRITICAL_SECTION)&allocLock);

    return total;
}

int64_t MemoryWorker::CalculateResidentBytesLocked(bool& approximate) const {
    int64_t residentTotal = 0;
    bool anyAccurate = false;
    approximate = true;

    const size_t strideBytes = (size_t)refreshStrideKB * 1024;
    for (size_t i = 0; i < allocatedMemory.size(); ++i) {
        uint64_t blockResident = 0;
        bool blockApproximate = true;
        if (SystemCompat::QueryRegionResidentBytes(allocatedMemory[i].ptr,
                                                   (size_t)allocatedMemory[i].sizeBytes,
                                                   strideBytes,
                                                   blockResident,
                                                   blockApproximate)) {
            residentTotal += (int64_t)blockResident;
            if (!blockApproximate) {
                anyAccurate = true;
            }
        }
    }

    if (!allocatedMemory.empty() && anyAccurate) {
        approximate = false;
    }
    return residentTotal;
}

void MemoryWorker::UpdateResidentStats() {
    EnterCriticalSection(&allocLock);

    bool approximate = true;
    int64_t residentBytes = CalculateResidentBytesLocked(approximate);
    if (residentBytes == 0 && !allocatedMemory.empty()) {
        ProcessMemorySnapshot snapshot;
        if (SystemCompat::QueryCurrentProcessMemory(snapshot)) {
            residentBytes = (int64_t)snapshot.workingSetBytes;
            approximate = true;
        }
    }

    residentBytesCache = residentBytes;
    residentApproximate = approximate;

    LeaveCriticalSection(&allocLock);
}

void MemoryWorker::RefreshAllocatedPages(DWORD nowTick) {
    if (!refreshEnabled) return;

    EnterCriticalSection(&allocLock);

    if (allocatedMemory.empty()) {
        lastRefreshTick = nowTick;
        LeaveCriticalSection(&allocLock);
        return;
    }

    if (refreshCursor >= allocatedMemory.size()) {
        refreshCursor = 0;
    }

    const size_t strideBytes = (size_t)refreshStrideKB * 1024;
    size_t refreshedBlocks = 0;
    const size_t maxBlocksPerCycle = allocatedMemory.size() < 4 ? allocatedMemory.size() : 4;

    while (refreshedBlocks < maxBlocksPerCycle && !allocatedMemory.empty()) {
        if (refreshCursor >= allocatedMemory.size()) {
            refreshCursor = 0;
        }

        MemoryBlockInfo& block = allocatedMemory[refreshCursor];
        DWORD heldMs = nowTick - block.allocatedTick;
        if (heldMs >= (DWORD)(refreshAfterSec * 1000UL) && block.ptr && block.sizeBytes > 0) {
            for (size_t offset = 0; offset < (size_t)block.sizeBytes; offset += strideBytes) {
                volatile char* p = ((volatile char*)block.ptr) + offset;
                *p = (char)((*p + 1) & 0xFF);
            }
            block.lastTouchTick = nowTick;
        }

        ++refreshCursor;
        ++refreshedBlocks;
    }

    lastRefreshTick = nowTick;
    LeaveCriticalSection(&allocLock);
}

double MemoryWorker::GetUsage() const {
    if (!running || totalMemoryBytes == 0) return 0;

    int64_t allocated = GetAllocatedSize();
    double usage = (double)allocated / totalMemoryBytes * 100.0;

    if (usage > 100) usage = 100;

    return usage;
}

void MemoryWorker::AdjustLoad(double currentWorkerUsage, double targetWorkerUsage) {
    if (!running || totalMemoryBytes == 0) return;

    DWORD now = GetTickCount();
    if (now - lastAdjustTime < 1000) {
        return;
    }

    lastAdjustTime = now;

    int64_t maxTargetBytes = (int64_t)(totalMemoryBytes * targetWorkerUsage / 100.0);
    int64_t hardMaxBytes = (int64_t)(totalMemoryBytes * 0.95);
    if (maxTargetBytes > hardMaxBytes) maxTargetBytes = hardMaxBytes;
    if (maxTargetBytes < 0) maxTargetBytes = 0;

    int64_t policyTargetBytes = maxTargetBytes;
    if (now >= nextRandomizeTick || randomTargetBytes <= 0 || randomTargetBytes > maxTargetBytes) {
        policyTargetBytes = PickRandomTargetBytes(maxTargetBytes, now);
    } else {
        policyTargetBytes = randomTargetBytes;
    }

    int64_t currentBytes = GetAllocatedSize();
    int64_t diff = policyTargetBytes - currentBytes;

    int64_t adjustStep;
    int64_t absDiff = diff >= 0 ? diff : -diff;
    double diffPercent = (double)absDiff / totalMemoryBytes * 100.0;

    if (diffPercent > 10.0) {
        adjustStep = maxAdjustPerCycle;
    } else if (diffPercent > 5.0) {
        adjustStep = maxAdjustPerCycle / 2;
    } else if (diffPercent > 2.0) {
        adjustStep = maxAdjustPerCycle / 4;
    } else {
        adjustStep = maxAdjustPerCycle / 8;
    }

    if (adjustStep < 64LL * 1024 * 1024) {
        adjustStep = 64LL * 1024 * 1024;
    }

    if (policyTargetBytes > currentBytes + adjustStep) {
        policyTargetBytes = currentBytes + adjustStep;
    } else if (policyTargetBytes < currentBytes - adjustStep) {
        policyTargetBytes = currentBytes - adjustStep;
    }

    if (policyTargetBytes < 0) policyTargetBytes = 0;

    InterlockedExchange(&targetSizeMB, (LONG)(policyTargetBytes / (1024 * 1024)));
    (void)currentWorkerUsage;
}

MemoryWorkerStats MemoryWorker::GetStats() const {
    MemoryWorkerStats stats;
    stats.targetBytes = GetTargetSize();
    stats.refreshEnabled = refreshEnabled;

    EnterCriticalSection((LPCRITICAL_SECTION)&allocLock);
    stats.allocatedBytes = 0;
    for (size_t i = 0; i < allocatedMemory.size(); ++i) {
        stats.allocatedBytes += allocatedMemory[i].sizeBytes;
    }
    stats.residentBytes = residentBytesCache;
    stats.blockCount = allocatedMemory.size();
    stats.lastRefreshTick = lastRefreshTick;
    stats.residentApproximate = residentApproximate;
    LeaveCriticalSection((LPCRITICAL_SECTION)&allocLock);

    return stats;
}
