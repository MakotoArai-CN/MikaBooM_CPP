#pragma once
#include <windows.h>
#include <psapi.h>

// 条件编译：通过 Makefile 传递 USE_PDH 宏
#ifdef USE_PDH
    // x86/x64: 使用 PDH
    #include <pdh.h>
#endif

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
    
#ifdef USE_PDH
    // PDH相关（仅 x86/x64）
    PDH_HQUERY hQuery;
    PDH_HCOUNTER hCounter;
    bool usePDH;
    DWORD lastPdhCollectTime;
#endif
    
    // 系统版本
    DWORD majorVersion;
    DWORD minorVersion;
    
    // 平滑值
    double lastCPUValue;
    double lastMemValue;
    
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
#ifdef USE_PDH
    double GetCPUUsageViaPDH();
    void CleanupPDH();
#endif
    void DetectWindowsVersion();
};