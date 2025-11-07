# MikaBooM C++ Edition Makefile
# 自动检测编译器并支持多架构 - Windows 2000+ 兼容
# 支持 LLVM-MinGW 和 GCC MinGW-w64
# 完整支持 x86/x64/ARM/ARM64 资源文件

SHELL = cmd

# 检测可用编译器
CROSS_X86 := $(shell where i686-w64-mingw32-g++ 2>nul)
CROSS_X64 := $(shell where x86_64-w64-mingw32-g++ 2>nul)
CROSS_ARM := $(shell where armv7-w64-mingw32-g++ 2>nul)
CROSS_ARM_ALT := $(shell where arm-w64-mingw32-g++ 2>nul)
CROSS_ARM64 := $(shell where aarch64-w64-mingw32-g++ 2>nul)

LOCAL_GCC := $(shell where g++ 2>nul)
LOCAL_RC := $(shell where windres 2>nul)
LLVM_RC := $(shell where llvm-rc 2>nul)
LLVM_CVTRES := $(shell where llvm-cvtres 2>nul)

# 优先使用 armv7，备用 arm
ifndef CROSS_ARM
    CROSS_ARM := $(CROSS_ARM_ALT)
endif

# 检测本地编译器支持的架构
ifdef LOCAL_GCC
    GCC_TARGET := $(shell g++ -dumpmachine 2>nul)
    ifeq ($(findstring x86_64,$(GCC_TARGET)),x86_64)
        LOCAL_SUPPORTS_X64 = 1
        LOCAL_SUPPORTS_X86 = 1
    else ifeq ($(findstring i686,$(GCC_TARGET)),i686)
        LOCAL_SUPPORTS_X86 = 1
    else ifeq ($(findstring mingw32,$(GCC_TARGET)),mingw32)
        LOCAL_SUPPORTS_X86 = 1
    endif
endif

SRCDIR = src
RESDIR = res

# 过期时间（全局）
EXPIRE_DATE := $(shell powershell -Command "Get-Date (Get-Date).AddYears(2) -Format 'yyyy-MM-ddTHH:mm:ss'" 2>nul)
ifeq ($(EXPIRE_DATE),)
    EXPIRE_DATE := 2027-12-31T23:59:59
endif

