#pragma once

// Portable _s safe-string fallbacks for non-MSVC compilers (e.g. MinGW GCC
// linking msvcrt.dll).  When building with MSVC or a UCRT-based toolchain the
// _s functions are already available; these wrappers are only activated when
// the compiler does not provide them.
//
// Usage:
//   - Automatic: included in a classic msvcrt-based MinGW build, the header
//     detects that _s functions are missing and provides inline replacements.
//   - Forced: define FORCE_COMPAT_SAFE_STRING before including to always try
//     providing fallbacks (still guarded by per-function checks).
//
// MSVC and UCRT-based MinGW (llvm-mingw ucrt, MSYS2 ucrt64) already ship the
// real _s implementations; this header is a no-op in those environments.

#include <cstdio>
#include <cstring>
#include <ctime>
#include <cstdarg>

// ======================================================================
// Decide whether fallbacks are needed.
//
// We only provide them when ALL of these are true:
//   1. Not MSVC (_MSC_VER undefined)
//   2. The toolchain does NOT already expose _s via its CRT headers
//
// Detection: MinGW-w64 UCRT headers define __MINGW_SEC_WARN_STR when the
// secure overloads are available.  Classic msvcrt headers do not.
// ======================================================================

// Default: assume the CRT provides _s functions.
// We only flip the flag for known-deficient environments.

#if defined(__MINGW32__) && !defined(_MSC_VER) && !defined(__MINGW_SEC_WARN_STR)
  // Classic msvcrt-based MinGW (no _s in CRT headers).
  #define COMPAT_NEED_SAFE_STRING_FALLBACK 1
#endif

// Allow build system to force the check, but still respect per-function guards.
#if defined(FORCE_COMPAT_SAFE_STRING) && !defined(_MSC_VER) && !defined(__MINGW_SEC_WARN_STR)
  #ifndef COMPAT_NEED_SAFE_STRING_FALLBACK
  #define COMPAT_NEED_SAFE_STRING_FALLBACK 1
  #endif
#endif

#ifdef COMPAT_NEED_SAFE_STRING_FALLBACK

#include <cerrno>

// ------------------------------------------------------------------
// strcpy_s
// ------------------------------------------------------------------
#ifndef strcpy_s
inline int strcpy_s(char* dest, size_t destsz, const char* src) {
    if (!dest || !src || destsz == 0) return EINVAL;
    size_t len = strlen(src);
    if (len >= destsz) { dest[0] = '\0'; return ERANGE; }
    memcpy(dest, src, len + 1);
    return 0;
}
#endif

// ------------------------------------------------------------------
// strcat_s
// ------------------------------------------------------------------
#ifndef strcat_s
inline int strcat_s(char* dest, size_t destsz, const char* src) {
    if (!dest || !src || destsz == 0) return EINVAL;
    size_t dlen = strlen(dest);
    size_t slen = strlen(src);
    if (dlen + slen >= destsz) { dest[0] = '\0'; return ERANGE; }
    memcpy(dest + dlen, src, slen + 1);
    return 0;
}
#endif

// ------------------------------------------------------------------
// sprintf_s  (simplified: delegates to snprintf)
// ------------------------------------------------------------------
#ifndef sprintf_s
#define sprintf_s snprintf
#endif

// ------------------------------------------------------------------
// fopen_s
// ------------------------------------------------------------------
#ifndef fopen_s
inline int fopen_s(FILE** pFile, const char* filename, const char* mode) {
    if (!pFile) return EINVAL;
    *pFile = fopen(filename, mode);
    return (*pFile) ? 0 : errno;
}
#endif

// ------------------------------------------------------------------
// localtime_s  (MSVC signature: errno_t localtime_s(tm*, const time_t*))
// ------------------------------------------------------------------
#ifndef localtime_s
inline int localtime_s(struct tm* _tm, const time_t* _time) {
    if (!_tm || !_time) return EINVAL;
    struct tm* result = localtime(_time);
    if (!result) return EINVAL;
    *_tm = *result;
    return 0;
}
#endif

#endif // COMPAT_NEED_SAFE_STRING_FALLBACK
