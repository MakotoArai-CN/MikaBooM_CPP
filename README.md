<div align="center">
    <img src="./res/icon.png" width="128" height="128" alt="MikaBooM">
    <h1>MikaBooM C++ Edition</h1>
</div>

Windows系统资源监控与调整工具 - C++版本，让你的Windows发光发热！

> Windows 7 以上系统建议使用 Go版本[MikaBooM](https://github.com/MakotoArai-CN/MikaBooM)，更稳定，跨平台能力更强，程序数据更精确。

> **⚠免责申明：**
>
> - 🚫本软件仅用于学习交流，请勿用于非法用途。
> - ⚠高强度运算可能导致硬件过热，寿命缩减，请自行承担风险。

## 特性

- ✅ 支持 Windows 2000 到 Windows 11 全系列
- ✅ 支持多架构：x86、x64、ARM、ARM64
- ✅ 单文件可执行程序，无需依赖
- ✅ CPU 和内存使用率实时监控
- ✅ 智能负载调整
- ✅ 系统托盘图标
- ✅ 开机自启动
- ✅ 配置文件支持
- ✅ 杀毒软件免杀优化

## 编译

### 编译所有架构
```bash
make all-arch
# 或者
# make
```

### 编译特定架构
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
- `MikaBooM_x86.exe` - 32位 x86 版本
- `MikaBooM_x64.exe` - 64位 x64 版本
- `MikaBooM_arm.exe` - ARM 32位版本
- `MikaBooM_arm64.exe` - ARM 64位版本

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
```

## 兼容性

| Windows 版本  | x86 | x64 | ARM | ARM64 |
| ------------- | --- | --- | --- | ----- |
| Windows 2000  | ✅  | ❌  | ❌  | ❌    |
| Windows XP    | ✅  | ✅  | ❌  | ❌    |
| Windows Vista | ✅  | ✅  | ❌  | ❌    |
| Windows 7     | ✅  | ✅  | ❌  | ❌    |
| Windows 8/8.1 | ✅  | ✅  | ✅  | ❌    |
| Windows 10    | ✅  | ✅  | ✅  | ✅    |
| Windows 11    | ✅  | ✅  | ✅  | ✅    |

## 安全性说明

本程序采用以下技术确保安全运行：
- ✅ API动态加载
- ✅ 字符串加密
- ✅ 延迟执行
- ✅ 内存安全管理
- ✅ 无恶意行为

## 作者

Makoto

## 许可证

本项目遵循[AGPLv3](LICENSE)协议

## Issues

如果有任何问题，请在 [GitHub Issue](https://github.com/MakotoArai-CN/MikaBooM_CPP/issues) 页面提交。

## Changelog

- Version 1.0.3: 修复控制台显示BUG
  - ✅ 修复托盘图标隐藏/显示窗口问题
  - ✅ 优化控制台初始化逻辑
  - ✅ 改用llvm-mingw编译

- Version 1.0.2: 多架构支持 + 安全优化
  - ✅ 新增多架构编译支持 (x86/x64/ARM/ARM64)
  - ✅ 杀毒软件免杀优化
  - ✅ Windows全版本API优化
  - ✅ 修复托盘图标显示问题
  - ✅ 提升稳定性和兼容性

- Version 1.0.1: 优化代码结构，修复BUG
  - 优化代码结构，提高可读性
  - 修复CPU和内存阈值设置问题
  - 修复托盘图标显示问题
  - 支持Github仓库更新

- Version 1.0.0: Initial release
  - 初始测试版本
  - 支持Windows 2000 到 Windows 11
