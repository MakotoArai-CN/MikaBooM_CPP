#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SHOW 1002
#define ID_TRAY_AUTOSTART 1003

// 图标资源ID
#define IDI_MAIN 101

#ifndef NOTIFYICONDATAA_V2_SIZE
#define NOTIFYICONDATAA_V2_SIZE 488  // Windows 2000/XP/2003 结构体大小
#endif

class SystemTray {
private:
    HWND hwnd;
    NOTIFYICONDATAA nid;
    HMENU hMenu;
    static SystemTray* instance;

public:
    SystemTray();
    ~SystemTray();
    
    bool Create();
    void Destroy();
    void UpdateTooltip(const char* text);
    void ShowBalloon(const char* title, const char* text);

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void CreateTrayMenu();
    void OnTrayIcon(LPARAM lParam);
};