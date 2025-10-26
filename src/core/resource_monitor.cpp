#include "resource_monitor.h"
#include <iostream>

ResourceMonitor::ResourceMonitor()
    : pGetSystemTimes(NULL), useGetSystemTimes(false), 
      hQuery(NULL), hCounter(NULL), usePDH(false),
      lastPdhCollectTime(0), majorVersion(5), minorVersion(0),
      lastCPUValue(-1.0), lastMemValue(-1.0),
      stableCPUCount(0), stableMemCount(0) {
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;
    self = GetCurrentProcess();
    
    DetectWindowsVersion();
    InitCPU();
}

ResourceMonitor::~ResourceMonitor() {
    CleanupPDH();
}

void ResourceMonitor::DetectWindowsVersion() {
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    
    typedef LONG (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (hNtdll) {
        RtlGetVersionPtr pRtlGetVersion = (RtlGetVersionPtr)GetProcAddress(hNtdll, "RtlGetVersion");
        if (pRtlGetVersion) {
            OSVERSIONINFOEXW osviW;
            ZeroMemory(&osviW, sizeof(osviW));
            osviW.dwOSVersionInfoSize = sizeof(osviW);
            if (pRtlGetVersion((PRTL_OSVERSIONINFOEXW)&osviW) == 0) {
                majorVersion = osviW.dwMajorVersion;
                minorVersion = osviW.dwMinorVersion;
                return;
            }
        }
    }
    
    if (GetVersionExA((LPOSVERSIONINFOA)&osvi)) {
        majorVersion = osvi.dwMajorVersion;
        minorVersion = osvi.dwMinorVersion;
    }
}

double ResourceMonitor::SmoothValue(double newValue, double lastValue, double alpha) {
    if (lastValue < 0) return newValue;  // 首次调用
    return alpha * newValue + (1.0 - alpha) * lastValue;
}

void ResourceMonitor::InitCPU() {
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (hKernel32) {
        pGetSystemTimes = (PGetSystemTimes)GetProcAddress(hKernel32, "GetSystemTimes");
        if (pGetSystemTimes) {
            useGetSystemTimes = true;
            FILETIME idleTime, kernelTime, userTime;
            pGetSystemTimes(&idleTime, &kernelTime, &userTime);
            memcpy(&lastIdleTime, &idleTime, sizeof(FILETIME));
            memcpy(&lastKernelTime, &kernelTime, sizeof(FILETIME));
            memcpy(&lastUserTime, &userTime, sizeof(FILETIME));
            return;
        }
    }
    
    // PDH初始化 - 添加预热
    useGetSystemTimes = false;
    usePDH = false;
    
    PDH_STATUS status = PdhOpenQueryA(NULL, 0, &hQuery);
    if (status != ERROR_SUCCESS) {
        return;
    }
    
    status = PdhAddCounterA(hQuery, "\\Processor(_Total)\\% Processor Time", 0, &hCounter);
    if (status != ERROR_SUCCESS) {
        PdhCloseQuery(hQuery);
        hQuery = NULL;
        return;
    }
    
    // 预热：连续采样3次，让PDH稳定
    for (int i = 0; i < 3; i++) {
        PdhCollectQueryData(hQuery);
        Sleep(100);
    }
    
    usePDH = true;
    lastPdhCollectTime = GetTickCount();
}

void ResourceMonitor::CleanupPDH() {
    if (hCounter) {
        PdhRemoveCounter(hCounter);
        hCounter = NULL;
    }
    if (hQuery) {
        PdhCloseQuery(hQuery);
        hQuery = NULL;
    }
}

double ResourceMonitor::GetCPUUsage() {
    double rawValue;
    if (useGetSystemTimes) {
        rawValue = GetCPUUsageViaSystemTimes();
    } else {
        rawValue = GetCPUUsageViaPDH();
    }
    
    // 应用平滑滤波（老系统使用更强的平滑）
    double alpha = (majorVersion >= 6) ? 0.3 : 0.15;  // Win Vista+用0.3，老系统用0.15
    rawValue = SmoothValue(rawValue, lastCPUValue, alpha);
    lastCPUValue = rawValue;
    
    return rawValue;
}

double ResourceMonitor::GetCPUUsageViaSystemTimes() {
    FILETIME idleTime, kernelTime, userTime;
    ULARGE_INTEGER idle, kernel, user;
    
    pGetSystemTimes(&idleTime, &kernelTime, &userTime);
    
    memcpy(&idle, &idleTime, sizeof(FILETIME));
    memcpy(&kernel, &kernelTime, sizeof(FILETIME));
    memcpy(&user, &userTime, sizeof(FILETIME));
    
    ULONGLONG idleDiff = idle.QuadPart - lastIdleTime.QuadPart;
    ULONGLONG kernelDiff = kernel.QuadPart - lastKernelTime.QuadPart;
    ULONGLONG userDiff = user.QuadPart - lastUserTime.QuadPart;
    ULONGLONG totalDiff = kernelDiff + userDiff;
    
    double cpuUsage = 0.0;
    if (totalDiff > 0) {
        cpuUsage = (double)(totalDiff - idleDiff) / totalDiff * 100.0;
    }
    
    lastIdleTime = idle;
    lastKernelTime = kernel;
    lastUserTime = user;
    
    if (cpuUsage < 0.0) cpuUsage = 0.0;
    if (cpuUsage > 100.0) cpuUsage = 100.0;
    
    return cpuUsage;
}

double ResourceMonitor::GetCPUUsageViaPDH() {
    if (!usePDH || !hQuery || !hCounter) {
        return lastCPUValue > 0 ? lastCPUValue : 0.0;
    }
    
    DWORD currentTime = GetTickCount();
    DWORD elapsed = currentTime - lastPdhCollectTime;
    
    // 老系统降低采样频率要求：500ms即可（原来是1000ms）
    if (elapsed < 500) {
        return lastCPUValue > 0 ? lastCPUValue : 0.0;
    }
    
    PDH_STATUS status = PdhCollectQueryData(hQuery);
    if (status != ERROR_SUCCESS) {
        return lastCPUValue > 0 ? lastCPUValue : 0.0;
    }
    
    lastPdhCollectTime = currentTime;
    
    PDH_FMT_COUNTERVALUE counterValue;
    status = PdhGetFormattedCounterValue(hCounter, PDH_FMT_DOUBLE, NULL, &counterValue);
    if (status != ERROR_SUCCESS) {
        return lastCPUValue > 0 ? lastCPUValue : 0.0;
    }
    
    double cpuUsage = counterValue.doubleValue;
    if (cpuUsage < 0.0) cpuUsage = 0.0;
    if (cpuUsage > 100.0) cpuUsage = 100.0;
    
    return cpuUsage;
}

double ResourceMonitor::GetMemoryUsage() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (GlobalMemoryStatusEx(&memInfo)) {
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
        DWORDLONG physMemUsed = totalPhysMem - memInfo.ullAvailPhys;
        double rawValue = (double)physMemUsed / totalPhysMem * 100.0;
        
        // 老系统内存统计也需要平滑
        double alpha = (majorVersion >= 6) ? 0.3 : 0.15;
        rawValue = SmoothValue(rawValue, lastMemValue, alpha);
        lastMemValue = rawValue;
        
        return rawValue;
    }
    
    return lastMemValue > 0 ? lastMemValue : 0.0;
}

MEMORYSTATUSEX ResourceMonitor::GetMemoryInfo() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    return memInfo;
}

SYSTEM_INFO ResourceMonitor::GetSysInfo() {
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    return sysInfo;
}