# 根据传入的ARCH参数设置编译器和标志
ifdef ARCH
    ifeq ($(ARCH),x86)
        ifdef CROSS_X86
            CXX = i686-w64-mingw32-g++
            RC = i686-w64-mingw32-windres
            ARCH_FLAGS = -m32
            RC_TARGET = i686-w64-mingw32
            RCFLAGS = --target=$(RC_TARGET) -F pe-i386
            USE_WINDRES = 1
            USE_LLVM_RC = 0
            ARCH_VALID = 1
        else ifdef LOCAL_SUPPORTS_X86
            CXX = g++
            RC = windres
            ARCH_FLAGS = -m32
            RC_TARGET = i686-w64-mingw32
            RCFLAGS = --target=$(RC_TARGET) -F pe-i386
            USE_WINDRES = 1
            USE_LLVM_RC = 0
            ARCH_VALID = 1
        endif
        ARCH_SUFFIX = _x86
        RC_MACHINE = X86
        ARCH_DEFINES = -DUSE_PDH
    else ifeq ($(ARCH),x64)
        ifdef CROSS_X64
            CXX = x86_64-w64-mingw32-g++
            RC = x86_64-w64-mingw32-windres
            ARCH_FLAGS = -m64
            RC_TARGET = x86_64-w64-mingw32
            RCFLAGS = --target=$(RC_TARGET) -F pe-x86-64
            USE_WINDRES = 1
            USE_LLVM_RC = 0
            ARCH_VALID = 1
        else ifdef LOCAL_SUPPORTS_X64
            CXX = g++
            RC = windres
            ARCH_FLAGS = -m64
            RC_TARGET = x86_64-w64-mingw32
            RCFLAGS = --target=$(RC_TARGET) -F pe-x86-64
            USE_WINDRES = 1
            USE_LLVM_RC = 0
            ARCH_VALID = 1
        endif
        ARCH_SUFFIX = _x64
        RC_MACHINE = X64
        ARCH_DEFINES = -DUSE_PDH
    else ifeq ($(ARCH),arm)
        ifdef CROSS_ARM
            # 检测是 armv7 还是 arm
            ARM_TARGET := $(shell $(CROSS_ARM) -dumpmachine 2>nul)
            ifeq ($(findstring armv7,$(ARM_TARGET)),armv7)
                CXX = armv7-w64-mingw32-g++
                RC_TARGET = armv7-w64-mingw32
            else
                CXX = arm-w64-mingw32-g++
                RC_TARGET = arm-w64-mingw32
            endif
            ARCH_FLAGS = -march=armv7-a
            ifdef LLVM_RC
                USE_WINDRES = 0
                USE_LLVM_RC = 1
            else
                USE_WINDRES = 0
                USE_LLVM_RC = 0
            endif
            ARCH_VALID = 1
        else
            ARCH_VALID = 0
        endif
        ARCH_SUFFIX = _arm
        RC_MACHINE = ARM
        ARCH_DEFINES = -DNO_PDH
    else ifeq ($(ARCH),arm64)
        ifdef CROSS_ARM64
            CXX = aarch64-w64-mingw32-g++
            ARCH_FLAGS = -march=armv8-a
            ifdef LLVM_RC
                USE_WINDRES = 0
                USE_LLVM_RC = 1
            else
                USE_WINDRES = 0
                USE_LLVM_RC = 0
            endif
            ARCH_VALID = 1
        else
            ARCH_VALID = 0
        endif
        ARCH_SUFFIX = _arm64
        RC_MACHINE = ARM64
        RC_TARGET = aarch64-w64-mingw32
        ARCH_DEFINES = -DNO_PDH
    endif

    # 如果架构无效，报错
    ifndef ARCH_VALID
        $(error [ERROR] Compiler for $(ARCH) not found! Please install LLVM-MinGW or appropriate cross-compiler.)
    endif

    TARGET = MikaBooM$(ARCH_SUFFIX).exe
    OBJDIR = build\$(ARCH)

    CXXFLAGS = -std=c++11 -Wall -O2 $(ARCH_FLAGS) $(ARCH_DEFINES) \
               -static-libgcc -static-libstdc++ \
               -D_WIN32_WINNT=0x0500 \
               -DWINVER=0x0500 \
               -DEXPIRE_DATE=\"$(EXPIRE_DATE)\" \
               -ffunction-sections -fdata-sections

    # 根据架构设置链接库
    BASE_LDFLAGS = -static -static-libgcc -static-libstdc++ $(ARCH_FLAGS) \
                   -Wl,--gc-sections \
                   -lkernel32 -luser32 -lshell32 -ladvapi32 \
                   -lpsapi -lcomctl32
    
    ifeq ($(ARCH),arm)
        LDFLAGS = $(BASE_LDFLAGS) -lwininet -mconsole
    else ifeq ($(ARCH),arm64)
        LDFLAGS = $(BASE_LDFLAGS) -lwininet -mconsole
    else
        LDFLAGS = $(BASE_LDFLAGS) -lpdh -lwininet -mconsole
    endif

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

    ifeq ($(USE_WINDRES),1)
        RESOBJECT = $(OBJDIR)\resource.o
    else ifeq ($(USE_LLVM_RC),1)
        RESOBJECT = $(OBJDIR)\resource.o
        RESFILE = $(OBJDIR)\resource.res
    else
        RESOBJECT =
    endif
endif

# ========================================
# 默认目标：自动编译
# ========================================
.DEFAULT_GOAL := auto-build

all: auto-build

# 自动编译（根据编译器能力）
auto-build:
	@echo ========================================
	@echo MikaBooM Auto Build System
	@echo ========================================
	@echo Detecting compiler capabilities...
ifndef LOCAL_GCC
	@echo [ERROR] No GCC compiler found!
	@echo Please install MinGW-w64 or LLVM-MinGW
	@exit 1
endif
	@echo [OK] GCC found: $(LOCAL_GCC)
	@echo [OK] Target: $(GCC_TARGET)
ifdef LLVM_RC
	@echo [OK] LLVM RC: $(LLVM_RC)
	@echo [OK] LLVM CVTRES: $(LLVM_CVTRES)
else
	@echo [INFO] LLVM RC not found - ARM will skip resources
endif
	@echo ========================================
	@echo Supported architectures:
ifdef LOCAL_SUPPORTS_X86
	@echo   [OK] x86 (32-bit) - Will compile WITH ICON
	@$(MAKE) --no-print-directory ARCH=x86 single-build
else
	@echo   [SKIP] x86 (32-bit) - Not supported
endif
ifdef LOCAL_SUPPORTS_X64
	@echo   [OK] x64 (64-bit) - Will compile WITH ICON
	@$(MAKE) --no-print-directory ARCH=x64 single-build
