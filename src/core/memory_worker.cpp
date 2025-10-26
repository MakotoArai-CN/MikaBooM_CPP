#include "memory_worker.h"
#include "../utils/system_info.h"

MemoryWorker::MemoryWorker(int thresh)
    : threshold(thresh), running(0), targetSizeMB(0),
      totalMemoryBytes(16LL * 1024 * 1024 * 1024),
      workerThread(NULL), lastAdjustTime(0) {
    InitializeCriticalSection(&allocLock);
}

MemoryWorker::~MemoryWorker() {
    Stop();
    DeleteCriticalSection(&allocLock);
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
    // 检测系统版本，老系统使用更小的块
    DWORD major, minor;
    SystemInfo::GetRealWindowsVersion(major, minor);
    
    const int64_t chunkSize = 10LL * 1024 * 1024;  // 10MB
    int64_t actualChunkSize = (major >= 6) ? chunkSize : (5LL * 1024 * 1024);  // 老系统用5MB
    
    int64_t chunks = sizeBytes / actualChunkSize;
    
    EnterCriticalSection(&allocLock);
    
    for (int64_t i = 0; i < chunks && running; i++) {
        void* chunk = VirtualAlloc(NULL, actualChunkSize, 
                                    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (chunk) {
            // 老系统需要更保守的内存访问
            for (size_t j = 0; j < (size_t)actualChunkSize; j += 4096) {
                ((char*)chunk)[j] = (char)(j % 256);
            }
            allocatedMemory.push_back(chunk);
        } else {
            // 分配失败，停止继续分配
            break;
        }
        
        // 老系统添加延迟，避免内存分配过快
        if (major < 6) {
            Sleep(10);
        }
    }
    
    int64_t remainder = sizeBytes % actualChunkSize;
    if (remainder > 0 && running) {
        void* chunk = VirtualAlloc(NULL, remainder, 
                                    MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
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
    if (now - lastAdjustTime < 2000) {
        return;
    }
    lastAdjustTime = now;
    
    int64_t targetBytes = (int64_t)(totalMemoryBytes * targetWorkerUsage / 100.0);
    int64_t maxBytes = (int64_t)(totalMemoryBytes * 0.8);
    if (targetBytes > maxBytes) targetBytes = maxBytes;
    
    int64_t currentBytes = GetAllocatedSize();
    int64_t maxAdjust = 512LL * 1024 * 1024;
    
    if (targetBytes > currentBytes + maxAdjust) {
        targetBytes = currentBytes + maxAdjust;
    } else if (targetBytes < currentBytes - maxAdjust) {
        targetBytes = currentBytes - maxAdjust;
    }
    
    if (targetBytes < 0) targetBytes = 0;
    
    InterlockedExchange(&targetSizeMB, (LONG)(targetBytes / (1024 * 1024)));
}