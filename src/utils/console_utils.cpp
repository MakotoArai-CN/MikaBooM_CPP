#include "console_utils.h"
#include "version.h"
#include "system_info.h"
#include "compat_string.h"
#include "../platform/system_compat.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>

static HANDLE hConsole = NULL;
static WORD originalAttributes = 7;
static bool isInitialized = false;
bool ConsoleUtils::useUTF8 = false;

namespace {
std::string GetExecutableNameForDisplay() {
    char path[MAX_PATH] = {0};
    DWORD length = GetModuleFileNameA(NULL, path, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return "MikaBooM.exe";
    }

    const char* fileName = path;
    for (const char* p = path; *p; ++p) {
        if (*p == '\\' || *p == '/') {
            fileName = p + 1;
        }
    }

    return *fileName ? std::string(fileName) : std::string("MikaBooM.exe");
}
}

void ConsoleUtils::Init() {
    if (isInitialized) return;

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
        originalAttributes = consoleInfo.wAttributes;
    }

    useUTF8 = IsWindows7OrLater();

    if (useUTF8) {
        SetConsoleCP(65001);
        SetConsoleOutputCP(65001);
    } else {
        SetConsoleCP(437);
        SetConsoleOutputCP(437);
    }

    Version::InitAsciiArt(useUTF8);

    isInitialized = true;
}

void ConsoleUtils::Reinit() {
    isInitialized = false;
    hConsole = NULL;

    Sleep(100);

    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
        originalAttributes = consoleInfo.wAttributes;
    } else {
        originalAttributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    useUTF8 = IsWindows7OrLater();

    if (useUTF8) {
        SetConsoleCP(65001);
        SetConsoleOutputCP(65001);
    } else {
        SetConsoleCP(437);
        SetConsoleOutputCP(437);
    }

    Version::InitAsciiArt(useUTF8);

    isInitialized = true;

    COORD coordScreen = { 0, 0 };
    DWORD cCharsWritten;
    DWORD dwConSize;

    if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
        dwConSize = consoleInfo.dwSize.X * consoleInfo.dwSize.Y;
        FillConsoleOutputCharacter(hConsole, (TCHAR)' ', dwConSize, coordScreen, &cCharsWritten);
        FillConsoleOutputAttribute(hConsole, consoleInfo.wAttributes, dwConSize, coordScreen, &cCharsWritten);
        SetConsoleCursorPosition(hConsole, coordScreen);
    }
}

bool ConsoleUtils::IsWindows7OrLater() {
    return SystemInfo::IsWindows7OrLater();
}

void ConsoleUtils::SetColor(WORD color) {
    if (hConsole) {
        SetConsoleTextAttribute(hConsole, color);
    }
}

void ConsoleUtils::ResetColor() {
    if (hConsole) {
        SetConsoleTextAttribute(hConsole, originalAttributes);
    }
}

