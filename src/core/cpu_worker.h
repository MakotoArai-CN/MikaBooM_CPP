#pragma once
#include <windows.h>
#include <vector>

class CPUWorker {
private:
    int threshold;
    volatile LONG running;
    volatile LONG intensity;       // 0-100，表示强度百分比
    std::vector<HANDLE> workers;
    int numWorkers;
    DWORD lastAdjustTime;
    CRITICAL_SECTION adjustLock;

public:
    CPUWorker(int threshold);
    ~CPUWorker();
    
    void Start();
    void Stop();
    bool IsRunning() const { return running != 0; }
    void AdjustLoad(double currentWorkerUsage, double targetWorkerUsage);
    int GetIntensity() const { return intensity; }
    double GetUsage() const;  // 估算值，与Go版本一致

private:
    static DWORD WINAPI WorkerThreadProc(LPVOID lpParam);
    void WorkerThread();
    void DoWork(int intensityLevel);
};