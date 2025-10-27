#pragma once
#include <string>
#include <vector>
#include <windows.h>

class Updater {
public:
    struct UpdateInfo {
        bool available;
        std::string version;
        std::string downloadUrl;
    };
    
    typedef void (*ProgressCallback)(int percent, size_t downloaded, size_t total);
    
    Updater();
    ~Updater();
    
    // 一键更新（检测+下载+安装）
    void AutoUpdate();

private:
    UpdateInfo CheckForUpdates();
    bool DownloadToMemory(const std::string& url, std::vector<char>& buffer, ProgressCallback callback);
    bool ApplyUpdate(const std::vector<char>& newExeData);
    bool VerifyPEFormat(const std::vector<char>& data);
    bool ExtractDownloadUrl(const std::string& releaseUrl, std::string& directDownloadUrl);
    std::string GetCurrentExePath();
    std::string GetTempDir();
    std::string CreateUpdateBatch(const std::string& tempExePath);
};