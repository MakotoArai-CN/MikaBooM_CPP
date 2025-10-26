#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <psapi.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>

#include "core/resource_monitor.h"
#include "core/config_manager.h"
#include "core/cpu_worker.h"
#include "core/memory_worker.h"
#include "platform/system_tray.h"
#include "platform/autostart.h"
#include "utils/console_utils.h"
#include "utils/version.h"
#include "utils/system_info.h"

#ifdef _MSC_VER
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "pdh.lib")
#endif

volatile LONG g_running = 1;
volatile LONG g_show_window = 1;

ConfigManager* g_config = nullptr;
ResourceMonitor* g_monitor = nullptr;
CPUWorker* g_cpu_worker = nullptr;
MemoryWorker* g_memory_worker = nullptr;
SystemTray* g_tray = nullptr;

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT || signal == CTRL_CLOSE_EVENT) {
        InterlockedExchange(&g_running, 0);
        return TRUE;
    }
    return FALSE;
}

void ParseCommandLine(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            ConsoleUtils::ShowHelp();
            exit(0);
        } else if (arg == "-v" || arg == "--version") {
            ConsoleUtils::ShowVersion();
            exit(0);
        } else if (arg == "-auto") {
            ConsoleUtils::Init();
            if (AutoStart::Enable()) {
                ConsoleUtils::PrintSuccess(
                    ConsoleUtils::IsWindows7OrLater() ?
                    "自启动已启用" : "Auto-start enabled");
            } else {
                ConsoleUtils::PrintError(
                    ConsoleUtils::IsWindows7OrLater() ?
                    "启用自启动失败" : "Failed to enable auto-start");
            }
            exit(0);
        } else if (arg == "-noauto") {
            ConsoleUtils::Init();
            if (AutoStart::Disable()) {
                ConsoleUtils::PrintSuccess(
                    ConsoleUtils::IsWindows7OrLater() ?
                    "自启动已禁用" : "Auto-start disabled");
            } else {
                ConsoleUtils::PrintError(
                    ConsoleUtils::IsWindows7OrLater() ?
                    "禁用自启动失败" : "Failed to disable auto-start");
            }
            exit(0);
        } else if (arg == "-window" && i + 1 < argc) {
            std::string value = argv[++i];
            InterlockedExchange(&g_show_window, 
                (value == "true" || value == "1" || value == "yes" || value == "on") ? 1 : 0);
        } else if (arg == "-cpu" && i + 1 < argc) {
            int cpuThreshold = atoi(argv[++i]);
            if (cpuThreshold >= 0 && cpuThreshold <= 100) {
                g_config->SetCPUThreshold(cpuThreshold);
            }
        } else if (arg == "-mem" && i + 1 < argc) {
            int memThreshold = atoi(argv[++i]);
            if (memThreshold >= 0 && memThreshold <= 100) {
                g_config->SetMemoryThreshold(memThreshold);
            }
        } else if (arg == "-c" && i + 1 < argc) {
            g_config->SetConfigPath(argv[++i]);
        }
    }
}

