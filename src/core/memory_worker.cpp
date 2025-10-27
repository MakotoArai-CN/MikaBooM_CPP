#include "memory_worker.h"
#include "../utils/system_info.h"

MemoryWorker::MemoryWorker(int thresh, uint64_t totalMemory)
    : threshold(thresh), running(0), targetSizeMB(0), 
      totalMemoryBytes(totalMemory), workerThread(NULL), lastAdjustTime(0),
      optimalChunkSize(0), maxAdjustPerCycle(0) {
    InitializeCriticalSection(&allocLock);
    CalculateOptimalParameters();
}

MemoryWorker::~MemoryWorker() {
    Stop();
    DeleteCriticalSection(&allocLock);
}

void MemoryWorker::CalculateOptimalParameters() {
    // 根据总内存大小动态计算最优参数
    double totalGB = totalMemoryBytes / (1024.0 * 1024.0 * 1024.0);
    
    // 分配块大小：内存越大，块越大
    // 小于16GB: 10MB
    // 16-64GB: 20MB
    // 64-256GB: 50MB
    // 大于256GB: 100MB
    if (totalGB < 16.0) {
        optimalChunkSize = 10LL * 1024 * 1024;
    } else if (totalGB < 64.0) {
        optimalChunkSize = 20LL * 1024 * 1024;
    } else if (totalGB < 256.0) {
        optimalChunkSize = 50LL * 1024 * 1024;
    } else {
        optimalChunkSize = 100LL * 1024 * 1024;
    }
    
    // 每次最大调整量：总内存的2%（最少256MB，最多8GB）
    maxAdjustPerCycle = (int64_t)(totalMemoryBytes * 0.02);
    if (maxAdjustPerCycle < 256LL * 1024 * 1024) {
        maxAdjustPerCycle = 256LL * 1024 * 1024;
    }
    if (maxAdjustPerCycle > 8LL * 1024 * 1024 * 1024) {
        maxAdjustPerCycle = 8LL * 1024 * 1024 * 1024;
    }
}

void MemoryWorker::Start() {
    if (InterlockedCompareExchange(&running, 1, 0) != 0) return;
    
    InterlockedExchange(&targetSizeMB, 0);
    lastAdjustTime = GetTickCount();
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
        VirtualFree(allocatedMemory[i], 0, MEM_RELEASE);
    }
    allocatedMemory.clear();
    LeaveCriticalSection(&allocLock);
    
    InterlockedExchange(&targetSizeMB, 0);
}

DWORD WINAPI MemoryWorker::WorkerThreadProc(LPVOID lpParam) {
    MemoryWorker* self = (MemoryWorker*)lpParam;
    self->WorkerLoop();
    return 0;
}

void MemoryWorker::WorkerLoop() {
    while (running) {
        int64_t targetBytes = (int64_t)targetSizeMB * 1024 * 1024;
        int64_t currentBytes = GetAllocatedSize();
        int64_t diff = targetBytes - currentBytes;
        
        if (diff > 0) {
            AllocateMemory(diff);
        } else if (diff < 0) {
            FreeMemory(-diff);
        }
        
        Sleep(1000);
    }
}

