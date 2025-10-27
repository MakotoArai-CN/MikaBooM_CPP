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
    
    // 根据CPU核心数调整冷却时间
    // 核心多，调整可以更频繁
    if (numWorkers >= 16) {
        adjustCooldown = 300;  // 16核以上：300ms
    } else if (numWorkers >= 8) {
        adjustCooldown = 400;  // 8-15核：400ms
    } else {
        adjustCooldown = 500;  // 8核以下：500ms
    }
}

CPUWorker::~CPUWorker() {
    Stop();
    DeleteCriticalSection(&adjustLock);
}

void CPUWorker::Start() {
    if (InterlockedCompareExchange(&running, 1, 0) != 0) return;
    
    InterlockedExchange(&intensity, 30);
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
        WaitForMultipleObjects((DWORD)workers.size(), &workers[0], TRUE, 5000);
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
        DWORD workDuration = currentIntensity;
        DWORD sleepDuration = 100 - currentIntensity;
        
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
    // 增强计算负载以提高精确度
    volatile double result = 0;
    int iterations = 100 + intensityLevel * 10;
    
    // 1. 计算π的近似值
    for (int i = 0; i < iterations; i++) {
        result += pow(-1.0, i) / (2.0 * i + 1.0);
    }
    volatile double piApprox = result * 4.0;
    
    // 2. 三角函数计算
    for (int i = 0; i < iterations; i++) {
        double angle = i * 0.1;
        result += sin(angle) * cos(angle) * tan(angle);
    }
    
    // 3. 矩阵运算
    volatile double matrix[10][10];
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            matrix[i][j] = i * j * sin(i + j);
        }
    }
    
    // 4. 对数和指数运算
    for (int i = 1; i < iterations; i++) {
        result += log((double)i) * exp((double)(i % 10));
    }
    
    // 5. 开方和幂运算
    for (int i = 1; i < iterations; i++) {
        result += sqrt((double)i) * pow((double)i, 1.5);
    }
    
    (void)piApprox;
    (void)matrix;
}

double CPUWorker::GetUsage() const {
    if (!running) return 0;
    
    double estimatedUsage = intensity;
    if (estimatedUsage > 100) estimatedUsage = 100;
    return estimatedUsage;
}

void CPUWorker::AdjustLoad(double currentWorkerUsage, double targetWorkerUsage) {
    if (!running) return;
    
    EnterCriticalSection(&adjustLock);
    
    DWORD now = GetTickCount();
    if (now - lastAdjustTime < (DWORD)adjustCooldown) {
        LeaveCriticalSection(&adjustLock);
        return;
    }
    lastAdjustTime = now;
    
    double diff = targetWorkerUsage - currentWorkerUsage;
    LONG currentIntensity = intensity;
    LONG newIntensity = currentIntensity;
    
    // 使用更平滑的PID控制策略
    if (diff > 20) {
        newIntensity += 8;
    } else if (diff > 10) {
        newIntensity += 5;
    } else if (diff > 5) {
        newIntensity += 3;
    } else if (diff > 2) {
        newIntensity += 2;
    } else if (diff > 0.5) {
        newIntensity += 1;
    } else if (diff < -20) {
        newIntensity -= 8;
    } else if (diff < -10) {
        newIntensity -= 5;
    } else if (diff < -5) {
        newIntensity -= 3;
    } else if (diff < -2) {
        newIntensity -= 2;
    } else if (diff < -0.5) {
        newIntensity -= 1;
    }
    
    if (newIntensity < 0) newIntensity = 0;
    if (newIntensity > 100) newIntensity = 100;
    
    InterlockedExchange(&intensity, newIntensity);
    
    LeaveCriticalSection(&adjustLock);
}