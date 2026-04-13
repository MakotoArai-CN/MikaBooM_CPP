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

# 编译标志 - 注入过期时间与目标平台
# 注意：使用单引号包裹宏定义值，避免空格问题
CXXFLAGS = -std=c++11 -Wall -O2 \
           -D_WIN32_WINNT=0x0500 \
           -DWINVER=0x0500 \
           -DEXPIRE_DATE=\"$(EXPIRE_DATE)\"

# 链接标志 - 静态链接运行库，确保包含 PDH
LDFLAGS = -static-libgcc -static-libstdc++ \
          -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic \
          -lkernel32 -luser32 -lshell32 -ladvapi32 \
          -lpsapi -lcomctl32 -lpdh -lwininet \
          -mconsole

# 源文件
SOURCES = $(SRCDIR)/main.cpp \
          $(SRCDIR)/core/resource_monitor.cpp \
          $(SRCDIR)/core/config_manager.cpp \
          $(SRCDIR)/core/cpu_worker.cpp \
          $(SRCDIR)/core/memory_worker.cpp \
          $(SRCDIR)/platform/system_tray.cpp \
          $(SRCDIR)/platform/autostart.cpp \
          $(SRCDIR)/utils/console_utils.cpp \
          $(SRCDIR)/utils/version.cpp \
          $(SRCDIR)/utils/updater.cpp

RESOURCE = $(RESDIR)/resource.rc
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
RESOBJECT = $(OBJDIR)/resource.o

# 默认目标
all: directories $(TARGET)

# 创建目录
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
	@echo All DLLs should be available on the target OS
	@echo ========================================

.PHONY: all clean rebuild run check test directories info