#pragma once
#include <windows.h>
#include <string>

// 字符串加密/解密工具
class StringCrypt {
public:
    static std::string Decrypt(const char* encrypted, size_t len, unsigned char key) {
        std::string result;
        result.reserve(len);
        for (size_t i = 0; i < len; i++) {
            result += (char)(encrypted[i] ^ key ^ (i & 0xFF));
        }
        return result;
    }
    
    static void Encrypt(const char* input, char* output, size_t len, unsigned char key) {
        for (size_t i = 0; i < len; i++) {
            output[i] = input[i] ^ key ^ (i & 0xFF);
        }
    }
};

// API动态加载器
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

// 延迟执行
inline void RandomDelay(int minMs = 10, int maxMs = 100) {
    int delay = minMs + (rand() % (maxMs - minMs + 1));
    Sleep(delay);
}

// 内存清理（防止内存扫描）
inline void SecureZeroMem(void* ptr, size_t size) {
    volatile unsigned char* p = (volatile unsigned char*)ptr;
    while (size--) {
        *p++ = 0;
    }
}

// 字符串混淆宏 - 编译时加密字符串
#define OBFUSCATE_STR(str) StringCrypt::Decrypt(str, sizeof(str) - 1, 0x01)

// 分散内存访问模式（反沙箱检测）
inline void AntiSandboxMemoryPattern() {
    volatile char buffer[1024];
    for (int i = 0; i < 1024; i += (rand() % 64 + 1)) {
        buffer[i] = (char)(rand() % 256);
    }
    SecureZeroMem((void*)buffer, 1024);
}

// 时间检测（反调试）
inline bool IsDebuggerPresent_Safe() {
    DWORD startTime = GetTickCount();
    RandomDelay(10, 50);
    DWORD endTime = GetTickCount();
    
    // 如果时间差异过大，可能被调试
    if (endTime - startTime > 1000) {
        return true;
    }
    
    // 使用动态加载避免直接调用
    DynamicAPI kernel32;
    typedef BOOL (WINAPI *PIsDebuggerPresent)();
    PIsDebuggerPresent pIsDebuggerPresent = kernel32.GetFunction<PIsDebuggerPresent>(
        "kernel32.dll", "IsDebuggerPresent"
    );
    
    if (pIsDebuggerPresent) {
        return pIsDebuggerPresent() == TRUE;
    }
    
    return false;
}

// 环境检测（反虚拟机）
inline bool IsVirtualEnvironment() {
    // 检查 CPU 核心数（虚拟机通常核心少）
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    if (sysInfo.dwNumberOfProcessors < 2) {
        return true;  // 可疑
    }
    
    // 检查内存大小（虚拟机通常内存小）
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    GlobalMemoryStatusEx(&memInfo);
    if (memInfo.ullTotalPhys < (2ULL * 1024 * 1024 * 1024)) {  // < 2GB
        return true;  // 可疑
    }
    
    return false;
}