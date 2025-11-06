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
    
    // API函数指针
    typedef BOOL (WINAPI *PGetSystemTimes)(LPFILETIME, LPFILETIME, LPFILETIME);
    PGetSystemTimes pGetSystemTimes;
    bool useGetSystemTimes;
    
    // PDH相关
    PDH_HQUERY hQuery;
    PDH_HCOUNTER hCounter;
    bool usePDH;
    DWORD lastPdhCollectTime;
    
    // 系统版本
    DWORD majorVersion;
    DWORD minorVersion;
    
    // 平滑值
    double lastCPUValue;
    double lastMemValue;
    int stableCPUCount;
    int stableMemCount;
    
    // 平滑函数
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