#include "system_tray.h"
#include "../platform/autostart.h"
#include "../utils/system_info.h"
#include "../utils/console_utils.h"
#include <cstring>

extern volatile LONG g_running;
extern volatile LONG g_show_window;

SystemTray* SystemTray::instance = nullptr;

SystemTray::SystemTray() : hwnd(NULL), hMenu(NULL) {
    instance = this;
    memset(&nid, 0, sizeof(nid));
}

SystemTray::~SystemTray() {
    Destroy();
    instance = nullptr;
}

bool SystemTray::Create() {
    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "MikaBooMTrayClass";
    
    if (!RegisterClassExA(&wc)) {
        return false;
    }
    
    hwnd = CreateWindowExA(0, "MikaBooMTrayClass", "MikaBooM",
                          0, 0, 0, 0, 0, HWND_MESSAGE, NULL,
                          GetModuleHandle(NULL), NULL);
    
    if (!hwnd) {
        return false;
    }
    
    // Windows 2000/XP 使用较小的结构体
    DWORD major, minor;
    SystemInfo::GetRealWindowsVersion(major, minor);
    
    if (major < 6) {
        // Windows 2000/XP/2003/Vista
        nid.cbSize = NOTIFYICONDATAA_V2_SIZE;  // 旧版本大小
    } else {
        // Windows 7+
        nid.cbSize = sizeof(NOTIFYICONDATAA);
    }
    
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.uCallbackMessage = WM_TRAYICON;
    
    HINSTANCE hInstance = GetModuleHandle(NULL);
    
    nid.hIcon = LoadIconA(hInstance, MAKEINTRESOURCEA(IDI_MAIN));
    
    if (!nid.hIcon) {
        nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    }
    
    strncpy(nid.szTip, "MikaBooM - Resource Monitor", sizeof(nid.szTip) - 1);
    nid.szTip[sizeof(nid.szTip) - 1] = '\0';
    
    Shell_NotifyIconA(NIM_ADD, &nid);
    
    CreateTrayMenu();
    
    return true;
}

void SystemTray::Destroy() {
    if (nid.hWnd) {
        Shell_NotifyIconA(NIM_DELETE, &nid);
    }
    
    if (hMenu) {
        DestroyMenu(hMenu);
        hMenu = NULL;
    }
    
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = NULL;
    }
    
    UnregisterClassA("MikaBooMTrayClass", GetModuleHandle(NULL));
}

void SystemTray::UpdateTooltip(const char* text) {
    if (strlen(text) < sizeof(nid.szTip)) {
        strncpy(nid.szTip, text, sizeof(nid.szTip) - 1);
        nid.szTip[sizeof(nid.szTip) - 1] = '\0';
        Shell_NotifyIconA(NIM_MODIFY, &nid);
    }
}

void SystemTray::ShowBalloon(const char* title, const char* text) {
    DWORD major, minor;
    SystemInfo::GetRealWindowsVersion(major, minor);
    
    if (major < 5 || (major == 5 && minor == 0)) {
        // Windows 2000 不支持气泡
        return;
    }
    
    NOTIFYICONDATAA nid_balloon = nid;
    nid_balloon.uFlags |= NIF_INFO;
    nid_balloon.dwInfoFlags = NIIF_INFO;
    
    if (strlen(title) < sizeof(nid_balloon.szInfoTitle)) {
        strncpy(nid_balloon.szInfoTitle, title, sizeof(nid_balloon.szInfoTitle) - 1);
        nid_balloon.szInfoTitle[sizeof(nid_balloon.szInfoTitle) - 1] = '\0';
    }
    
    if (strlen(text) < sizeof(nid_balloon.szInfo)) {
        strncpy(nid_balloon.szInfo, text, sizeof(nid_balloon.szInfo) - 1);
        nid_balloon.szInfo[sizeof(nid_balloon.szInfo) - 1] = '\0';
    }
    
    Shell_NotifyIconA(NIM_MODIFY, &nid_balloon);
}

void SystemTray::CreateTrayMenu() {
    if (hMenu) {
        DestroyMenu(hMenu);
    }
    
    hMenu = CreatePopupMenu();
    
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_SHOW,
                g_show_window ? "Hide &Window" : "&Show Window");
    AppendMenuA(hMenu, MF_STRING | (AutoStart::IsEnabled() ? MF_CHECKED : 0),
                ID_TRAY_AUTOSTART, "&Auto Start");
    AppendMenuA(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuA(hMenu, MF_STRING, ID_TRAY_EXIT, "E&xit");
}

LRESULT CALLBACK SystemTray::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TRAYICON:
            if (instance) {
                instance->OnTrayIcon(lParam);
            }
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case ID_TRAY_EXIT:
                    InterlockedExchange(&g_running, 0);
                    break;
                    
                case ID_TRAY_SHOW:
                    if (g_show_window) {
                        FreeConsole();
                        InterlockedExchange(&g_show_window, 0);
                    } else {
                        AllocConsole();
                        
                        freopen("CONOUT$", "w", stdout);
                        freopen("CONOUT$", "w", stderr);
                        freopen("CONIN$", "r", stdin);
                        
                        ConsoleUtils::Reinit();
                        ConsoleUtils::ShowWelcome();
                        
                        InterlockedExchange(&g_show_window, 1);
                    }
                    
                    if (instance) {
                        instance->CreateTrayMenu();
                    }
                    break;
                    
                case ID_TRAY_AUTOSTART:
                    if (AutoStart::IsEnabled()) {
                        AutoStart::Disable();
                    } else {
                        AutoStart::Enable();
                    }
                    
                    if (instance) {
                        instance->CreateTrayMenu();
                    }
                    break;
            }
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    
    return 0;
}

void SystemTray::OnTrayIcon(LPARAM lParam) {
    if (lParam == WM_RBUTTONUP) {
        POINT pt;
        GetCursorPos(&pt);
        
        CreateTrayMenu();
        
        SetForegroundWindow(hwnd);
        TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        PostMessage(hwnd, WM_NULL, 0, 0);
    }
}