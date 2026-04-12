<!-- markdownlint-disable MD033 MD041 -->
<div align="center">
    <img src="./res/icon.png" width="128" height="128" alt="MikaBooM">
    <h1>MikaBooM C++ Edition</h1>
</div>
<!-- markdownlint-enable MD033 MD041 -->

Windows系统资源监控与调整工具 - C++版本，让你的Windows发光发热！

> Windows 7 以上系统建议使用 Go 版本 [MikaBooM](https://github.com/MakotoArai-CN/MikaBooM)，更稳定、跨平台能力更强、程序数据更精确。
>
> **⚠ 免责声明**
>
> - 本软件仅用于学习交流，请勿用于非法用途。
> - 高强度运算可能导致硬件过热、寿命缩减，请自行承担风险。

## 特性

- 支持 Windows 2000 到 Windows 11 全系列（需使用对应工具链构建）
- 支持多架构：x86、x64、ARM、ARM64
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

### 使用 MinGW

> 如果需要支持 Windows 2000/2003/XP 等旧系统，请使用 MinGW 4.9.2 等 32 位旧版本编译。

```bash
# 32位 x86
make x86

# 64位 x64
make x64

# ARM 32位
make arm

# ARM 64位
make arm64
```

### 重新编译

```bash
make rebuild
```

## 输出文件

编译后会生成以下文件：

- `MikaBooM_x86.exe` - 32 位 x86 版本
- `MikaBooM_x64.exe` - 64 位 x64 版本
- `MikaBooM_arm.exe` - ARM 32 位版本
- `MikaBooM_arm64.exe` - ARM 64 位版本

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
- GitHub Hosted Runner 默认构建现代 Windows `x86/x64`
- `arm` 与 `arm64` 通过独立 self-hosted runner 通道构建并发布
- 可选 `legacy-x86` 自托管发布通道用于 Windows 2000/XP 兼容包
- Release 仅发布实际构建成功的 `.exe` 产物，不上传 README 等说明文件
- 发布版本以 `src/utils/version.h` 为准，并校验 `res/resource.rc` 是否同步

## 兼容性

> 注意：下面的 Windows 兼容性建立在 `legacy-x86` 构建之上；现代 `x64/UCRT` 构建不代表能直接运行在 Windows 2000/XP 上。

| Windows 版本 | 支持状态 |
| --- | --- |
| Windows 2000 | ✅ |
| Windows XP | ✅ |
| Windows Vista | ✅ |
| Windows 2003 | ✅ |
| Windows 2008 | ✅ |
| Windows 2012 | ✅ |
| Windows 2016 | ✅ |
| Windows 7 | ✅ |
| Windows 8/8.1 | ✅ |
| Windows 10 | ✅ |
| Windows 11 | ✅ |

## 许可证

本项目遵循 [AGPLv3](LICENSE) 协议。

## Issues

如果有任何问题，请在 [GitHub Issue](https://github.com/MakotoArai-CN/MikaBooM_CPP/issues) 页面提交。

## Changelog

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
