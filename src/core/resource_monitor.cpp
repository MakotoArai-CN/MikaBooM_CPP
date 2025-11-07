#include "resource_monitor.h"
#include "../utils/anti_detect.h"
#include <iostream>

ResourceMonitor::ResourceMonitor()
    : pGetSystemTimes(NULL), useGetSystemTimes(false),
#ifdef USE_PDH
      hQuery(NULL), hCounter(NULL), usePDH(false),
      lastPdhCollectTime(0),
#endif
      majorVersion(5), minorVersion(0),
      lastCPUValue(-1.0), lastMemValue(-1.0) {
    
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    numProcessors = sysInfo.dwNumberOfProcessors;
    
    self = GetCurrentProcess();
    
    DetectWindowsVersion();
    RandomDelay(50, 150);
    InitCPU();
}

ResourceMonitor::~ResourceMonitor() {
#ifdef USE_PDH
    CleanupPDH();
#endif
}

void ResourceMonitor::DetectWindowsVersion() {
    OSVERSIONINFOEXA osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
    
    typedef LONG (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOEXW);
    DynamicAPI ntdllLoader;
    
    const char encDll[] = {0x6f, 0x74, 0x67, 0x6e, 0x69, 0x2a, 0x63, 0x6a, 0x65};
    std::string dllName = StringCrypt::Decrypt(encDll, 9, 0x01);
    
    RtlGetVersionPtr pRtlGetVersion = ntdllLoader.GetFunction<RtlGetVersionPtr>(
        dllName.c_str(), "RtlGetVersion"
    );
    
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
    
    if (GetVersionExA((LPOSVERSIONINFOA)&osvi)) {
        majorVersion = osvi.dwMajorVersion;
        minorVersion = osvi.dwMinorVersion;
    }
}

double ResourceMonitor::SmoothValue(double newValue, double lastValue, double alpha) {
    if (lastValue < 0) return newValue;
    return alpha * newValue + (1.0 - alpha) * lastValue;
}

void ResourceMonitor::InitCPU() {
    DynamicAPI kernel32Loader;
    pGetSystemTimes = kernel32Loader.GetFunction<PGetSystemTimes>(
        "kernel32.dll", "GetSystemTimes"
    );
    
    if (pGetSystemTimes) {
        useGetSystemTimes = true;
        FILETIME idleTime, kernelTime, userTime;
        pGetSystemTimes(&idleTime, &kernelTime, &userTime);
        
        memcpy(&lastIdleTime, &idleTime, sizeof(FILETIME));
        memcpy(&lastKernelTime, &kernelTime, sizeof(FILETIME));
        memcpy(&lastUserTime, &userTime, sizeof(FILETIME));
        return;
    }
    
#ifdef USE_PDH
    // PDH 备用方案（仅 x86/x64）
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
    
    for (int i = 0; i < 3; i++) {
        PdhCollectQueryData(hQuery);
        Sleep(100);
    }
    
    usePDH = true;
    lastPdhCollectTime = GetTickCount();
#else
    // ARM/ARM64: 不使用 PDH
    useGetSystemTimes = false;
#endif
}

#ifdef USE_PDH
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
#endif

double ResourceMonitor::GetCPUUsage() {
    double rawValue;
    
    if (useGetSystemTimes) {
        rawValue = GetCPUUsageViaSystemTimes();
    }
#ifdef USE_PDH
    else {
        rawValue = GetCPUUsageViaPDH();
    }
#else
    else {
        // ARM/ARM64 降级：返回上次值或 0
        rawValue = lastCPUValue > 0 ? lastCPUValue : 0.0;
    }
#endif
    
    double alpha = (majorVersion >= 6) ? 0.3 : 0.15;
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
    ULONGLONG systemDiff = kernelDiff + userDiff;
    
    double cpuUsage = 0.0;
    if (systemDiff > 0) {
        cpuUsage = (double)(systemDiff - idleDiff) / systemDiff * 100.0;
    }
    
    lastIdleTime = idle;
    lastKernelTime = kernel;
    lastUserTime = user;
    
    if (cpuUsage < 0.0) cpuUsage = 0.0;
    if (cpuUsage > 100.0) cpuUsage = 100.0;
    
    return cpuUsage;
}

#ifdef USE_PDH
double ResourceMonitor::GetCPUUsageViaPDH() {
    if (!usePDH || !hQuery || !hCounter) {
        return lastCPUValue > 0 ? lastCPUValue : 0.0;
    }
    
    DWORD currentTime = GetTickCount();
    DWORD elapsed = currentTime - lastPdhCollectTime;
    
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
#endif

double ResourceMonitor::GetMemoryUsage() {
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    
    if (GlobalMemoryStatusEx(&memInfo)) {
        DWORDLONG totalPhysMem = memInfo.ullTotalPhys;
        DWORDLONG physMemUsed = totalPhysMem - memInfo.ullAvailPhys;
        
        double rawValue = (double)physMemUsed / totalPhysMem * 100.0;
        
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