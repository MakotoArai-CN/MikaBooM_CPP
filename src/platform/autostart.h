#pragma once
#include <string>

class AutoStart {
public:
    static bool Enable();
    static bool Disable();
    static bool IsEnabled();

private:
    static std::string GetExePath();
    static const char* GetRegistryKey();
};