#pragma once
#include <string>

class Version {
public:
    static const char* GetVersion() { return "1.0.1"; }
    static const char* GetBuildDate() { return __DATE__ " " __TIME__; }
    static const char* GetExpireDate() {
#ifdef EXPIRE_DATE
        return EXPIRE_DATE;
#else
        return "2027-12-31 23:59:59";
#endif
    }
    static const char* GetAuthor() { return "Makoto"; }
    
    static bool IsValid();
    static int GetDaysUntilExpire();
    
    static void InitAsciiArt(bool isWin7OrLater);
    static std::string GetRandomAsciiArt();
    
    // 新增：版本检测功能
    static bool CheckForUpdates(std::string& latestVersion, std::string& downloadUrl);
    static int CompareVersion(const char* v1, const char* v2);

private:
    static const char* asciiArtsUTF8[];
    static const char* asciiArtsCP437[];
    static const int asciiArtCountUTF8;
    static const int asciiArtCountCP437;
    static bool useUTF8Art;
    
    // GitHub API
    static const char* GITHUB_API_URL;
};