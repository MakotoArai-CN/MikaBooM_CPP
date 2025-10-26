#pragma once
#include <windows.h>
#include <vector>
#include <stdint.h>

class MemoryWorker {
private:
    int threshold;
    volatile LONG running;
    std::vector<void*> allocatedMemory;
    volatile LONG targetSizeMB;
    int64_t totalMemoryBytes;
    HANDLE workerThread;
    DWORD lastAdjustTime;
    CRITICAL_SECTION allocLock;

public:
    MemoryWorker(int threshold);
    ~MemoryWorker();
    
    void Start();
    void Stop();
    bool IsRunning() const { return running != 0; }
    void AdjustLoad(double currentWorkerUsage, double targetWorkerUsage);
    int64_t GetAllocatedSize() const;
    int64_t GetTargetSize() const { return targetSizeMB * 1024LL * 1024LL; }
    double GetUsage() const;
    void SetTotalMemory(uint64_t total) { totalMemoryBytes = total; }

private:
    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    void WorkerLoop();
    void AllocateMemory(int64_t sizeBytes);
    void FreeMemory(int64_t sizeBytes);
};