void MemoryWorker::AllocateMemory(int64_t sizeBytes) {
    DWORD major, minor;
    SystemInfo::GetRealWindowsVersion(major, minor);
    
    // 使用动态计算的块大小
    int64_t actualChunkSize = optimalChunkSize;
    
    // Windows XP/2003使用较小的块
    if (major < 6) {
        actualChunkSize = actualChunkSize / 2;
        if (actualChunkSize < 5LL * 1024 * 1024) {
            actualChunkSize = 5LL * 1024 * 1024;
        }
    }
    
    int64_t chunks = sizeBytes / actualChunkSize;
    
    EnterCriticalSection(&allocLock);
    for (int64_t i = 0; i < chunks && running; i++) {
        void* chunk = VirtualAlloc(NULL, (SIZE_T)actualChunkSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (chunk) {
            // 触碰内存页以确保分配
            for (size_t j = 0; j < (size_t)actualChunkSize; j += 4096) {
                ((char*)chunk)[j] = (char)(j % 256);
            }
            allocatedMemory.push_back(chunk);
        } else {
            break;
        }
        
        if (major < 6) {
            Sleep(10);
        }
    }
    
    int64_t remainder = sizeBytes % actualChunkSize;
    if (remainder > 0 && running) {
        void* chunk = VirtualAlloc(NULL, (SIZE_T)remainder, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (chunk) {
            for (size_t j = 0; j < (size_t)remainder; j += 4096) {
                ((char*)chunk)[j] = (char)(j % 256);
            }
            allocatedMemory.push_back(chunk);
        }
    }
    LeaveCriticalSection(&allocLock);
}

void MemoryWorker::FreeMemory(int64_t sizeBytes) {
    int64_t freed = 0;
    
    EnterCriticalSection(&allocLock);
    while (freed < sizeBytes && !allocatedMemory.empty()) {
        void* ptr = allocatedMemory.back();
        allocatedMemory.pop_back();
        
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(ptr, &mbi, sizeof(mbi))) {
            freed += mbi.RegionSize;
            VirtualFree(ptr, 0, MEM_RELEASE);
        }
    }
    LeaveCriticalSection(&allocLock);
}

int64_t MemoryWorker::GetAllocatedSize() const {
    int64_t total = 0;
    
    EnterCriticalSection((LPCRITICAL_SECTION)&allocLock);
    for (size_t i = 0; i < allocatedMemory.size(); i++) {
        MEMORY_BASIC_INFORMATION mbi;
        if (VirtualQuery(allocatedMemory[i], &mbi, sizeof(mbi))) {
            total += mbi.RegionSize;
        }
    }
    LeaveCriticalSection((LPCRITICAL_SECTION)&allocLock);
    
    return total;
}

double MemoryWorker::GetUsage() const {
    if (!running || totalMemoryBytes == 0) return 0;
    
    int64_t allocated = GetAllocatedSize();
    double usage = (double)allocated / totalMemoryBytes * 100.0;
    
    if (usage > 100) usage = 100;
    return usage;
}

void MemoryWorker::AdjustLoad(double currentWorkerUsage, double targetWorkerUsage) {
    if (!running) return;
    
    DWORD now = GetTickCount();
    if (now - lastAdjustTime < 1000) {
        return;
    }
    lastAdjustTime = now;
    
    int64_t targetBytes = (int64_t)(totalMemoryBytes * targetWorkerUsage / 100.0);
    
    // 最大不超过总内存的95%
    int64_t maxBytes = (int64_t)(totalMemoryBytes * 0.95);
    if (targetBytes > maxBytes) targetBytes = maxBytes;
    
    int64_t currentBytes = GetAllocatedSize();
    int64_t diff = targetBytes - currentBytes;
    
    // 根据差距和总内存大小动态调整步长
    int64_t adjustStep;
    double diffPercent = (double)abs(diff) / totalMemoryBytes * 100.0;
    
    if (diffPercent > 10.0) {
        // 差距>10%，快速调整
        adjustStep = maxAdjustPerCycle;
    } else if (diffPercent > 5.0) {
        // 差距>5%，中速调整
        adjustStep = maxAdjustPerCycle / 2;
    } else if (diffPercent > 2.0) {
        // 差距>2%，慢速调整
        adjustStep = maxAdjustPerCycle / 4;
    } else {
        // 差距<2%，微调
        adjustStep = maxAdjustPerCycle / 8;
    }
    
    // 确保至少有64MB的调整
    if (adjustStep < 64LL * 1024 * 1024) {
        adjustStep = 64LL * 1024 * 1024;
    }
    
    if (targetBytes > currentBytes + adjustStep) {
        targetBytes = currentBytes + adjustStep;
    } else if (targetBytes < currentBytes - adjustStep) {
        targetBytes = currentBytes - adjustStep;
    }
    
    if (targetBytes < 0) targetBytes = 0;
    
    InterlockedExchange(&targetSizeMB, (LONG)(targetBytes / (1024 * 1024)));
}