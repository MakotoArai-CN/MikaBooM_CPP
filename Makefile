# MikaBooM Legacy x86 GNU Makefile
# 用于构建 Windows 2000 / XP / 2003 专用的 legacy x86 发行物
# 现代多架构构建请使用 Makefile.msvc
#
# Toolchain priority:
#   1. i686-w64-mingw32-g++ (explicit cross-compiler)
#   2. g++ whose -dumpmachine starts with i686 (native 32-bit MinGW)
# An x86_64 g++ will NOT be used even if it is the only one on PATH.

SHELL = cmd

CROSS_X86 := $(shell where i686-w64-mingw32-g++ 2>nul)
LOCAL_GCC := $(shell where g++ 2>nul)
LOCAL_RC := $(shell where windres 2>nul)
CROSS_RC := $(shell where i686-w64-mingw32-windres 2>nul)
LLVM_OBJDUMP := $(shell where llvm-objdump 2>nul)
LOCAL_OBJDUMP := $(shell where objdump 2>nul)

ifdef LLVM_OBJDUMP
    OBJDUMP = llvm-objdump
else ifdef LOCAL_OBJDUMP
    OBJDUMP = objdump
endif

# Only accept a local g++ that truly targets i686 (32-bit).
# We must NOT match x86_64-w64-mingw32 which also contains "mingw32".
ifdef LOCAL_GCC
    GCC_TARGET := $(shell g++ -dumpmachine 2>nul)
    ifeq ($(findstring i686,$(GCC_TARGET)),i686)
        LOCAL_SUPPORTS_X86 = 1
    endif
endif

SRCDIR = src
RESDIR = res
OBJDIR = build\legacy_x86
TARGET = MikaBooM_x86_win2000.exe

EXPIRE_DATE := $(shell powershell -Command "Get-Date (Get-Date).AddYears(2) -Format 'yyyy-MM-ddTHH:mm:ss'" 2>nul)
ifeq ($(EXPIRE_DATE),)
    EXPIRE_DATE := 2027-12-31T23:59:59
endif

ifdef CROSS_X86
    CXX = i686-w64-mingw32-g++
  ifdef CROSS_RC
    RC = i686-w64-mingw32-windres
  else ifdef LOCAL_RC
    RC = windres
  endif
else ifdef LOCAL_SUPPORTS_X86
    CXX = g++
    RC = windres
endif

# i686 native compilers don't need -m32, but it doesn't hurt and is
# required when a cross-compiler is used on a 64-bit host.
ARCH_FLAGS = -m32

CXXFLAGS = -std=c++11 -Wall -O2 $(ARCH_FLAGS) \
           -static-libgcc -static-libstdc++ \
           -D_WIN32_WINNT=0x0500 \
           -DWINVER=0x0500 \
           -DFORCE_COMPAT_SAFE_STRING \
           -DEXPIRE_DATE=\"$(EXPIRE_DATE)\" \
           -ffunction-sections -fdata-sections

LDFLAGS = -static -static-libgcc -static-libstdc++ $(ARCH_FLAGS) \
          -Wl,--gc-sections \
          -lkernel32 -luser32 -lshell32 -ladvapi32 \
          -lpsapi -lcomctl32 -lwininet \
          -mconsole

OBJECTS = $(OBJDIR)\main.o \
          $(OBJDIR)\core\resource_monitor.o \
          $(OBJDIR)\core\config_manager.o \
          $(OBJDIR)\core\cpu_worker.o \
          $(OBJDIR)\core\memory_worker.o \
          $(OBJDIR)\platform\system_tray.o \
          $(OBJDIR)\platform\autostart.o \
          $(OBJDIR)\utils\console_utils.o \
          $(OBJDIR)\utils\version.o \
          $(OBJDIR)\utils\updater.o

RESOBJECT = $(OBJDIR)\resource.o

.DEFAULT_GOAL := legacy-x86

all: legacy-x86

legacy-x86: single-build

single-build: check-toolchain directories info-build $(TARGET)

check-toolchain:
ifndef CXX
	@echo [ERROR] No suitable x86 MinGW compiler found.
	@echo Install i686-w64-mingw32-g++ or a 32-bit MinGW g++ toolchain.
	@exit 1
endif
ifndef RC
	@echo [ERROR] No suitable resource compiler found.
	@echo Install i686-w64-mingw32-windres or windres.
	@exit 1
endif
ifndef OBJDUMP
	@echo [ERROR] No suitable objdump tool found.
	@echo Install llvm-objdump or objdump.
	@exit 1
endif

info-build:
	@echo ========================================
	@echo MikaBooM Legacy x86 Build System
	@echo ========================================
	@echo Output: $(TARGET)
	@echo Compiler: $(CXX)
	@echo Resource Compiler: $(RC)
	@echo Target OS: Windows 2000 / XP / 2003
	@echo Expire Date: $(EXPIRE_DATE)
	@echo ========================================

