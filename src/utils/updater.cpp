#include "updater.h"
#include "version.h"
#include "console_utils.h"
#include <windows.h>
#include <wininet.h>
#include <cstdio>

#ifdef _MSC_VER
#pragma comment(lib, "wininet.lib")
#endif

Updater::Updater() {
}

Updater::~Updater() {
}

std::string Updater::GetCurrentExePath() {
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return std::string(buffer);
}

std::string Updater::GetTempDir() {
    char tempPath[MAX_PATH];
    ::GetTempPathA(MAX_PATH, tempPath);
    return std::string(tempPath);
}

bool Updater::ExtractDownloadUrl(const std::string& releaseUrl, std::string& directDownloadUrl) {
    size_t tagPos = releaseUrl.find("/tag/");
    if (tagPos == std::string::npos) return false;
    
    std::string baseUrl = releaseUrl.substr(0, tagPos);
    std::string tag = releaseUrl.substr(tagPos + 5);
    
    directDownloadUrl = baseUrl + "/download/" + tag + "/MikaBooM.exe";
    return true;
}

bool Updater::VerifyPEFormat(const std::vector<char>& data) {
    if (data.size() < 64) return false;
    
    if (data[0] != 'M' || data[1] != 'Z') return false;
    
    DWORD peOffset = *(DWORD*)(&data[0x3C]);
    if (peOffset + 4 > data.size()) return false;
    
    if (data[peOffset] != 'P' || data[peOffset + 1] != 'E' ||
        data[peOffset + 2] != 0 || data[peOffset + 3] != 0) {
        return false;
    }
    
    return true;
}

Updater::UpdateInfo Updater::CheckForUpdates() {
    UpdateInfo info;
    info.available = false;
    
    std::string latestVersion, downloadUrl;
    if (Version::CheckForUpdates(latestVersion, downloadUrl)) {
        int cmp = Version::CompareVersion(latestVersion.c_str(), Version::GetVersion());
        if (cmp > 0) {
            info.available = true;
            info.version = latestVersion;
            info.downloadUrl = downloadUrl;
        }
    }
    
    return info;
}

bool Updater::DownloadToMemory(const std::string& url, std::vector<char>& buffer, ProgressCallback callback) {
    HINTERNET hInternet = NULL;
    HINTERNET hRequest = NULL;
    bool success = false;
    
    char tempBuffer[8192];
    DWORD bytesRead = 0;
    DWORD contentLength = 0;
    size_t totalDownloaded = 0;
    
    buffer.clear();
    
    // 随机化 User-Agent（伪装正常浏览器）
    const char* userAgents[] = {
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36",
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:109.0) Gecko/20100101",
        "Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36"
    };
    int agent_index = GetTickCount() % 3;
    
    hInternet = InternetOpenA(userAgents[agent_index], 
                             INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        ConsoleUtils::PrintError("Failed to initialize network");
        return false;
    }
    
    std::string downloadUrl;
    if (!ExtractDownloadUrl(url, downloadUrl)) {
        downloadUrl = url;
    }
    
    // 添加随机延迟（模拟人类行为）
    Sleep(100 + (rand() % 200));
    
    hRequest = InternetOpenUrlA(hInternet, downloadUrl.c_str(), NULL, 0,
                               INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest) {
        InternetCloseHandle(hInternet);
        ConsoleUtils::PrintError("Failed to connect to server");
        return false;
    }
    
    DWORD bufferSize = sizeof(contentLength);
    HttpQueryInfoA(hRequest, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
                  &contentLength, &bufferSize, NULL);
    
    if (contentLength > 0) {
        buffer.reserve(contentLength);
    }
    
    // 读取时添加随机延迟（避免流量异常）
    while (InternetReadFile(hRequest, tempBuffer, sizeof(tempBuffer), &bytesRead) && bytesRead > 0) {
        buffer.insert(buffer.end(), tempBuffer, tempBuffer + bytesRead);
        totalDownloaded += bytesRead;
        
        if (callback) {
            int percent = contentLength > 0 ? (int)((totalDownloaded * 100) / contentLength) : 0;
            callback(percent, totalDownloaded, contentLength);
        }
        
        // 随机延迟（1-5ms）
        if (totalDownloaded % (1024 * 1024) == 0) {
            Sleep(1 + (rand() % 5));
        }
    }
    
    if (!VerifyPEFormat(buffer)) {
        buffer.clear();
        ConsoleUtils::PrintError("Downloaded file is corrupted");
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hInternet);
        return false;
    }
    
    success = true;
    
    if (hRequest) InternetCloseHandle(hRequest);
    if (hInternet) InternetCloseHandle(hInternet);
    
    return success;
}

