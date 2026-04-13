#pragma once
#include <windows.h>
#include <psapi.h>
#include <string>
#include "../platform/system_compat.h"

// PDH 类型定义（动态加载，不依赖 pdh.h）
typedef HANDLE PDH_HQUERY;
typedef HANDLE PDH_HCOUNTER;
typedef LONG   PDH_STATUS;

#ifndef PDH_FMT_DOUBLE
#define PDH_FMT_DOUBLE  ((DWORD)0x00000200)
#endif
#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS 0L
#endif

typedef struct _PDH_FMT_COUNTERVALUE {
    DWORD CStatus;
    union {
        LONG     longValue;
        double   doubleValue;
        LONGLONG largeValue;
        LPCSTR   AnsiStringValue;
        LPCWSTR  WideStringValue;
    };
} PDH_FMT_COUNTERVALUE;

// PDH 函数指针类型
typedef PDH_STATUS (WINAPI *PPdhOpenQueryA)(LPCSTR, DWORD_PTR, PDH_HQUERY*);
typedef PDH_STATUS (WINAPI *PPdhAddCounterA)(PDH_HQUERY, LPCSTR, DWORD_PTR, PDH_HCOUNTER*);
typedef PDH_STATUS (WINAPI *PPdhCollectQueryData)(PDH_HQUERY);
typedef PDH_STATUS (WINAPI *PPdhGetFormattedCounterValue)(PDH_HCOUNTER, DWORD, LPDWORD, PDH_FMT_COUNTERVALUE*);
typedef PDH_STATUS (WINAPI *PPdhRemoveCounter)(PDH_HCOUNTER);
typedef PDH_STATUS (WINAPI *PPdhCloseQuery)(PDH_HQUERY);

class ResourceMonitor {
private:
    ULARGE_INTEGER lastIdleTime;
    ULARGE_INTEGER lastKernelTime;
    ULARGE_INTEGER lastUserTime;
    int numProcessors;
    HANDLE self;

    // GetSystemTimes 函数指针
    typedef BOOL (WINAPI *PGetSystemTimes)(LPFILETIME, LPFILETIME, LPFILETIME);
    PGetSystemTimes pGetSystemTimes;
    bool useGetSystemTimes;

    // PDH 动态加载（运行时检测，不依赖 pdh.lib）
    HMODULE hPdhModule;
    PDH_HQUERY hQuery;
    PDH_HCOUNTER hCounter;
    bool usePDH;
    DWORD lastPdhCollectTime;
    PPdhOpenQueryA pPdhOpenQuery;
    PPdhAddCounterA pPdhAddCounter;
    PPdhCollectQueryData pPdhCollectQuery;
    PPdhGetFormattedCounterValue pPdhGetFormattedValue;
    PPdhRemoveCounter pPdhRemoveCounter;
    PPdhCloseQuery pPdhCloseQuery;

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
    MemoryStatusSnapshot GetMemoryInfo();
    SYSTEM_INFO GetSysInfo();

private:
    void InitCPU();
    bool LoadPDH();
    double GetCPUUsageViaSystemTimes();
    double GetCPUUsageViaPDH();
    void CleanupPDH();
    void DetectWindowsVersion();
};
