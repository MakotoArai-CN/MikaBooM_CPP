#pragma once
#include <string>

class Version {
public:
    static const char* GetVersion() { return "1.0.0"; }
    static const char* GetBuildDate() { return __DATE__ " " __TIME__; }
    
    // 使用编译时注入的过期时间，如果未定义则使用默认值
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

private:
    static const char* asciiArtsUTF8[];
    static const char* asciiArtsCP437[];
    static const int asciiArtCountUTF8;
    static const int asciiArtCountCP437;
    static const int asciiArtCount;
    static bool useUTF8Art;
};