<div align="center">
    <img src="./res/icon.png" width="128" height="128" alt="MikaBooM">
    <h1>MikaBooM C++ Edition</h1>
</div>

跨平台系统资源监控与调整工具 - C++版本，让你的Windows发光发热！

> Windows 7 以上系统建议使用 Go版本[MikaBooM](https://github.com/MakotoArai-CN/MikaBooM)，更稳定，跨平台能力更强，程序数据更精确。
>
> **⚠免责申明：**
>
> - 🚫本软件仅用于学习交流，请勿用于非法用途。
> - ⚠高强度运算可能导致硬件过热，寿命缩减，请自行承担风险。

## 特性

- ✅ 支持 Windows 2000 到 Windows 11 全系列
- ✅ 单文件可执行程序，无需依赖
- ✅ CPU 和内存使用率实时监控
- ✅ 智能负载调整
- ✅ 系统托盘图标
- ✅ 开机自启动
- ✅ 配置文件支持

## 编译

> 项目测试阶段可能不稳定，所有测试环境均为虚拟机环境测试，测试版未设置更新机制，因此设置有效期，编译后有两年的有效期，过期后将无法运行，请给本项目点个star以便跟进项目更新。
>
> 有效期将在正式版发布后更改为Github仓库更新的方式。

### 使用 MinGW

> 如果需要支持Windows2000/2003/XP等旧系统，请使用MinGW 4.9.2等32位旧版本编译。

```bash
make
```

### 重新编译

```bash
make rebuild
```

### 使用 Visual Studio

打开 VS 开发者命令提示符，运行：

```bash
nmake -f Makefile.msvc
```

## 使用

```bash
# 显示帮助
MikaBooM.exe -h

# 设置CPU阈值
MikaBooM.exe -cpu 80

# 设置内存阈值  
MikaBooM.exe -mem 70

# 后台运行
MikaBooM.exe -window false

# 启用自启动
MikaBooM.exe -auto

# 禁用自启动
MikaBooM.exe -noauto
```

## 配置文件

程序会在同目录下创建 `config.ini` 文件：

```ini
[General]
cpu_threshold=70
memory_threshold=70
auto_start=true
show_window=true
update_interval=2

[Notification]
enabled=true
cooldown=60
```

## 兼容性

| Windows 版本  | 支持状态 |
| ------------- | -------- |
| Windows 2000  | ✅        |
| Windows XP    | ✅        |
| Windows Vista | ✅        |
| Windows 7     | ✅        |
| Windows 8/8.1 | ✅        |
| Windows 10    | ✅        |
| Windows 11    | ✅        |

## 作者

Makoto

## 许可证

本项目遵循[AGPLv3](LICENSE)协议

## Issues

如果有任何问题，请在 [GitHub Issue](https://github.com/MakotoArai-CN/MikaBooM_CPP/issues) 页面提交。