void MonitorLoop() {
    DWORD last_update = GetTickCount();
    bool last_cpu_worker_state = false;
    bool last_mem_worker_state = false;
    
    // 迟滞计数器 - 防止频繁切换
    int cpu_start_count = 0;
    int cpu_stop_count = 0;
    int mem_start_count = 0;
    int mem_stop_count = 0;
    
    // 老系统需要更多确认次数
    int confirm_threshold = SystemInfo::IsWindows7OrLater() ? 2 : 3;
    
    MSG msg;
    while (g_running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        DWORD now = GetTickCount();
        DWORD elapsed_ms = now - last_update;
        
        if (elapsed_ms >= (DWORD)(g_config->GetUpdateInterval() * 1000)) {
            last_update = now;
            
            double total_cpu = g_monitor->GetCPUUsage();
            double total_mem = g_monitor->GetMemoryUsage();
            
            int cpu_threshold = g_config->GetCPUThreshold();
            int mem_threshold = g_config->GetMemoryThreshold();
            
            double cpu_worker_usage = 0;
            if (g_cpu_worker && g_cpu_worker->IsRunning()) {
                cpu_worker_usage = g_cpu_worker->GetUsage();
            }
            
            double other_cpu_usage = total_cpu - cpu_worker_usage;
            if (other_cpu_usage < 0) other_cpu_usage = 0;
            
            // 添加迟滞区间：±5%
            int hysteresis = 5;
            bool should_cpu_work;
            
            if (last_cpu_worker_state) {
                // 当前运行中，需要明显超过阈值才停止
                should_cpu_work = other_cpu_usage < (cpu_threshold + hysteresis);
            } else {
                // 当前停止中，需要明显低于阈值才启动
                should_cpu_work = other_cpu_usage < (cpu_threshold - hysteresis);
            }
            
            // 状态变化需要连续确认
            if (should_cpu_work != last_cpu_worker_state) {
                if (should_cpu_work) {
                    cpu_start_count++;
                    cpu_stop_count = 0;
                    
                    if (cpu_start_count >= confirm_threshold) {
                        if (g_cpu_worker) {
                            g_cpu_worker->Start();
                            if (g_show_window) {
                                ConsoleUtils::PrintInfo(
                                    ConsoleUtils::IsWindows7OrLater() ?
                                    "[CPU] 其他程序占用 %.1f%% < 阈值 %d%%，开始CPU计算" :
                                    "[CPU] Other %.1f%% < threshold %d%%, starting",
                                    other_cpu_usage, cpu_threshold);
                            }
                        }
                        last_cpu_worker_state = true;
                        cpu_start_count = 0;
                    }
                } else {
                    cpu_stop_count++;
                    cpu_start_count = 0;
                    
                    if (cpu_stop_count >= confirm_threshold) {
                        if (g_cpu_worker) {
                            g_cpu_worker->Stop();
                            if (g_show_window) {
                                ConsoleUtils::PrintInfo(
                                    ConsoleUtils::IsWindows7OrLater() ?
                                    "[CPU] 其他程序占用 %.1f%% >= 阈值 %d%%，停止CPU计算" :
                                    "[CPU] Other %.1f%% >= threshold %d%%, stopping",
                                    other_cpu_usage, cpu_threshold);
                            }
                        }
                        last_cpu_worker_state = false;
                        cpu_stop_count = 0;
                    }
                }
            } else {
                // 状态未变化，重置计数器
                cpu_start_count = 0;
                cpu_stop_count = 0;
            }
            
            // CPU负载调整
            if (last_cpu_worker_state && g_cpu_worker && g_cpu_worker->IsRunning()) {
                double target_cpu_worker_usage = cpu_threshold - other_cpu_usage;
                if (target_cpu_worker_usage < 0) target_cpu_worker_usage = 0;
                if (target_cpu_worker_usage > 100) target_cpu_worker_usage = 100;
                g_cpu_worker->AdjustLoad(cpu_worker_usage, target_cpu_worker_usage);
            }
            
            // === 内存部分同样处理 ===
            double mem_worker_usage = 0;
            if (g_memory_worker && g_memory_worker->IsRunning()) {
                mem_worker_usage = g_memory_worker->GetUsage();
            }
            
            double other_mem_usage = total_mem - mem_worker_usage;
            if (other_mem_usage < 0) other_mem_usage = 0;
            
            bool should_mem_work;
            if (last_mem_worker_state) {
                should_mem_work = other_mem_usage < (mem_threshold + hysteresis);
            } else {
                should_mem_work = other_mem_usage < (mem_threshold - hysteresis);
            }
            
            if (should_mem_work != last_mem_worker_state) {
                if (should_mem_work) {
                    mem_start_count++;
                    mem_stop_count = 0;
                    
                    if (mem_start_count >= confirm_threshold) {
                        if (g_memory_worker) {
                            g_memory_worker->Start();
                            if (g_show_window) {
                                ConsoleUtils::PrintInfo(
                                    ConsoleUtils::IsWindows7OrLater() ?
                                    "[MEM] 其他程序占用 %.1f%% < 阈值 %d%%，开始内存计算" :
                                    "[MEM] Other %.1f%% < threshold %d%%, starting",
                                    other_mem_usage, mem_threshold);
                            }
                        }
                        last_mem_worker_state = true;
                        mem_start_count = 0;
                    }
                } else {
                    mem_stop_count++;
                    mem_start_count = 0;
                    
                    if (mem_stop_count >= confirm_threshold) {
                        if (g_memory_worker) {
                            g_memory_worker->Stop();
                            if (g_show_window) {
                                ConsoleUtils::PrintInfo(
                                    ConsoleUtils::IsWindows7OrLater() ?
                                    "[MEM] 其他程序占用 %.1f%% >= 阈值 %d%%，停止内存计算" :
                                    "[MEM] Other %.1f%% >= threshold %d%%, stopping",
                                    other_mem_usage, mem_threshold);
                            }
                        }
                        last_mem_worker_state = false;
                        mem_stop_count = 0;
                    }
                }
            } else {
                mem_start_count = 0;
                mem_stop_count = 0;
            }
            
            // 内存负载调整
            if (last_mem_worker_state && g_memory_worker && g_memory_worker->IsRunning()) {
                double target_mem_worker_usage = mem_threshold - other_mem_usage;
                if (target_mem_worker_usage < 0) target_mem_worker_usage = 0;
                if (target_mem_worker_usage > 80) target_mem_worker_usage = 80;
                g_memory_worker->AdjustLoad(mem_worker_usage, target_mem_worker_usage);
            }
            
            // 更新托盘图标
            if (g_tray) {
                char tooltip[256];
                snprintf(tooltip, sizeof(tooltip),
                    "MikaBooM\nCPU: %.1f%% / %d%%\nMEM: %.1f%% / %d%%",
                    total_cpu, cpu_threshold, total_mem, mem_threshold);
                g_tray->UpdateTooltip(tooltip);
            }
            
            // 显示状态
            if (g_show_window) {
                ConsoleUtils::PrintStatus(
                    total_cpu, total_mem,
                    g_cpu_worker && g_cpu_worker->IsRunning(),
                    g_memory_worker && g_memory_worker->IsRunning(),
                    g_cpu_worker ? g_cpu_worker->GetIntensity() : 0,
                    g_memory_worker ? (size_t)(g_memory_worker->GetAllocatedSize() / 1024 / 1024) : 0
                );
            }
        }
        
        Sleep(100);
    }
}

