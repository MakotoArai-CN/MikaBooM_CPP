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

// ------------------------------------------------------------------
// Macro redirects — these MUST be defined before CRT headers are
// pulled in so that the compiler never sees the msvcrt.dll import
// declarations for these symbols.
// ------------------------------------------------------------------

// sprintf_s -> snprintf
#ifndef sprintf_s
#define sprintf_s snprintf
#endif

// sscanf_s -> sscanf  (our codebase only uses %d / %c, no %s)
#ifndef sscanf_s
#define sscanf_s sscanf
#endif

#endif // COMPAT_NEED_SAFE_STRING_FALLBACK — macro section

// Now pull in CRT headers (safe: macros above will intercept declarations)
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdarg>

#ifdef COMPAT_NEED_SAFE_STRING_FALLBACK

#include <cerrno>

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
#define strcpy_s compat_strcpy_s

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
#define strcat_s compat_strcat_s

// ------------------------------------------------------------------
// fopen_s
// ------------------------------------------------------------------
inline int compat_fopen_s(FILE** pFile, const char* filename, const char* mode) {
    if (!pFile) return EINVAL;
    *pFile = fopen(filename, mode);
    return (*pFile) ? 0 : errno;
}
#define fopen_s compat_fopen_s

// ------------------------------------------------------------------
// freopen_s
// ------------------------------------------------------------------
inline int compat_freopen_s(FILE** pFile, const char* filename, const char* mode, FILE* stream) {
    if (!pFile) return EINVAL;
    *pFile = freopen(filename, mode, stream);
    return (*pFile) ? 0 : errno;
}
#define freopen_s compat_freopen_s

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
#define localtime_s compat_localtime_s

#endif // COMPAT_NEED_SAFE_STRING_FALLBACK
