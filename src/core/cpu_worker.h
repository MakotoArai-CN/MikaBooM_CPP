#pragma once
#include <windows.h>
#include <vector>

class CPUWorker {
private:
    int threshold;
    volatile LONG running;
    volatile LONG intensity;
    std::vector<HANDLE> workers;
    int numWorkers;
    DWORD lastAdjustTime;
    CRITICAL_SECTION adjustLock;
    
    // 动态调整参数
    int adjustCooldown;  // 调整冷却时间（毫秒）

public:
    CPUWorker(int threshold);
    ~CPUWorker();
    
    void Start();
    void Stop();
    bool IsRunning() const { return running != 0; }
    
    void AdjustLoad(double currentWorkerUsage, double targetWorkerUsage);
    int GetIntensity() const { return intensity; }
    double GetUsage() const;

private:
    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    void WorkerThread();
    void DoWork(int intensityLevel);
};