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
    bool checkUpdates;

    int memoryRandomMinMB;
    int memoryRandomMaxMB;
    int memoryRandomIntervalMinSec;
    int memoryRandomIntervalMaxSec;
    bool memoryRefreshEnabled;
    int memoryRefreshAfterSec;
    int memoryRefreshIntervalSec;
    int memoryRefreshStrideKB;

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
    bool GetCheckUpdates() const { return checkUpdates; }
    int GetMemoryRandomMinMB() const { return memoryRandomMinMB; }
    int GetMemoryRandomMaxMB() const { return memoryRandomMaxMB; }
    int GetMemoryRandomIntervalMinSec() const { return memoryRandomIntervalMinSec; }
    int GetMemoryRandomIntervalMaxSec() const { return memoryRandomIntervalMaxSec; }
    bool GetMemoryRefreshEnabled() const { return memoryRefreshEnabled; }
    int GetMemoryRefreshAfterSec() const { return memoryRefreshAfterSec; }
    int GetMemoryRefreshIntervalSec() const { return memoryRefreshIntervalSec; }
    int GetMemoryRefreshStrideKB() const { return memoryRefreshStrideKB; }

    void SetCPUThreshold(int value) { cpuThreshold = value; }
    void SetMemoryThreshold(int value) { memoryThreshold = value; }
    void SetAutoStart(bool value) { autoStart = value; }
    void SetShowWindow(bool value) { showWindow = value; }
    void SetUpdateInterval(int value) { updateInterval = value; }
    void SetNotificationEnabled(bool value) { notificationEnabled = value; }
    void SetNotificationCooldown(int value) { notificationCooldown = value; }
    void SetEnableWorker(bool value) { enableWorker = value; }
    void SetCheckUpdates(bool value) { checkUpdates = value; }
    void SetMemoryRandomMinMB(int value) { memoryRandomMinMB = value; }
    void SetMemoryRandomMaxMB(int value) { memoryRandomMaxMB = value; }
    void SetMemoryRandomIntervalMinSec(int value) { memoryRandomIntervalMinSec = value; }
    void SetMemoryRandomIntervalMaxSec(int value) { memoryRandomIntervalMaxSec = value; }
    void SetMemoryRefreshEnabled(bool value) { memoryRefreshEnabled = value; }
    void SetMemoryRefreshAfterSec(int value) { memoryRefreshAfterSec = value; }
    void SetMemoryRefreshIntervalSec(int value) { memoryRefreshIntervalSec = value; }
    void SetMemoryRefreshStrideKB(int value) { memoryRefreshStrideKB = value; }

private:
    void SetDefaults();
    std::string GetExePath();
    void ParseLine(const std::string& line);
    std::string Trim(const std::string& str);
};