std::string Updater::CreateUpdateBatch(const std::string& tempExePath) {
    std::string currentPath = GetCurrentExePath();
    std::string backupPath = currentPath + ".old";
    std::string scriptPath = GetTempDir() + "mikaboom_update.bat";
    
    FILE* script = fopen(scriptPath.c_str(), "w");
    if (!script) return "";
    
    bool useUTF8 = ConsoleUtils::IsWindows7OrLater();
    
    fprintf(script, "@echo off\n");
    if (useUTF8) {
        fprintf(script, "chcp 65001 >nul 2>&1\n");
        fprintf(script, "title MikaBooM Update\n");
    } else {
        fprintf(script, "title MikaBooM Update\n");
    }
    fprintf(script, "timeout /t 2 /nobreak >nul\n");
    fprintf(script, "\n");
    
    fprintf(script, "if exist \"%s\" del /f /q \"%s\" >nul 2>&1\n", backupPath.c_str(), backupPath.c_str());
    fprintf(script, "move /y \"%s\" \"%s\" >nul 2>&1\n", currentPath.c_str(), backupPath.c_str());
    fprintf(script, "if errorlevel 1 goto cleanup\n");
    fprintf(script, "\n");
    
    fprintf(script, "move /y \"%s\" \"%s\" >nul 2>&1\n", tempExePath.c_str(), currentPath.c_str());
    fprintf(script, "if errorlevel 1 (\n");
    fprintf(script, "    move /y \"%s\" \"%s\" >nul 2>&1\n", backupPath.c_str(), currentPath.c_str());
    fprintf(script, "    goto cleanup\n");
    fprintf(script, ")\n");
    fprintf(script, "\n");
    
    fprintf(script, "if exist \"%s\" del /f /q \"%s\" >nul 2>&1\n", backupPath.c_str(), backupPath.c_str());
    fprintf(script, "start \"\" \"%s\"\n", currentPath.c_str());
    fprintf(script, "\n");
    
    fprintf(script, ":cleanup\n");
    fprintf(script, "if exist \"%s\" del /f /q \"%s\" >nul 2>&1\n", tempExePath.c_str(), tempExePath.c_str());
    fprintf(script, "(goto) 2>nul & del /f /q \"%%~f0\"\n");
    
    fclose(script);
    return scriptPath;
}

bool Updater::ApplyUpdate(const std::vector<char>& newExeData) {
    if (newExeData.empty()) return false;
    
    char tempFileName[64];
    sprintf(tempFileName, "mikaboom_new_%lu.tmp", GetTickCount());
    std::string tempExePath = GetTempDir() + tempFileName;
    
    FILE* tempFile = fopen(tempExePath.c_str(), "wb");
    if (!tempFile) {
        ConsoleUtils::PrintError("Failed to create temp file");
        return false;
    }
    
    fwrite(&newExeData[0], 1, newExeData.size(), tempFile);
    fclose(tempFile);
    
    std::string scriptPath = CreateUpdateBatch(tempExePath);
    if (scriptPath.empty()) {
        DeleteFileA(tempExePath.c_str());
        ConsoleUtils::PrintError("Failed to create update script");
        return false;
    }
    
    SHELLEXECUTEINFOA sei = {0};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = "open";
    sei.lpFile = scriptPath.c_str();
    sei.nShow = SW_HIDE;
    
    if (!ShellExecuteExA(&sei)) {
        DeleteFileA(tempExePath.c_str());
        DeleteFileA(scriptPath.c_str());
        ConsoleUtils::PrintError("Failed to start updater");
        return false;
    }
    
    ExitProcess(0);
    return true;
}

void Updater::AutoUpdate() {
    bool useUTF8 = ConsoleUtils::IsWindows7OrLater();
    
    printf("\n");
    ConsoleUtils::PrintInfo(useUTF8 ? "检查更新中..." : "Checking for updates...");
    
    UpdateInfo info = CheckForUpdates();
    
    if (!info.available) {
        ConsoleUtils::PrintSuccess(useUTF8 ? "当前已是最新版本" : "Already up to date");
        printf("\n");
        return;
    }
    
    printf("\n");
    ConsoleUtils::PrintWarning(
        useUTF8 ? "发现新版本: %s  (当前: %s)" : "New version: %s  (current: %s)",
        info.version.c_str(), Version::GetVersion());
    printf("\n");
    
    ConsoleUtils::PrintInfo(useUTF8 ? "开始下载..." : "Downloading...");
    printf("\n");
    
    std::vector<char> newExeData;
    bool downloadSuccess = DownloadToMemory(info.downloadUrl, newExeData, 
        [](int percent, size_t downloaded, size_t total) {
            if (total > 0) {
                printf("\r  ");
                int barWidth = 40;
                int pos = barWidth * percent / 100;
                printf("[");
                for (int i = 0; i < barWidth; ++i) {
                    if (i < pos) printf("=");
                    else if (i == pos) printf(">");
                    else printf(" ");
                }
                printf("] %d%%  %.2f/%.2f MB     ",
                       percent,
                       downloaded / (1024.0 * 1024.0),
                       total / (1024.0 * 1024.0));
                fflush(stdout);
            }
        });
    
    printf("\n\n");
    
    if (!downloadSuccess || newExeData.empty()) {
        return;
    }
    
    ConsoleUtils::PrintSuccess(
        useUTF8 ? "下载完成: %.2f MB" : "Download complete: %.2f MB",
        newExeData.size() / (1024.0 * 1024.0));
    printf("\n");
    
    ConsoleUtils::PrintInfo(useUTF8 ? "安装更新..." : "Installing update...");
    
    if (!ApplyUpdate(newExeData)) {
        return;
    }
}