void ConsoleUtils::PrintBanner() {
    if (!isInitialized) Init();

    SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("============================================\n");
    printf("||           M I K A B O O M               ||\n");
    printf("||       Resource Monitor System          ||\n");
    printf("============================================\n");
    ResetColor();
    printf("\n");

    SetColor(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    std::string art = Version::GetRandomAsciiArt();
    printf("%s\n\n", art.c_str());
    ResetColor();

    SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("Resource Monitor - Miku Edition\n");
    printf("=======================================\n");
    ResetColor();
    printf("\n");
}

void ConsoleUtils::PrintSystemInfo() {
    if (!isInitialized) Init();

    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);

    MemoryStatusSnapshot memInfo;
    const bool hasMemoryInfo = SystemCompat::QueryMemoryStatus(memInfo);

    SetColor(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);

    std::string expireDate = Version::GetExpireDate();
    size_t pos = expireDate.find('T');
    if (pos != std::string::npos) {
        expireDate[pos] = ' ';
    }

    if (useUTF8) {
        printf(">> 操作系统: %s\n", SystemInfo::GetOSName());
        printf(">> CPU核心数: %lu\n", sysInfo.dwNumberOfProcessors);
        printf(">> 物理内存: %.2f GB\n", hasMemoryInfo ? memInfo.totalPhys / (1024.0 * 1024.0 * 1024.0) : 0.0);
        printf("\n");
        printf(">> 版本: %s\n", Version::GetVersion());
        printf(">> 构建时间: %s\n", Version::GetBuildDate());
        printf(">> 升级时间: %s\n", expireDate.c_str());
        printf(">> 作者: %s\n", Version::GetAuthor());
    } else {
        printf("OS: %s\n", SystemInfo::GetOSName());
        printf("CPU Cores: %lu\n", sysInfo.dwNumberOfProcessors);
        printf("Physical Memory: %.2f GB\n", hasMemoryInfo ? memInfo.totalPhys / (1024.0 * 1024.0 * 1024.0) : 0.0);
        printf("\n");
        printf("Version: %s\n", Version::GetVersion());
        printf("Build Date: %s\n", Version::GetBuildDate());
        printf("Expire Date: %s\n", expireDate.c_str());
        printf("Author: %s\n", Version::GetAuthor());
    }

    ResetColor();
    printf("\n");

    if (!Version::IsValid()) {
        SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
        if (useUTF8) {
            printf("=======================================\n");
            printf("!! 版本需更新\n");
            printf("!! 不支持的版本，请更新应用程序\n");
            printf("!! 当前仅支持监控功能，计算功能文件缺失\n");
            printf("=======================================\n");
        } else {
            printf("=======================================\n");
            printf("WARNING: Version needs update\n");
            printf("Please update the application\n");
            printf("Only monitoring mode available\n");
            printf("=======================================\n");
        }
        ResetColor();
    } else {
        int daysLeft = Version::GetDaysUntilExpire();
        if (daysLeft <= 30) {
            SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
            if (useUTF8) {
                printf("=======================================\n");
                printf("!! 版本将在 %d 天后更新\n", daysLeft);
                printf("=======================================\n");
            } else {
                printf("=======================================\n");
                printf("WARNING: Version will expire in %d days\n", daysLeft);
                printf("=======================================\n");
            }
            ResetColor();
        } else if (daysLeft <= 90) {
            SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
            if (useUTF8) {
                printf(">> 版本更新期剩余: %d 天\n", daysLeft);
            } else {
                printf("Version valid for %d days\n", daysLeft);
            }
            ResetColor();
        } else {
            SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            if (useUTF8) {
                printf(">> 版本更新 (剩余 %d 天)\n", daysLeft);
            } else {
                printf("Version valid (%d days remaining)\n", daysLeft);
            }
            ResetColor();
        }
    }

    printf("\n");
}

void ConsoleUtils::ShowWelcome() {
    Init();
    PrintBanner();
    PrintSystemInfo();
}

void ConsoleUtils::ShowVersion() {
    Init();
    PrintBanner();
    PrintSystemInfo();
}

