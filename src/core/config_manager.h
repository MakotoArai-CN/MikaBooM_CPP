#pragma once
#include <string>
#include <map>

class ConfigManager {
private:
    std::string configPath;
    std::map<std::string, std::string> config;
    
    int cpuThreshold;
    int memoryThreshold;
    bool autoStart;
    bool showWindow;
    int updateInterval;
    bool notificationEnabled;
    int notificationCooldown;
    bool enableWorker;
    bool checkUpdates;  // 新增：是否检查更新

public:
    ConfigManager();
    ~ConfigManager();
    
    void Load();
    void Save();
    void SetConfigPath(const std::string& path);
    
    int GetCPUThreshold() const { return cpuThreshold; }
    int GetMemoryThreshold() const { return memoryThreshold; }
    bool GetAutoStart() const { return autoStart; }
    bool GetShowWindow() const { return showWindow; }
    int GetUpdateInterval() const { return updateInterval; }
    bool GetNotificationEnabled() const { return notificationEnabled; }
    int GetNotificationCooldown() const { return notificationCooldown; }
    bool GetEnableWorker() const { return enableWorker; }
    bool GetCheckUpdates() const { return checkUpdates; }  // 新增
    
    void SetCPUThreshold(int value) { cpuThreshold = value; }
    void SetMemoryThreshold(int value) { memoryThreshold = value; }
    void SetAutoStart(bool value) { autoStart = value; }
    void SetShowWindow(bool value) { showWindow = value; }
    void SetUpdateInterval(int value) { updateInterval = value; }
    void SetNotificationEnabled(bool value) { notificationEnabled = value; }
    void SetNotificationCooldown(int value) { notificationCooldown = value; }
    void SetEnableWorker(bool value) { enableWorker = value; }
    void SetCheckUpdates(bool value) { checkUpdates = value; }  // 新增

private:
    void SetDefaults();
    std::string GetExePath();
    void ParseLine(const std::string& line);
    std::string Trim(const std::string& str);
};