else
	@echo   [SKIP] x64 (64-bit) - Not supported
endif
ifdef CROSS_ARM
ifdef LLVM_RC
	@echo   [OK] ARM (32-bit) - Will compile WITH ICON
else
	@echo   [OK] ARM (32-bit) - Will compile [NO ICON]
endif
	@$(MAKE) --no-print-directory ARCH=arm single-build
else
	@echo   [SKIP] ARM (32-bit) - Compiler not found (need armv7-w64-mingw32-g++ or arm-w64-mingw32-g++)
endif
ifdef CROSS_ARM64
ifdef LLVM_RC
	@echo   [OK] ARM64 (64-bit) - Will compile WITH ICON
else
	@echo   [OK] ARM64 (64-bit) - Will compile [NO ICON]
endif
	@$(MAKE) --no-print-directory ARCH=arm64 single-build
else
	@echo   [SKIP] ARM64 (64-bit) - Compiler not found (need aarch64-w64-mingw32-g++)
endif
	@echo ========================================
	@echo Build Summary
	@echo ========================================
	@if exist MikaBooM_x86.exe (echo [OK] MikaBooM_x86.exe [WITH ICON] & dir MikaBooM_x86.exe | findstr "MikaBooM")
	@if exist MikaBooM_x64.exe (echo [OK] MikaBooM_x64.exe [WITH ICON] & dir MikaBooM_x64.exe | findstr "MikaBooM")
	@if exist MikaBooM_arm.exe (echo [OK] MikaBooM_arm.exe & dir MikaBooM_arm.exe | findstr "MikaBooM")
	@if exist MikaBooM_arm64.exe (echo [OK] MikaBooM_arm64.exe & dir MikaBooM_arm64.exe | findstr "MikaBooM")
	@echo ========================================

# ========================================
# 单独架构编译（用户可调用）
# ========================================
x86:
	@$(MAKE) --no-print-directory ARCH=x86 single-build

x64:
	@$(MAKE) --no-print-directory ARCH=x64 single-build

arm:
	@$(MAKE) --no-print-directory ARCH=arm single-build

arm64:
	@$(MAKE) --no-print-directory ARCH=arm64 single-build

# ========================================
# 单个架构编译流程
# ========================================
single-build: directories info-build $(TARGET)

info-build:
	@echo ========================================
	@echo Building: $(TARGET)
	@echo Compiler: $(CXX)
ifeq ($(USE_WINDRES),1)
	@echo RC: $(RC) [windres]
	@echo RC Flags: $(RCFLAGS)
else ifeq ($(USE_LLVM_RC),1)
	@echo RC: llvm-rc + llvm-cvtres [LLVM]
	@echo RC Machine: $(RC_MACHINE)
else
	@echo RC: [DISABLED - No icon]
endif
	@echo Arch: $(ARCH)
	@echo Expire: $(EXPIRE_DATE)
	@echo ========================================

directories:
	@if not exist build mkdir build
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist $(OBJDIR)\core mkdir $(OBJDIR)\core
	@if not exist $(OBJDIR)\platform mkdir $(OBJDIR)\platform
	@if not exist $(OBJDIR)\utils mkdir $(OBJDIR)\utils

# 目标文件
$(TARGET): $(OBJECTS) $(RESOBJECT)
	@echo Linking $(TARGET)...
	@$(CXX) $(OBJECTS) $(RESOBJECT) -o $@ $(LDFLAGS)
	@llvm-strip $@ 2>nul || strip -s $@ 2>nul || echo [INFO] Strip skipped
	@echo ========================================
	@echo BUILD SUCCESS: $(TARGET)
	@echo Target OS: Windows 2000 / XP / Vista / 7+
	@echo ========================================
	@if exist $(TARGET) dir $(TARGET) | findstr $(TARGET)
	@echo ========================================

# ========================================
# 编译规则
# ========================================
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

# ========================================
# 资源文件编译规则
# ========================================

# 使用 windres (x86/x64)
ifeq ($(USE_WINDRES),1)
$(OBJDIR)\resource.o: $(RESDIR)\resource.rc
	@echo [RC] resource.rc [windres]
	@echo [RC] Target: $(RC_TARGET)
	@$(RC) $(RCFLAGS) -D_WIN32_WINNT=0x0500 -i $< -o $@
endif

# 使用 llvm-rc + llvm-cvtres (ARM/ARM64)
ifeq ($(USE_LLVM_RC),1)
$(OBJDIR)\resource.res: $(RESDIR)\resource.rc
	@echo [RC] resource.rc [llvm-rc]
	@echo [RC] Machine: $(RC_MACHINE)
	@llvm-rc /FO$(OBJDIR)\resource.res $(RESDIR)\resource.rc
	