directories:
	@if not exist build mkdir build
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist $(OBJDIR)\core mkdir $(OBJDIR)\core
	@if not exist $(OBJDIR)\platform mkdir $(OBJDIR)\platform
	@if not exist $(OBJDIR)\utils mkdir $(OBJDIR)\utils

$(TARGET): $(OBJECTS) $(RESOBJECT)
	@echo Linking $(TARGET)...
	@$(CXX) $(OBJECTS) $(RESOBJECT) -o $@ $(LDFLAGS)
	@llvm-strip $@ 2>nul || strip -s $@ 2>nul || echo [INFO] Strip skipped
	@echo ========================================
	@echo BUILD SUCCESS: $(TARGET)
	@echo Target OS: Windows 2000 / XP / 2003
	@echo ========================================
	@if exist $(TARGET) dir $(TARGET) | findstr $(TARGET)
	@echo ========================================

$(OBJDIR)\main.o: $(SRCDIR)\main.cpp
	@echo [CXX] main.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\core\resource_monitor.o: $(SRCDIR)\core\resource_monitor.cpp
	@echo [CXX] resource_monitor.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\core\config_manager.o: $(SRCDIR)\core\config_manager.cpp
	@echo [CXX] config_manager.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\core\cpu_worker.o: $(SRCDIR)\core\cpu_worker.cpp
	@echo [CXX] cpu_worker.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\core\memory_worker.o: $(SRCDIR)\core\memory_worker.cpp
	@echo [CXX] memory_worker.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\platform\system_tray.o: $(SRCDIR)\platform\system_tray.cpp
	@echo [CXX] system_tray.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\platform\autostart.o: $(SRCDIR)\platform\autostart.cpp
	@echo [CXX] autostart.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\utils\console_utils.o: $(SRCDIR)\utils\console_utils.cpp
	@echo [CXX] console_utils.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\utils\version.o: $(SRCDIR)\utils\version.cpp
	@echo [CXX] version.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\utils\updater.o: $(SRCDIR)\utils\updater.cpp
	@echo [CXX] updater.cpp
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)\resource.o: $(RESDIR)\resource.rc
	@echo [RC] resource.rc
	@$(RC) -D_WIN32_WINNT=0x0500 $< -o $@

clean:
	@echo Cleaning...
	@if exist build rmdir /s /q build
	@if exist $(TARGET) del /f /q $(TARGET)
	@echo Clean complete.

rebuild: clean legacy-x86

check:
	@echo ========================================
	@echo Legacy x86 Toolchain Detection
	@echo ========================================
	@echo i686-w64-mingw32-g++: $(CROSS_X86)
	@echo i686-w64-mingw32-windres: $(CROSS_RC)
	@echo Local GCC: $(LOCAL_GCC)
	@echo Local RC: $(LOCAL_RC)
	@echo llvm-objdump: $(LLVM_OBJDUMP)
	@echo Local objdump: $(LOCAL_OBJDUMP)
	@echo GCC Target: $(GCC_TARGET)
	@echo Selected Compiler: $(CXX)
	@echo Selected Resource Compiler: $(RC)
	@echo Selected objdump: $(OBJDUMP)
	@echo ========================================

check-deps:
	@echo ========================================
	@echo Checking DLL Dependencies
	@echo ========================================
	@if exist $(TARGET) (echo $(TARGET): & $(OBJDUMP) -p $(TARGET) | findstr "DLL Name" & echo ========================================)
	@echo Legacy x86 binary should stay compatible with Windows 2000 / XP / 2003.
	@echo ========================================

check-imports:
	@echo ========================================
	@echo Checking forbidden imports for Win2000
	@echo ========================================
	@if not exist $(TARGET) (echo [ERROR] $(TARGET) not found & exit 1)
	@$(OBJDUMP) -p $(TARGET) | findstr /C:"AcquireSRWLockExclusive" /C:"InitOnceExecuteOnce" /C:"FlsAlloc" /C:"GetThreadId" && (echo [ERROR] Forbidden imports found & exit 1) || echo [OK] No forbidden imports found
	@echo ========================================

run:
	@if exist $(TARGET) ($(TARGET)) else echo No executable found

help:
	@echo ========================================
	@echo MikaBooM Legacy x86 Build System - Help
	@echo ========================================
	@echo   make legacy-x86    - Build MikaBooM_x86_win2000.exe
	@echo   make clean         - Remove build output
	@echo   make rebuild       - Clean and rebuild legacy x86
	@echo   make check         - Detect available x86 MinGW tools
	@echo   make check-deps    - Show DLL dependencies
	@echo   make check-imports - Check forbidden Win2000 imports
	@echo   make run           - Run the built executable
	@echo ========================================

.PHONY: all legacy-x86 single-build check-toolchain info-build directories \
        clean rebuild check check-deps check-imports run help
