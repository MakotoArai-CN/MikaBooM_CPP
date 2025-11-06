#pragma once
#include <string>

class Version {
public:
    static const char* GetVersion() { 
        return "1.0.2"; 
    }
    
    static const char* GetBuildDate() { 
        return __DATE__ " " __TIME__; 
    }
    
    static const char* GetExpireDate() {
        #ifdef EXPIRE_DATE
            return EXPIRE_DATE;
        #else
            return "2027-12-31T23:59:59";
        #endif
    }
    
    static const char* GetAuthor() { 
        return "Makoto"; 
    }
    
    // 架构信息
    static const char* GetArch() {
        #if defined(__aarch64__) || defined(_M_ARM64)
            return "arm64";
        #elif defined(__arm__) || defined(_M_ARM)
            return "arm";
        #elif defined(__x86_64__) || defined(_M_X64)
            return "x64";
        #elif defined(__i386__) || defined(_M_IX86)
            return "x86";
        #else
            return "unknown";
        #endif
    }
    
    static bool IsValid();
    static int GetDaysUntilExpire();
    static void InitAsciiArt(bool isWin7OrLater);
    static std::string GetRandomAsciiArt();
    static bool CheckForUpdates(std::string& latestVersion, std::string& downloadUrl);
    static int CompareVersion(const char* v1, const char* v2);
    
private:
    static const char* asciiArtsUTF8[];
    static const char* asciiArtsCP437[];
    static const int asciiArtCountUTF8;
    static const int asciiArtCountCP437;
    static bool useUTF8Art;
    static const char* GITHUB_API_URL;
};