int main(int argc, char* argv[]) {
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);
    
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);
    
    g_config = new ConfigManager();
    g_config->Load();
    
    ConsoleUtils::Init();
    ParseCommandLine(argc, argv);
    
    if (!g_show_window) {
        FreeConsole();
    } else {
        ConsoleUtils::ShowWelcome();
        bool useUTF8 = ConsoleUtils::IsWindows7OrLater();
        if (useUTF8) {
            printf(">> CPU阈值: %d%%\n", g_config->GetCPUThreshold());
            printf(">> 内存阈值: %d%%\n", g_config->GetMemoryThreshold());
        } else {
            printf("CPU Threshold: %d%%\n", g_config->GetCPUThreshold());
            printf("Memory Threshold: %d%%\n", g_config->GetMemoryThreshold());
        }
        printf("\n");
    }
    
    if (!Version::IsValid()) {
        if (g_show_window) {
            ConsoleUtils::PrintWarning(
                ConsoleUtils::IsWindows7OrLater() ?
                "版本已过期，仅监控模式" :
                "Version expired, monitoring mode only");
        }
        g_config->SetEnableWorker(false);
    }
    
    g_monitor = new ResourceMonitor();
    
    if (g_config->GetEnableWorker() && Version::IsValid()) {
        g_cpu_worker = new CPUWorker(g_config->GetCPUThreshold());
        g_memory_worker = new MemoryWorker(g_config->GetMemoryThreshold());
    }
    
    g_tray = new SystemTray();
    if (!g_tray->Create()) {
        if (g_show_window) {
            ConsoleUtils::PrintError(
                ConsoleUtils::IsWindows7OrLater() ?
                "创建系统托盘图标失败" :
                "Failed to create system tray icon");
        }
    }
    
    if (g_config->GetAutoStart()) {
        AutoStart::Enable();
    }
    
    if (g_show_window) {
        ConsoleUtils::PrintSuccess(
            ConsoleUtils::IsWindows7OrLater() ?
            "程序启动成功，开始监控..." :
            "Started successfully, monitoring...");
        printf("\n");
    }
    
    MonitorLoop();
    
    if (g_show_window) {
        ConsoleUtils::PrintInfo(
            ConsoleUtils::IsWindows7OrLater() ?
            "正在清理资源..." :
            "Cleaning up resources...");
    }
    
    if (g_cpu_worker) {
        g_cpu_worker->Stop();
        delete g_cpu_worker;
        g_cpu_worker = nullptr;
    }
    
    if (g_memory_worker) {
        g_memory_worker->Stop();
        delete g_memory_worker;
        g_memory_worker = nullptr;
    }
    
    if (g_tray) {
        g_tray->Destroy();
        delete g_tray;
        g_tray = nullptr;
    }
    
    if (g_monitor) {
        delete g_monitor;
        g_monitor = nullptr;
    }
    
    if (g_config) {
        g_config->Save();
        delete g_config;
        g_config = nullptr;
    }
    
    if (g_show_window) {
        ConsoleUtils::PrintSuccess(
            ConsoleUtils::IsWindows7OrLater() ?
            "程序已安全退出" :
            "Exited safely");
    }
    
    return 0;
}