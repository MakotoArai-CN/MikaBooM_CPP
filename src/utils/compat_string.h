#pragma once

// Portable _s safe-string fallbacks for non-MSVC compilers (e.g. MinGW GCC
// linking msvcrt.dll).  When building with MSVC or a UCRT-based toolchain the
// _s functions are already available; these wrappers are only activated when
// the compiler does not provide them.
//
// IMPORTANT: This header uses #define macros to redirect _s calls to standard
// C functions BEFORE the CRT headers declare them as msvcrt.dll imports.
// It MUST be included (or force-included via -include) before any CRT header.
//
// MSVC and UCRT-based MinGW (llvm-mingw ucrt, MSYS2 ucrt64) already ship the
// real _s implementations; this header is a no-op in those environments.

// ======================================================================
// Decide whether fallbacks are needed.
// ======================================================================

#if defined(__MINGW32__) && !defined(_MSC_VER) && !defined(__MINGW_SEC_WARN_STR)
  #define COMPAT_NEED_SAFE_STRING_FALLBACK 1
#endif

#if defined(FORCE_COMPAT_SAFE_STRING) && !defined(_MSC_VER) && !defined(__MINGW_SEC_WARN_STR)
  #ifndef COMPAT_NEED_SAFE_STRING_FALLBACK
  #define COMPAT_NEED_SAFE_STRING_FALLBACK 1
  #endif
#endif

#ifdef COMPAT_NEED_SAFE_STRING_FALLBACK

#endif // COMPAT_NEED_SAFE_STRING_FALLBACK — detection section

// Now pull in CRT headers
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdarg>
#include <cerrno>

#ifdef COMPAT_NEED_SAFE_STRING_FALLBACK

// ------------------------------------------------------------------
// Wrapper functions that will be called via macros
// ------------------------------------------------------------------

// sprintf_s wrapper: redirect to snprintf
inline int compat_sprintf_s(char* buffer, size_t sizeOfBuffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsnprintf(buffer, sizeOfBuffer, format, args);
    va_end(args);
    return result;
}

// sscanf_s wrapper: redirect to sscanf (our code only uses %d/%c, no %s)
inline int compat_sscanf_s(const char* buffer, const char* format, ...) {
    va_list args;
    va_start(args, format);
    int result = vsscanf(buffer, format, args);
    va_end(args);
    return result;
}

// ------------------------------------------------------------------
// strcpy_s
// ------------------------------------------------------------------
inline int compat_strcpy_s(char* dest, size_t destsz, const char* src) {
    if (!dest || !src || destsz == 0) return EINVAL;
    size_t len = strlen(src);
    if (len >= destsz) { dest[0] = '\0'; return ERANGE; }
    memcpy(dest, src, len + 1);
    return 0;
}

// ------------------------------------------------------------------
// strcat_s
// ------------------------------------------------------------------
inline int compat_strcat_s(char* dest, size_t destsz, const char* src) {
    if (!dest || !src || destsz == 0) return EINVAL;
    size_t dlen = strlen(dest);
    size_t slen = strlen(src);
    if (dlen + slen >= destsz) { dest[0] = '\0'; return ERANGE; }
    memcpy(dest + dlen, src, slen + 1);
    return 0;
}

// ------------------------------------------------------------------
// fopen_s
// ------------------------------------------------------------------
inline int compat_fopen_s(FILE** pFile, const char* filename, const char* mode) {
    if (!pFile) return EINVAL;
    *pFile = fopen(filename, mode);
    return (*pFile) ? 0 : errno;
}

// ------------------------------------------------------------------
// freopen_s
// ------------------------------------------------------------------
inline int compat_freopen_s(FILE** pFile, const char* filename, const char* mode, FILE* stream) {
    if (!pFile) return EINVAL;
    *pFile = freopen(filename, mode, stream);
    return (*pFile) ? 0 : errno;
}

// ------------------------------------------------------------------
// localtime_s  (MSVC signature: errno_t localtime_s(tm*, const time_t*))
// ------------------------------------------------------------------
inline int compat_localtime_s(struct tm* _tm, const time_t* _time) {
    if (!_tm || !_time) return EINVAL;
    struct tm* result = localtime(_time);
    if (!result) return EINVAL;
    *_tm = *result;
    return 0;
}

// ------------------------------------------------------------------
// Macro redirects — defined AFTER all wrapper functions
// ------------------------------------------------------------------
#define strcpy_s compat_strcpy_s
#define strcat_s compat_strcat_s
#define fopen_s compat_fopen_s
#define freopen_s compat_freopen_s
#define localtime_s compat_localtime_s
#define sprintf_s compat_sprintf_s
#define sscanf_s compat_sscanf_s

#endif // COMPAT_NEED_SAFE_STRING_FALLBACK
