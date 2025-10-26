#include "console_utils.h"
#include "version.h"
#include "system_info.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <cstring>

static HANDLE hConsole = NULL;
static WORD originalAttributes = 7;
static bool isInitialized = false;
bool ConsoleUtils::useUTF8 = false;

void ConsoleUtils::Init() {
    if (isInitialized) return; // 防止重复初始化
    
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    if (GetConsoleScreenBufferInfo(hConsole, &consoleInfo)) {
        originalAttributes = consoleInfo.wAttributes;
    }
    
    // 检测系统版本
    useUTF8 = IsWindows7OrLater();
    
    if (useUTF8) {
        // Windows 7+ 使用UTF-8以支持盲文字符和中文
        SetConsoleCP(65001);
        SetConsoleOutputCP(65001);
    } else {
        // Windows XP及以下使用代码页437以支持扩展ASCII
        SetConsoleCP(437);
        SetConsoleOutputCP(437);
    }
    
    // 初始化版本模块的字符画选择
    Version::InitAsciiArt(useUTF8);
    
    isInitialized = true;
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
    // 确保已初始化
    if (!isInitialized) Init();
    
    SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    printf("============================================\n");
    printf("||           M I K A B O O M            ||\n");
    printf("||       Resource Monitor System        ||\n");
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
    // 确保已初始化
    if (!isInitialized) Init();
    
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    SetColor(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
    
    // 获取过期日期并转换显示格式
    std::string expireDate = Version::GetExpireDate();
    // 将 T 替换为空格以便显示
    size_t pos = expireDate.find('T');
    if (pos != std::string::npos) {
        expireDate[pos] = ' ';
    }
    
    if (useUTF8) {
        printf(">> 操作系统: %s\n", SystemInfo::GetOSName());
        printf("\n");
        printf(">> 版本: %s\n", Version::GetVersion());
        printf(">> 构建时间: %s\n", Version::GetBuildDate());
        printf(">> 升级时间: %s\n", expireDate.c_str());
        printf(">> 作者: %s\n", Version::GetAuthor());
    } else {
        printf("OS: %s\n", SystemInfo::GetOSName());
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
    Init(); // 确保初始化
    PrintBanner();
    PrintSystemInfo();
}

void ConsoleUtils::ShowVersion() {
    Init(); // 确保初始化
    PrintBanner();
    PrintSystemInfo();
}

void ConsoleUtils::PrintHelpContent() {
    // 确保已初始化
    if (!isInitialized) Init();
    
    SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    if (useUTF8) {
        printf(">> 用法:\n");
    } else {
        printf("USAGE:\n");
    }
    ResetColor();
    printf("  MikaBooM.exe [options] <value>\n\n");
    
    SetColor(FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY);
    if (useUTF8) {
        printf(">> 选项:\n");
    } else {
        printf("OPTIONS:\n");
    }
    ResetColor();
    
    if (useUTF8) {
        printf("  -cpu <value>        设置CPU占用率阈值 (0-100)\n");
        printf("                      示例: -cpu 80\n\n");
        printf("  -mem <value>        设置内存占用率阈值 (0-100)\n");
        printf("                      示例: -mem 70\n\n");
        printf("  -window <value>     设置窗口显示模式\n");
        printf("                      true/1/yes/on  - 显示窗口（前台运行）\n");
        printf("                      false/0/no/off - 隐藏窗口（后台运行）\n");
        printf("                      示例: -window false\n\n");
        printf("  -auto               启用开机自启动\n\n");
        printf("  -noauto             禁用开机自启动\n\n");
        printf("  -c <file>           指定配置文件路径\n");
        printf("                      支持绝对路径和相对路径\n");
        printf("                      示例: -c C:\\config.ini\n\n");
        printf("  -v                  显示版本信息\n\n");
        printf("  -h                  显示此帮助信息\n\n");
        
        SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf(">> 示例:\n");
        ResetColor();
        printf("  # 首次运行（自动创建配置文件）\n");
        printf("  MikaBooM.exe\n\n");
        printf("  # 使用默认配置文件，设置CPU阈值80%%\n");
        printf("  MikaBooM.exe -cpu 80\n\n");
        printf("  # 后台运行，不显示窗口\n");
        printf("  MikaBooM.exe -window false\n\n");
        printf("  # 启用开机自启动\n");
        printf("  MikaBooM.exe -auto\n\n");
        printf("  # 禁用开机自启动\n");
        printf("  MikaBooM.exe -noauto\n\n");
        printf("  # 使用自定义配置文件\n");
        printf("  MikaBooM.exe -c C:\\custom-config.ini\n\n");
        printf("  # 查看版本信息\n");
        printf("  MikaBooM.exe -v\n\n");
        
        SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf(">> 配置文件:\n");
        ResetColor();
        printf("  默认位置: 可执行文件同级目录下的 config.ini\n\n");
        printf("  支持的配置项:\n");
        printf("    - cpu_threshold      CPU阈值 (0-100)\n");
        printf("    - memory_threshold   内存阈值 (0-100)\n");
        printf("    - show_window        是否显示窗口 (true/false)\n");
        printf("    - auto_start         是否自启动 (true/false)\n");
        printf("    - update_interval    更新间隔（秒）\n");
        printf("    - notification       通知设置\n\n");
        
        SetColor(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf(">> 作者: Makoto\n");
        SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf(">> 项目: MikaBooM - Resource Monitor Miku Edition\n");
    } else {
        printf("  -cpu <value>        Set CPU threshold (0-100)\n");
        printf("                      Example: -cpu 80\n\n");
        printf("  -mem <value>        Set Memory threshold (0-100)\n");
        printf("                      Example: -mem 70\n\n");
        printf("  -window <value>     Set window display mode\n");
        printf("                      true/1/yes/on  - Show window\n");
        printf("                      false/0/no/off - Hide window\n");
        printf("                      Example: -window false\n\n");
        printf("  -auto               Enable auto-start\n\n");
        printf("  -noauto             Disable auto-start\n\n");
        printf("  -c <file>           Specify config file path\n");
        printf("                      Example: -c C:\\config.ini\n\n");
        printf("  -v                  Show version\n\n");
        printf("  -h                  Show this help\n\n");
        
        SetColor(FOREGROUND_GREEN | FOREGROUND_INTENSITY);
        printf("EXAMPLES:\n");
        ResetColor();
        printf("  MikaBooM.exe\n");
        printf("  MikaBooM.exe -cpu 80\n");
        printf("  MikaBooM.exe -window false\n");
        printf("  MikaBooM.exe -auto\n\n");
        
        SetColor(FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf("Author: Makoto\n");
        SetColor(FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY);
        printf("Project: MikaBooM - Resource Monitor Miku Edition\n");
    }
    ResetColor();
    printf("\n");
}

void ConsoleUtils::ShowHelp() {
    Init(); // 确保初始化
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

void ConsoleUtils::PrintStatus(double cpu, double mem, bool cpuWork, bool memWork, int cpuIntensity, size_t memAllocMB) {
    time_t now = time(0);
    struct tm timeinfo;
    struct tm* tmp = localtime(&now);
    if (tmp) {
        timeinfo = *tmp;
    } else {
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
            printf(" [内存计算: 运行中 (已分配:%luMB)]", (unsigned long)memAllocMB);
        } else {
            printf(" [MEM-W: ON, A:%luMB]", (unsigned long)memAllocMB);
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