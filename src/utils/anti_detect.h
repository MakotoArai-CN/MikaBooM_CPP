#pragma once
#include <windows.h>
#include <string>

// 字符串加密工具（XOR + 滚动密钥）
class StringCrypt {
public:
    static std::string Decrypt(const char* encrypted, size_t len, unsigned char key) {
        std::string result;
        result.reserve(len);
        unsigned char rolling_key = key;
        for (size_t i = 0; i < len; i++) {
            result += (char)(encrypted[i] ^ rolling_key);
            rolling_key = (rolling_key * 7 + 13) & 0xFF;  // 滚动密钥
        }
        return result;
    }
    
    static void Encrypt(const char* input, char* output, size_t len, unsigned char key) {
        unsigned char rolling_key = key;
        for (size_t i = 0; i < len; i++) {
            output[i] = input[i] ^ rolling_key;
            rolling_key = (rolling_key * 7 + 13) & 0xFF;
        }
    }
};

// API动态加载器（增加混淆）
class DynamicAPI {
private:
    HMODULE hModule;
    
public:
    DynamicAPI() : hModule(NULL) {}
    ~DynamicAPI() {
        if (hModule) {
            FreeLibrary(hModule);
        }
    }
    
    template<typename T>
    T GetFunction(const char* dllName, const char* funcName) {
        if (!hModule) {
            hModule = LoadLibraryA(dllName);
            if (!hModule) return NULL;
        }
        return (T)GetProcAddress(hModule, funcName);
    }
};

// 随机延迟（增加抖动）
inline void RandomDelay(int minMs = 10, int maxMs = 100) {
    int jitter = (rand() % 20) - 10;  // ±10ms 抖动
    int delay = minMs + (rand() % (maxMs - minMs + 1)) + jitter;
    if (delay < 0) delay = 0;
    Sleep(delay);
}

// 安全内存清理（防止优化消除）
inline void SecureZeroMem(void* ptr, size_t size) {
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    while (size--) {
        *p++ = 0;
    }
}

// 混淆字符串宏
#define OBFUSCATE_STR(str) StringCrypt::Decrypt(str, sizeof(str) - 1, 0x47)

// 正常行为模拟（伪装成系统检测）
inline void SystemHealthCheck() {
    // 模拟正常的系统健康检查
    volatile char buffer[512];
    for (int i = 0; i < 512; i += (rand() % 32 + 1)) {
        buffer[i] = (char)(GetTickCount() % 256);
    }
    SecureZeroMem((void*)buffer, 512);
}

// 时间差异检测（反调试）- 改名混淆
inline bool PerformanceCheck() {
    DWORD t1 = GetTickCount();
    
    // 执行一些正常操作
    volatile int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += i;
    }
    
    DWORD t2 = GetTickCount();
    
    // 如果时间差异过大（被调试/沙箱减速）
    if (t2 - t1 > 500) {
        return false;
    }
    
    // 动态加载检测函数（避免直接调用）
    DynamicAPI kernel32;
    typedef BOOL (WINAPI *PIsDebuggerPresent)();
    PIsDebuggerPresent pFunc = kernel32.GetFunction<PIsDebuggerPresent>(
        "kernel32.dll", "IsDebuggerPresent"
    );
    
    if (pFunc && pFunc()) {
        return false;
    }
    
    return true;
}

// 系统资源检查（反虚拟机）- 改名混淆
inline bool ValidateSystemResources() {
    // 检查 CPU 核心数
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    
    // 真实物理机通常 >= 2 核心
    bool has_sufficient_cores = sysInfo.dwNumberOfProcessors >= 2;
    
    // 检查物理内存
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);
    
    // 真实物理机通常 >= 2GB
    bool has_sufficient_memory = memInfo.ullTotalPhys >= (2ULL * 1024 * 1024 * 1024);
    
    // 检查磁盘（虚拟机通常磁盘较小）
    ULARGE_INTEGER freeBytesAvailable, totalNumberOfBytes;
    if (GetDiskFreeSpaceExA("C:\\", &freeBytesAvailable, &totalNumberOfBytes, NULL)) {
        // 真实物理机通常 >= 50GB
        bool has_sufficient_disk = totalNumberOfBytes.QuadPart >= (50ULL * 1024 * 1024 * 1024);
        
        if (!has_sufficient_cores || !has_sufficient_memory || !has_sufficient_disk) {
            return false;
        }
    }
    
    return true;
}

// 混淆的反调试检测（伪装成配置初始化）
inline bool InitializeConfiguration() {
    // 执行正常的系统健康检查
    SystemHealthCheck();
    
    // 时间检测
    if (!PerformanceCheck()) {
        return false;
    }
    
    // 资源检测
    if (!ValidateSystemResources()) {
        return false;
    }
    
    return true;
}