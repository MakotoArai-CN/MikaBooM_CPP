# MikaBooM C++ Edition Makefile
# 支持 MinGW - 静态链接

CXX = g++
RC = windres
TARGET = MikaBooM.exe
SRCDIR = src
RESDIR = res
OBJDIR = build

# 获取编译时间（使用 ISO 8601 格式，避免空格问题）
# 格式: 2026-12-26T16:00:47
EXPIRE_DATE := $(shell powershell -Command "Get-Date (Get-Date).AddYears(2) -Format 'yyyy-MM-ddTHH:mm:ss'")

# 如果 PowerShell 失败，使用备用方案
ifeq ($(EXPIRE_DATE),)
    CURRENT_YEAR := $(shell date +%Y)
    EXPIRE_YEAR := $(shell echo $$(($(CURRENT_YEAR) + 2)))
    EXPIRE_DATE := $(shell date +'$(EXPIRE_YEAR)-%m-%dT%H:%M:%S')
endif

# 编译标志 - 基础静态链接 + 注入过期时间
# 注意：使用单引号包裹宏定义值，避免空格问题
CXXFLAGS = -std=c++11 -Wall -O2 \
           -static-libgcc -static-libstdc++ \
           -D_WIN32_WINNT=0x0500 \
           -DWINVER=0x0500 \
           -DEXPIRE_DATE=\"$(EXPIRE_DATE)\"

# 链接标志 - 静态链接 C++ 运行库，确保包含 PDH
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
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	@if not exist $(OBJDIR)\core mkdir $(OBJDIR)\core
	@if not exist $(OBJDIR)\platform mkdir $(OBJDIR)\platform
	@if not exist $(OBJDIR)\utils mkdir $(OBJDIR)\utils

# 链接
$(TARGET): $(OBJECTS) $(RESOBJECT)
	@echo ========================================
	@echo Build Information:
	@echo ----------------------------------------
	@echo Expire Date: $(EXPIRE_DATE)
	@echo ========================================
	@echo Linking $(TARGET)...
	@echo ========================================
	$(CXX) $(OBJECTS) $(RESOBJECT) -o $@ $(LDFLAGS)
	@echo Stripping symbols to reduce size...
	@strip -s $@
	@echo ========================================
	@echo Build complete: $(TARGET)
	@echo Expire Date: $(EXPIRE_DATE)
	@echo ========================================
	@dir $(TARGET) | findstr $(TARGET)
	@echo ========================================

# 编译源文件
$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo Compiling $<...
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# 编译资源文件
$(OBJDIR)/resource.o: $(RESOURCE)
	@echo Compiling resources...
	@$(RC) -D_WIN32_WINNT=0x0500 $< -o $@

# 显示构建信息
info:
	@echo ========================================
	@echo MikaBooM Build Information
	@echo ========================================
	@echo Expire Date: $(EXPIRE_DATE)
	@echo ========================================

# 检查依赖项
check: $(TARGET)
	@echo ========================================
	@echo Checking DLL dependencies...
	@echo ========================================
	@objdump -p $(TARGET) | findstr "DLL Name"
	@echo ========================================

# 清理
clean:
	@echo Cleaning build files...
	@if exist $(OBJDIR) rmdir /s /q $(OBJDIR)
	@if exist $(TARGET) del $(TARGET)
	@echo Clean complete.

# 完全重新编译
rebuild: clean all

# 运行
run: $(TARGET)
	$(TARGET)

# 运行测试
test: $(TARGET)
	@echo ========================================
	@echo Testing parameters...
	@echo ========================================
	@echo.
	@echo Test 1: Help
	@echo ----------------------------------------
	@$(TARGET) -h
	@echo.
	@echo Test 2: Version
	@echo ----------------------------------------
	@$(TARGET) -v
	@echo.
	@echo ========================================

.PHONY: all clean rebuild run check test directories info