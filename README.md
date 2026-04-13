<!-- markdownlint-disable MD033 MD041 -->
<div align="center">
    <img src="./res/icon.png" width="128" height="128" alt="MikaBooM">
    <h1>MikaBooM C++ Edition</h1>
</div>

Windows系统资源监控与调整工具 - C++版本，让你的Windows发光发热！

> Windows 7 以上系统建议使用 Go 版本 [MikaBooM](https://github.com/MakotoArai-CN/MikaBooM)，更稳定、跨平台能力更强、程序数据更精确。
>
> **⚠ 免责声明**
>
> - 本软件仅用于学习交流，请勿用于非法用途。
> - 高强度运算可能导致硬件过热、寿命缩减，请自行承担风险。

## 特性

- 支持多架构：modern x86、modern x64、legacy x86、ARM、ARM64
- Windows 2000 / XP / 2003 通过独立的 legacy x86 发行物支持
- modern MSVC 构建不再宣称单个 x86 exe 覆盖 Windows 2000 到 Windows 11
- 单文件可执行程序，无需依赖
- CPU 和内存使用率实时监控
- 智能负载调整
- 系统托盘图标
- 开机自启动
- 配置文件支持

## 编译

> 项目测试阶段可能不稳定，所有测试环境均为虚拟机环境测试。
>
> 测试版未设置更新机制，因此设置了有效期；编译后有两年的有效期，过期后将无法运行。欢迎给本项目点个 Star 以便跟进项目更新。
>
> 有效期将在正式版发布后更改为 GitHub 仓库更新方式。

### 使用 MSVC（推荐）

```bash
# 构建当前架构（默认 x64）
nmake /F Makefile.msvc

# 指定架构
nmake /F Makefile.msvc ARCH=x86
nmake /F Makefile.msvc ARCH=x64
nmake /F Makefile.msvc ARCH=arm
nmake /F Makefile.msvc ARCH=arm64

# 构建全部架构
nmake /F Makefile.msvc all-arch
```

### 使用 GNU Make（legacy x86）

```bash
# 构建 Windows 2000 / XP / 2003 专用 x86 包
make legacy-x86

# 检查 Win2000 禁止导入项
make check-imports
```

### 重新编译

```bash
make rebuild
```

## 输出文件

编译后会生成以下文件：

| 文件 | 架构 | 说明 |
| --- | --- | --- |
| `MikaBooM_x86.exe` | modern x86 (32-bit) | 现代 MSVC 构建，不再宣称支持 Windows 2000 |
| `MikaBooM_x64.exe` | modern x64 (64-bit) | 现代 MSVC 构建 |
| `MikaBooM_x86_win2000.exe` | legacy x86 (32-bit) | Windows 2000 / XP / 2003 专用发行物 |
| `MikaBooM_arm.exe` | ARM (32-bit) | Windows RT / 10 ARM |
| `MikaBooM_arm64.exe` | ARM64 (64-bit) | Windows 10/11 ARM64 |

当前仓库采用 modern / legacy 双轨发布：Win2000/XP 仅由独立 legacy x86 包承诺，不能再把 modern x86 视为 Win2000 通用包。

## 使用

```bash
# 显示帮助
MikaBooM_x64.exe -h

# 设置CPU阈值
MikaBooM_x64.exe -cpu 80

# 设置内存阈值
MikaBooM_x64.exe -mem 70

# 后台运行
MikaBooM_x64.exe -window false

# 启用自启动
MikaBooM_x64.exe -auto

# 禁用自启动
MikaBooM_x64.exe -noauto

# 更新程序
MikaBooM_x64.exe -update
```

## 配置文件

程序会在同目录下创建 `config.ini` 文件：

```ini
[General]
cpu_threshold=45
memory_threshold=69
auto_start=true
show_window=true
update_interval=2
check_updates=true

[Notification]
enabled=true
cooldown=60

[MemoryWorker]
random_min_mb=256
random_max_mb=512
random_interval_min_sec=30
random_interval_max_sec=90
refresh_enabled=true
refresh_after_sec=60
refresh_interval_sec=30
refresh_stride_kb=4
```

新增参数示例：

- `MikaBooM_x64.exe -mem-min 256 -mem-max 768`
- `MikaBooM_x64.exe -mem-freq-min 30 -mem-freq-max 120`
- `MikaBooM_x64.exe -mem-refresh true -mem-refresh-interval 30 -mem-refresh-stride 4`

运行中状态会额外显示：

- 目标内存值
- 已申请内存值
- 估算驻留内存值
- 驻留比例
- 页面刷新是否启用

## 自动发布

- 新增 GitHub Actions 工作流 `.github/workflows/Release.yml`
- GitHub Hosted Runner 自动构建 modern 架构：`x86`、`x64`、`ARM`、`ARM64`
- 额外构建独立的 legacy x86 发行物：`MikaBooM_x86_win2000.exe`
- modern 构建由 MSVC 交叉编译完成；legacy x86 通过 GNU Make / MinGW 链路构建
- ARM 发行物使用 `windows-2022` + `10.0.22621.0` SDK，其余 modern 架构使用 `windows-latest` + `10.0.26100.0`
- Release 仅发布实际构建成功的 `.exe` 产物，不上传 README 等说明文件
- 发布版本以 `src/utils/version.h` 为准，并校验 `res/resource.rc` 是否同步
- legacy x86 发布前会执行导入表审计，避免误发包含 Win2000 禁止导入项的二进制

## 兼容性

当前仓库采用 modern / legacy 双轨兼容策略：modern 构建覆盖较新的 Windows 版本，Windows 2000 / XP / 2003 由独立 legacy x86 发行物承诺。

| Windows 版本 | modern x86 | modern x64 | legacy x86 | ARM | ARM64 |
| --- | --- | --- | --- | --- | --- |
| Windows 2000 | - | - | ✅ | - | - |
| Windows XP (32-bit) | - | - | ✅ | - | - |
| Windows XP x64 / Server 2003 | - | ⚠️ | - | - | - |
| Windows Vista / Server 2008 | ⚠️ | ⚠️ | - | - | - |
| Windows 7 / Server 2008 R2 | ⚠️ | ⚠️ | - | - | - |
| Windows 8 / 8.1 / RT | ⚠️ | ⚠️ | - | ✅ | - |
| Windows 10 | ✅ | ✅ | - | ✅ | ✅ |
| Windows 11 | ✅ | ✅ | - | - | ✅ |

> 说明：`✅` 表示当前发行物明确支持；`⚠️` 表示代码层面已尽量兼容，但 modern MSVC 产物的实际最低支持系统仍以工具链与运行库验证结果为准。Windows 2000 / XP / 2003 请始终使用 `MikaBooM_x86_win2000.exe`。

## 许可证

本项目遵循 [AGPLv3](LICENSE) 协议。

## Issues

如果有任何问题，请在 [GitHub Issue](https://github.com/MakotoArai-CN/MikaBooM_CPP/issues) 页面提交。

## Changelog

- Version 1.0.5: 统一 modern 构建并补回 legacy x86 发行物
  - modern 架构统一为 x86 / x64 / ARM / ARM64 输出
  - Windows 2000 / XP / 2003 改由独立 `MikaBooM_x86_win2000.exe` 承诺支持
  - 按架构设置 PE subsystem version（x86=5.00, x64=5.02, arm/arm64=6.02/10.00）
  - 修复 `anti_detect.h` 中 `GlobalMemoryStatusEx` 直接调用导致 Win2000 无法加载的 bug
  - updater 按架构和系统版本选择发行资产
  - CI 同时构建 modern 架构与 legacy x86，并对 legacy x86 执行导入表审计
- Version 1.0.4: 修复控制台显示 BUG
  - 修复托盘图标隐藏/显示窗口问题
  - 优化控制台初始化逻辑
  - 改用 llvm-mingw 编译
  - 修复内存泄漏问题
  - 增强免杀能力
- Version 1.0.2: 多架构支持 + 安全优化
  - 新增多架构编译支持 (x86/x64/ARM/ARM64)
  - 杀毒软件免杀优化
  - Windows 全版本 API 优化
  - 修复托盘图标显示问题
  - 提升稳定性和兼容性
- Version 1.0.1: 优化代码结构，修复 BUG
  - 优化代码结构，提高可读性
  - 修复 CPU 和内存阈值设置问题
  - 修复托盘图标显示问题
  - 支持 GitHub 仓库更新
- Version 1.0.0: Initial release
  - 初始测试版本
  - 支持 Windows 2000 到 Windows 11
