#pragma once
#include <windows.h>
#include <vector>
#include <stdint.h>

struct MemoryBlockInfo {
    void* ptr;
    int64_t sizeBytes;
    DWORD allocatedTick;
    DWORD lastTouchTick;

    MemoryBlockInfo() : ptr(NULL), sizeBytes(0), allocatedTick(0), lastTouchTick(0) {}
    MemoryBlockInfo(void* p, int64_t size, DWORD tick)
        : ptr(p), sizeBytes(size), allocatedTick(tick), lastTouchTick(tick) {}
};

struct MemoryWorkerStats {
    int64_t targetBytes;
    int64_t allocatedBytes;
    int64_t residentBytes;
    size_t blockCount;
    DWORD lastRefreshTick;
    bool refreshEnabled;
    bool residentApproximate;

    MemoryWorkerStats()
        : targetBytes(0), allocatedBytes(0), residentBytes(0), blockCount(0),
          lastRefreshTick(0), refreshEnabled(false), residentApproximate(false) {}
};

class MemoryWorker {
private:
    volatile LONG running;
    std::vector<MemoryBlockInfo> allocatedMemory;
    volatile LONG targetSizeMB;
    uint64_t totalMemoryBytes;
    HANDLE workerThread;
    DWORD lastAdjustTime;
    CRITICAL_SECTION allocLock;

    int64_t optimalChunkSize;
    int64_t maxAdjustPerCycle;

    int randomMinMB;
    int randomMaxMB;
    int randomIntervalMinSec;
    int randomIntervalMaxSec;
    bool refreshEnabled;
    int refreshAfterSec;
    int refreshIntervalSec;
    int refreshStrideKB;

    DWORD nextRandomizeTick;
    DWORD lastRefreshTick;
    int64_t randomTargetBytes;
    size_t refreshCursor;
    int64_t residentBytesCache;
    bool residentApproximate;

public:
    MemoryWorker(int threshold, uint64_t totalMemory);
    ~MemoryWorker();

    void Start();
    void Stop();
    bool IsRunning() const { return running != 0; }

    void AdjustLoad(double currentWorkerUsage, double targetWorkerUsage);
    int64_t GetAllocatedSize() const;
    int64_t GetTargetSize() const { return targetSizeMB * 1024LL * 1024LL; }
    double GetUsage() const;
    void SetTotalMemory(uint64_t total) { totalMemoryBytes = total; }
    void ConfigureRandomRange(int minMB, int maxMB, int intervalMinSec, int intervalMaxSec);
    void ConfigureRefresh(bool enabled, int afterSec, int intervalSec, int strideKB);
    MemoryWorkerStats GetStats() const;

private:
    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    void WorkerLoop();
    void AllocateMemory(int64_t sizeBytes);
    void FreeMemory(int64_t sizeBytes);
    void CalculateOptimalParameters();
    void RefreshAllocatedPages(DWORD nowTick);
    void UpdateResidentStats();
    int64_t CalculateResidentBytesLocked(bool& approximate) const;
    int64_t PickRandomTargetBytes(int64_t maxAllowedBytes, DWORD nowTick);
    DWORD PickNextRandomTick(DWORD nowTick) const;
};
