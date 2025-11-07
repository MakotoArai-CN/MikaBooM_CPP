#pragma once
#include <windows.h>
#include <string>

class ConsoleUtils {
public:
    static void Init();
    static void Reinit();
    static void ShowWelcome();
    static void ShowVersion();
    static void ShowHelp();
    static void PrintInfo(const char* format, ...);
    static void PrintSuccess(const char* format, ...);
    static void PrintWarning(const char* format, ...);
    static void PrintError(const char* format, ...);
    static void PrintStatus(double cpu, double mem, bool cpuWork, bool memWork, 
                          int cpuIntensity, size_t memAllocMB);
    static bool IsWindows7OrLater();
    
private:
    static void SetColor(WORD color);
    static void ResetColor();
    static void PrintBanner();
    static void PrintSystemInfo();
    static void PrintVersionInfo();
    static void PrintHelpContent();
    static bool useUTF8;
    static bool initialized;
};