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