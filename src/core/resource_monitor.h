#pragma once
#include <windows.h>
#include <psapi.h>
#include <pdh.h>
#include <string>

class ResourceMonitor {
private:
    ULARGE_INTEGER lastIdleTime;
    ULARGE_INTEGER lastKernelTime;
    ULARGE_INTEGER lastUserTime;
    int numProcessors;
    HANDLE self;
    
    typedef BOOL (WINAPI *PGetSystemTimes)(LPFILETIME, LPFILETIME, LPFILETIME);
    PGetSystemTimes pGetSystemTimes;
    bool useGetSystemTimes;
    
    PDH_HQUERY hQuery;
    PDH_HCOUNTER hCounter;
    bool usePDH;
    DWORD lastPdhCollectTime;
    
    DWORD majorVersion;
    DWORD minorVersion;
    
    // 平滑滤波相关
    double lastCPUValue;
    double lastMemValue;
    int stableCPUCount;
    int stableMemCount;
    
    // 辅助函数
    double SmoothValue(double newValue, double lastValue, double alpha = 0.3);

public:
    ResourceMonitor();
    ~ResourceMonitor();
    
    double GetCPUUsage();
    double GetMemoryUsage();
    MEMORYSTATUSEX GetMemoryInfo();
    SYSTEM_INFO GetSysInfo();

private:
    void InitCPU();
    double GetCPUUsageViaSystemTimes();
    double GetCPUUsageViaPDH();
    void CleanupPDH();
    void DetectWindowsVersion();
};