void ConsoleUtils::PrintHelpContent() {
    if (!isInitialized) Init();

    SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    if (useUTF8) {
        printf(">> 用法:\n");
    } else {
        printf("USAGE:\n");
    }
    ResetColor();
    const std::string exeName = GetExecutableNameForDisplay();
    printf("  %s [options] <value>\n\n", exeName.c_str());

    SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    if (useUTF8) {
        printf(">> 选项:\n");
    } else {
        printf("OPTIONS:\n");
    }
    ResetColor();

    if (useUTF8) {
        printf("  -cpu <value>                设置CPU占用率阈值 (0-100)\n");
        printf("  -mem <value>                设置内存占用率阈值 (0-100)\n");
        printf("  -mem-min <MB>               设置内存随机占用最小值\n");
        printf("  -mem-max <MB>               设置内存随机占用最大值\n");
        printf("  -mem-freq-min <sec>         设置随机目标变化最短周期\n");
        printf("  -mem-freq-max <sec>         设置随机目标变化最长周期\n");
        printf("  -mem-refresh <true|false>   设置是否启用页面刷新\n");
        printf("  -mem-refresh-after <sec>    设置分配多久后开始刷新页面\n");
        printf("  -mem-refresh-interval <sec> 设置刷新周期\n");
        printf("  -mem-refresh-stride <KB>    设置刷新步长\n\n");
        printf("  -window <value>             设置窗口显示模式\n");
        printf("  -auto                       启用开机自启动\n");
        printf("  -noauto                     禁用开机自启动\n");
        printf("  -update                     检测并安装更新（一键完成）\n");
        printf("  -c <file>                   指定配置文件路径\n");
        printf("  -v                          显示版本信息\n");
        printf("  -h                          显示此帮助信息\n\n");

        SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf(">> 示例:\n");
        ResetColor();
        printf("  %s -mem-min 256 -mem-max 512\n", exeName.c_str());
        printf("  %s -mem-freq-min 30 -mem-freq-max 60\n", exeName.c_str());
        printf("  %s -mem-refresh true -mem-refresh-interval 30\n\n", exeName.c_str());
    } else {
        printf("  -cpu <value>                Set CPU threshold (0-100)\n");
        printf("  -mem <value>                Set memory threshold (0-100)\n");
        printf("  -mem-min <MB>               Set minimum random memory target\n");
        printf("  -mem-max <MB>               Set maximum random memory target\n");
        printf("  -mem-freq-min <sec>         Set minimum random interval\n");
        printf("  -mem-freq-max <sec>         Set maximum random interval\n");
        printf("  -mem-refresh <true|false>   Enable or disable page refresh\n");
        printf("  -mem-refresh-after <sec>    Delay before refresh starts\n");
        printf("  -mem-refresh-interval <sec> Set refresh interval\n");
        printf("  -mem-refresh-stride <KB>    Set refresh stride\n");
        printf("  -window <value>             Set window mode\n");
        printf("  -auto                       Enable auto-start\n");
        printf("  -noauto                     Disable auto-start\n");
        printf("  -update                     Check and install updates\n");
        printf("  -c <file>                   Specify config file\n");
        printf("  -v                          Show version\n");
        printf("  -h                          Show help\n\n");
    }

    SetColor(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("%s Makoto\n", useUTF8 ? ">> 作者:" : "Author:");
    SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("%s MikaBooM - Resource Monitor\n", useUTF8 ? ">> 项目:" : "Project:");
    ResetColor();
    printf("\n");
}

void ConsoleUtils::ShowHelp() {
    Init();
    PrintBanner();
    PrintHelpContent();
}

void ConsoleUtils::PrintInfo(const char* format, ...) {
    SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("[INFO] ");
    ResetColor();
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void ConsoleUtils::PrintSuccess(const char* format, ...) {
    SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    printf("[OK] ");
    ResetColor();
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void ConsoleUtils::PrintWarning(const char* format, ...) {
    SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("[WARN] ");
    ResetColor();
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void ConsoleUtils::PrintError(const char* format, ...) {
    SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    printf("[ERROR] ");
    ResetColor();
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
}

void ConsoleUtils::PrintStatus(double cpu, double mem, bool cpuWork, bool memWork,
                               int cpuIntensity, size_t memAllocMB, size_t memResidentMB,
                               size_t memTargetMB, int residentRatio, bool refreshEnabled,
                               bool residentApproximate) {
    time_t now = time(0);
    struct tm timeinfo;
    if (localtime_s(&timeinfo, &now) != 0) {
        memset(&timeinfo, 0, sizeof(timeinfo));
    }

    printf("[%02d:%02d:%02d] ", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    if (cpu > 80) {
        SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    } else if (cpu > 60) {
        SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    } else {
        SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
    printf("CPU: %5.1f%% ", cpu);
    ResetColor();

    if (mem > 80) {
        SetColor(FOREGROUND_RED | FOREGROUND_INTENSITY);
    } else if (mem > 60) {
        SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    } else {
        SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
    printf("MEM: %5.1f%%", mem);
    ResetColor();

    if (cpuWork) {
        SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        if (useUTF8) {
            printf(" [CPU计算: 运行中 (强度:%d%%)]", cpuIntensity);
        } else {
            printf(" [CPU-W: ON, I:%d%%]", cpuIntensity);
        }
    } else {
        SetColor(FOREGROUND_INTENSITY);
        if (useUTF8) {
            printf(" [CPU计算: 已停止]");
        } else {
            printf(" [CPU-W: OFF]");
        }
    }
    ResetColor();

    if (memWork) {
        SetColor(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        if (useUTF8) {
            printf(" [内存计算: 运行中 (目标:%luMB 已分配:%luMB 驻留:%luMB 比例:%d%% 刷新:%s%s)]",
                   (unsigned long)memTargetMB,
                   (unsigned long)memAllocMB,
                   (unsigned long)memResidentMB,
                   residentRatio,
                   refreshEnabled ? "开" : "关",
                   residentApproximate ? ",估算" : "");
        } else {
            printf(" [MEM-W: ON, T:%luMB A:%luMB R:%luMB RR:%d%% %s%s]",
                   (unsigned long)memTargetMB,
                   (unsigned long)memAllocMB,
                   (unsigned long)memResidentMB,
                   residentRatio,
                   refreshEnabled ? "REF:ON" : "REF:OFF",
                   residentApproximate ? ",APPROX" : "");
        }
    } else {
        SetColor(FOREGROUND_INTENSITY);
        if (useUTF8) {
            printf(" [内存计算: 已停止]");
        } else {
            printf(" [MEM-W: OFF]");
        }
    }
    ResetColor();
    printf("\n");
}
