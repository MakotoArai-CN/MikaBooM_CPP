#include "cpu_worker.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

CPUWorker::CPUWorker(int thresh)
    : threshold(thresh), running(0), intensity(30), lastAdjustTime(0) {
    
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    numWorkers = sysInfo.dwNumberOfProcessors;
    
    InitializeCriticalSection(&adjustLock);
}

CPUWorker::~CPUWorker() {
    Stop();
    DeleteCriticalSection(&adjustLock);
}

void CPUWorker::Start() {
    if (InterlockedCompareExchange(&running, 1, 0) != 0) return;
    
    InterlockedExchange(&intensity, 30);  // 初始强度30%
    lastAdjustTime = GetTickCount();
    
    workers.clear();
    for (int i = 0; i < numWorkers; i++) {
        HANDLE hThread = CreateThread(NULL, 0, WorkerThreadProc, this, 0, NULL);
        if (hThread) {
            SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
            workers.push_back(hThread);
        }
    }
}

void CPUWorker::Stop() {
    if (InterlockedCompareExchange(&running, 0, 1) != 1) return;
    
    if (!workers.empty()) {
        WaitForMultipleObjects(workers.size(), &workers[0], TRUE, 5000);
        for (size_t i = 0; i < workers.size(); i++) {
            CloseHandle(workers[i]);
        }
    }
    
    workers.clear();
    InterlockedExchange(&intensity, 0);
}

DWORD WINAPI CPUWorker::WorkerThreadProc(LPVOID lpParam) {
    CPUWorker* self = (CPUWorker*)lpParam;
    self->WorkerThread();
    return 0;
}

void CPUWorker::WorkerThread() {
    while (running) {
        LONG currentIntensity = intensity;
        
        // 100ms周期，根据强度分配工作和休眠时间
        DWORD workDuration = currentIntensity;      // 0-100 ms
        DWORD sleepDuration = 100 - currentIntensity; // 100-0 ms
        
        if (workDuration > 0) {
            DWORD startTime = GetTickCount();
            while (GetTickCount() - startTime < workDuration && running) {
                DoWork(currentIntensity);
            }
        }
        
        if (sleepDuration > 0 && running) {
            Sleep(sleepDuration);
        }
    }
}

void CPUWorker::DoWork(int intensityLevel) {
    // 密集计算，与Go版本等效
    volatile double result = 0;
    
    // 1. 计算Pi（泰勒级数）
    for (int i = 0; i < 1000; i++) {
        result += pow(-1.0, i) / (2.0 * i + 1.0);
    }
    volatile double piApprox = result * 4.0;
    
    // 2. 三角函数计算
    for (int i = 0; i < 100; i++) {
        double angle = i * 0.1;
        result += sin(angle) * cos(angle) * tan(angle);
    }
    
    // 3. 矩阵计算
    volatile double matrix[10][10];
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            matrix[i][j] = i * j * sin(i + j);
        }
    }
    
    // 4. 对数和指数
    for (int i = 1; i < 100; i++) {
        result += log((double)i) * exp((double)(i % 10));
    }
    
    // 5. 平方根和幂运算
    for (int i = 1; i < 100; i++) {
        result += sqrt((double)i) * pow((double)i, 1.5);
    }
    
    // 防止编译器优化
    (void)piApprox;
    (void)matrix;
}

// 与Go版本完全一致的估算方法
double CPUWorker::GetUsage() const {
    if (!running) return 0;
    
    // estimatedUsage = (intensity / 100.0) * workers * 100.0 / NumCPU
    double estimatedUsage = (intensity / 100.0) * numWorkers * 100.0 / numWorkers;
    
    if (estimatedUsage > 100) estimatedUsage = 100;
    return estimatedUsage;
}

// 与Go版本完全一致的调整逻辑
void CPUWorker::AdjustLoad(double currentWorkerUsage, double targetWorkerUsage) {
    if (!running) return;
    
    EnterCriticalSection(&adjustLock);
    
    DWORD now = GetTickCount();
    // 500ms调整间隔
    if (now - lastAdjustTime < 500) {
        LeaveCriticalSection(&adjustLock);
        return;
    }
    lastAdjustTime = now;
    
    // 计算误差
    double diff = targetWorkerUsage - currentWorkerUsage;
    
    LONG currentIntensity = intensity;
    LONG newIntensity = currentIntensity;
    
    // 阶梯式调整，与Go版本完全一致
    if (diff > 20) {
        newIntensity += 10;
    } else if (diff > 10) {
        newIntensity += 5;
    } else if (diff > 5) {
        newIntensity += 3;
    } else if (diff > 2) {
        newIntensity += 2;
    } else if (diff > 0.5) {
        newIntensity += 1;
    } else if (diff < -20) {
        newIntensity -= 10;
    } else if (diff < -10) {
        newIntensity -= 5;
    } else if (diff < -5) {
        newIntensity -= 3;
    } else if (diff < -2) {
        newIntensity -= 2;
    } else if (diff < -0.5) {
        newIntensity -= 1;
    }
    
    // 限制范围
    if (newIntensity < 0) newIntensity = 0;
    if (newIntensity > 100) newIntensity = 100;
    
    InterlockedExchange(&intensity, newIntensity);
    
    LeaveCriticalSection(&adjustLock);
}