$(OBJDIR)\resource.o: $(OBJDIR)\resource.res
	@echo [CVTRES] resource.res -> resource.o
	@llvm-cvtres /MACHINE:$(RC_MACHINE) /OUT:$@ $<
endif

# ========================================
# 实用工具
# ========================================
clean:
	@echo Cleaning...
	@if exist build rmdir /s /q build
	@if exist MikaBooM_x86.exe del /f /q MikaBooM_x86.exe
	@if exist MikaBooM_x64.exe del /f /q MikaBooM_x64.exe
	@if exist MikaBooM_arm.exe del /f /q MikaBooM_arm.exe
	@if exist MikaBooM_arm64.exe del /f /q MikaBooM_arm64.exe
	@echo Clean complete.

rebuild: clean auto-build

check:
	@echo ========================================
	@echo Compiler Detection
	@echo ========================================
	@echo Local GCC: $(LOCAL_GCC)
	@echo Local RC: $(LOCAL_RC)
	@echo GCC Target: $(GCC_TARGET)
	@echo ========================================
	@echo Cross Compilers:
	@echo   x86: $(CROSS_X86)
	@echo   x64: $(CROSS_X64)
	@echo   ARM: $(CROSS_ARM)
	@echo   ARM64: $(CROSS_ARM64)
	@echo ========================================
	@echo LLVM Tools:
	@echo   llvm-rc: $(LLVM_RC)
	@echo   llvm-cvtres: $(LLVM_CVTRES)
	@echo ========================================
	@echo Architecture Support:
ifdef LOCAL_SUPPORTS_X86
	@echo   [OK] x86 (32-bit) WITH ICON
else
	@echo   [SKIP] x86 (32-bit)
endif
ifdef LOCAL_SUPPORTS_X64
	@echo   [OK] x64 (64-bit) WITH ICON
else
	@echo   [SKIP] x64 (64-bit)
endif
ifdef CROSS_ARM
ifdef LLVM_RC
	@echo   [OK] ARM (32-bit) WITH ICON
else
	@echo   [OK] ARM (32-bit) NO ICON
endif
else
	@echo   [SKIP] ARM (32-bit) - Need armv7-w64-mingw32-g++ or arm-w64-mingw32-g++
endif
ifdef CROSS_ARM64
ifdef LLVM_RC
	@echo   [OK] ARM64 (64-bit) WITH ICON
else
	@echo   [OK] ARM64 (64-bit) NO ICON
endif
else
	@echo   [SKIP] ARM64 (64-bit) - Need aarch64-w64-mingw32-g++
endif
	@echo ========================================

check-deps:
	@echo ========================================
	@echo Checking DLL Dependencies
	@echo ========================================
	@if exist MikaBooM_x86.exe (echo MikaBooM_x86.exe: & llvm-objdump -p MikaBooM_x86.exe 2>nul | findstr "DLL Name" || objdump -p MikaBooM_x86.exe | findstr "DLL Name" & echo ========================================)
	@if exist MikaBooM_x64.exe (echo MikaBooM_x64.exe: & llvm-objdump -p MikaBooM_x64.exe 2>nul | findstr "DLL Name" || objdump -p MikaBooM_x64.exe | findstr "DLL Name" & echo ========================================)
	@if exist MikaBooM_arm.exe (echo MikaBooM_arm.exe: & llvm-objdump -p MikaBooM_arm.exe 2>nul | findstr "DLL Name" || objdump -p MikaBooM_arm.exe | findstr "DLL Name" & echo ========================================)
	@if exist MikaBooM_arm64.exe (echo MikaBooM_arm64.exe: & llvm-objdump -p MikaBooM_arm64.exe 2>nul | findstr "DLL Name" || objdump -p MikaBooM_arm64.exe | findstr "DLL Name" & echo ========================================)
	@echo All DLLs should be available in Windows 2000
	@echo ========================================

run:
	@if exist MikaBooM_x64.exe (MikaBooM_x64.exe) else if exist MikaBooM_x86.exe (MikaBooM_x86.exe) else if exist MikaBooM_arm64.exe (MikaBooM_arm64.exe) else if exist MikaBooM_arm.exe (MikaBooM_arm.exe) else echo No executable found

.PHONY: all auto-build x86 x64 arm arm64 single-build info-build \
        directories clean rebuild check check-deps run