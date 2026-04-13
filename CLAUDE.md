# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and run

### GNU Make / MinGW
- `make` — build the default target from [Makefile](Makefile)
- `make clean` — remove `build/` and generated `.exe` files
- `make rebuild` — clean + rebuild (currently points at `auto-build`; see notes below)
- `make check` — print detected compiler / resource compiler support
- `make check-deps` — inspect DLL dependencies of built executables

### MSVC / NMake
Prefer this path for explicit architecture selection; [Makefile.msvc](Makefile.msvc) is the only build file in the repo that currently defines per-arch targets.

- `nmake /F Makefile.msvc` — build for the auto-detected architecture
- `nmake /F Makefile.msvc ARCH=x64`
- `nmake /F Makefile.msvc ARCH=x86`
- `nmake /F Makefile.msvc ARCH=arm`
- `nmake /F Makefile.msvc ARCH=arm64`
- `nmake /F Makefile.msvc all-arch` — build all four architectures
- `nmake /F Makefile.msvc clean`
- `nmake /F Makefile.msvc rebuild`
- `nmake /F Makefile.msvc check`
- `nmake /F Makefile.msvc check-deps`
- `nmake /F Makefile.msvc run`

### Runtime CLI
Use a built executable directly, e.g. `MikaBooM_x64.exe`.

- `MikaBooM_x64.exe -h` — help
- `MikaBooM_x64.exe -v` — version / system info
- `MikaBooM_x64.exe -cpu 80` — set CPU threshold
- `MikaBooM_x64.exe -mem 70` — set memory threshold
- `MikaBooM_x64.exe -window false` — run without console window
- `MikaBooM_x64.exe -auto` / `-noauto` — enable / disable auto-start
- `MikaBooM_x64.exe -update` — check/download/apply latest release
- `MikaBooM_x64.exe -c C:\path\to\config.ini` — use a custom config file

### Tests / lint
No dedicated test suite, single-test runner, or lint configuration was found in the repository.

## High-level architecture

### App model
This is a native Win32 console application with a tray icon. The main control flow lives in [src/main.cpp](src/main.cpp):

1. Startup validation/initialization runs before anything else.
2. [ConfigManager](src/core/config_manager.h) loads `config.ini` from the executable directory by default.
3. [ConsoleUtils](src/utils/console_utils.h) configures console encoding and prints the banner/help/status output.
4. [Version](src/utils/version.h) / [Updater](src/utils/updater.h) optionally check GitHub releases for updates.
5. [ResourceMonitor](src/core/resource_monitor.h) samples current CPU and memory usage.
6. [CPUWorker](src/core/cpu_worker.h) and [MemoryWorker](src/core/memory_worker.h) are created only when the version is still valid.
7. [SystemTray](src/platform/system_tray.h) creates a hidden message window and tray icon.
8. `MonitorLoop()` polls usage, applies hysteresis + multi-cycle confirmation, starts/stops workers, updates the tray tooltip, and prints live status.

### Core subsystems
- [src/core/resource_monitor.cpp](src/core/resource_monitor.cpp) abstracts resource measurement. CPU sampling prefers `GetSystemTimes`; x86/x64 builds can fall back to PDH; memory queries go through [src/platform/system_compat.h](src/platform/system_compat.h) for old/new Windows compatibility.
- [src/core/cpu_worker.cpp](src/core/cpu_worker.cpp) creates one worker thread per processor and simulates CPU load with synthetic math work. `AdjustLoad()` changes the shared `intensity` value rather than recreating threads.
- [src/core/memory_worker.cpp](src/core/memory_worker.cpp) uses a single background thread that grows/shrinks committed memory toward a target. Allocation size and adjustment step are derived from total RAM so behavior scales with the machine.
- [src/core/config_manager.cpp](src/core/config_manager.cpp) owns persisted thresholds, update interval, window visibility, auto-start, and update checks. CLI flags mutate config state and shutdown writes the file back.

### Platform integration
- [src/platform/system_tray.cpp](src/platform/system_tray.cpp) owns the tray icon, tooltip, right-click menu, console show/hide behavior, and auto-start toggle.
- [src/platform/autostart.cpp](src/platform/autostart.cpp) writes `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`.
- [res/resource.rc](res/resource.rc) embeds the icon and Windows version metadata.

### Utility layer
- [src/utils/console_utils.cpp](src/utils/console_utils.cpp) splits behavior between UTF-8-capable Windows 7+ consoles and legacy CP437 consoles.
- [src/utils/version.cpp](src/utils/version.cpp) stores the app version/expiry metadata and calls the GitHub Releases API for update discovery.
- [src/utils/updater.cpp](src/utils/updater.cpp) downloads the new executable into memory, writes a temporary batch script, replaces the current exe, and relaunches.
- [src/utils/system_info.h](src/utils/system_info.h) centralizes OS version detection used by console, tray, and compatibility logic.
- [src/utils/anti_detect.h](src/utils/anti_detect.h) is referenced from startup and worker/resource code for initialization guards, timing jitter, and dynamic API lookup; changes there affect startup behavior broadly.

## Important repo-specific notes
- README build instructions and the current GNU [Makefile](Makefile) are out of sync: README documents `make x86/x64/arm/arm64`, but those targets are not defined in the current file.
- The GNU [Makefile](Makefile) has `rebuild: clean auto-build`, but no `auto-build` target is present. If you touch build tooling, reconcile README + `Makefile` + `Makefile.msvc` together.
- App version constants live in two places: [src/utils/version.h](src/utils/version.h) and [res/resource.rc](res/resource.rc). They must stay in sync — the CI `verify-version` job enforces this.
- Each architecture outputs a single universal exe (no separate legacy builds). PE subsystem version is set per-arch in `Makefile.msvc` (x86=5.00, x64=5.02, arm=6.02, arm64=10.00).
- The project is heavily optimized for old Windows compatibility (`_WIN32_WINNT=0x0500`, legacy `NOTIFYICONDATA`, dynamic API fallback). When changing platform code, check behavior on both legacy and modern Windows paths instead of assuming Windows 10+ only.
- All Vista+ APIs must be loaded dynamically via `GetProcAddress` — never link directly. See `system_compat.h`, `resource_monitor.cpp`, and `anti_detect.h` for the established pattern.
