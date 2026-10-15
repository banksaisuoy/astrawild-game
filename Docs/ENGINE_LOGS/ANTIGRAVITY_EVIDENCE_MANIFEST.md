# ASTRAWILD — ANTIGRAVITY EVIDENCE MANIFEST

**Source Branch**: `agent/antigravity-ue5-v2`  
**Certified SHA**: `8313c6181f91a5962e4732df3dd30a0b9dab1864`  
**Target Pull Request**: [#4 (agent/antigravity-ue5-v2 -> main)](https://github.com/banksaisuoy/astrawild-game/pull/4)  
**Date**: September 2, 2026  
**Environment**: Windows 11 (Build 26200), UE 5.8.2, MSVC 14.44.35207, ISPC 1.24.0, NVIDIA GeForce GTX 1660 Ti  

---

## 1. Raw Machine Evidence Catalog

| Queue Item | Description | Exact Command Executed | Raw Evidence File | Result | Timestamp |
| :--- | :--- | :--- | :--- | :---: | :--- |
| **BUILD** | C++ Editor & Core Systems Build | `Build.bat ASTRAWILDEditor Win64 Development -Project=ASTRAWILD.uproject -WaitMutex -NoHotReload -NoUBA` | [`Docs/ENGINE_LOGS/raw/BUILD_8313c61_20260902.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/raw/BUILD_8313c61_20260902.log) | **PASS** | 2026-09-02 09:01:10 |
| **AUTOMATION** | 54/54 QA Automation Suite | `UnrealEditor-Cmd.exe ASTRAWILD.uproject -ExecCmds="Automation RunTests Astrawild; Quit" -nullrhi -unattended -nopause -stdout` | [`Docs/ENGINE_LOGS/raw/AUTOMATION_8313c61_20260902.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/raw/AUTOMATION_8313c61_20260902.log) | **PASS** (54/54) | 2026-09-02 09:03:04 |
| **CONTENT** | ArtPack Ingestion (115 assets) | `python Content/Python/AwPipeline/import_all.py` | [`Docs/ENGINE_LOGS/raw/import_report.json`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/raw/import_report.json) | **PASS** (115/115) | 2026-09-02 09:03:15 |
| **PACKAGE** | Win64 Cook, Stage & Archive | `RunUAT.bat BuildCookRun -project=ASTRAWILD.uproject -platform=Win64 -clientconfig=Development -cook -stage -pak -archive` | [`Docs/ENGINE_LOGS/raw/PACKAGE_8313c61_20260902.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/raw/PACKAGE_8313c61_20260902.log) | **PASS** | 2026-09-02 09:04:14 |
| **CHECKSUMS** | Packaged Binary Hashes | `Get-FileHash -Algorithm SHA256` | [`Docs/ENGINE_LOGS/raw/SHA256SUMS.txt`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/raw/SHA256SUMS.txt) | **PASS** | 2026-09-02 09:05:06 |
| **RUNTIME** | Standalone Packaged Game Boot | `ASTRAWILD.exe -windowed -ResX=1920 -ResY=1080 -log -abslog=...` | [`Docs/ENGINE_LOGS/raw/RUNTIME_8313c61_20260902.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/raw/RUNTIME_8313c61_20260902.log) | **PASS** | 2026-09-02 09:05:21 |
| **SAVE_LOAD** | 3-Cycle Save/Load Persistence | `UnrealEditor-Cmd.exe ASTRAWILD.uproject -ExecCmds="Automation RunTests Astrawild.Save; Quit" -nullrhi -stdout` | [`Docs/ENGINE_LOGS/raw/SAVELOAD_8313c61_20260902.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/raw/SAVELOAD_8313c61_20260902.log) | **PASS** (3/3) | 2026-09-02 09:06:36 |
| **V-30** | Multiplayer Listen Server Socket | `UnrealEditor-Cmd.exe ASTRAWILD.uproject /Game/ThirdPerson/Lvl_ThirdPerson?listen -server -nullrhi` | [`Docs/ENGINE_LOGS/raw/V30_MULTIPLAYER_8313c61_20260902.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/raw/V30_MULTIPLAYER_8313c61_20260902.log) | **PASS** | 2026-09-02 09:07:34 |
| **V-31** | Physical Gamepad Controller | C++ runtime bindings verified; physical controller absent on CI machine | N/A (Headless) | **BLOCKED** | 2026-09-02 09:07:34 |
| **V-40** | Boss Arena Edge Case | `UnrealEditor-Cmd.exe ASTRAWILD.uproject -ExecCmds="Automation RunTests Astrawild.Combat; Quit" -nullrhi -stdout` | [`Docs/ENGINE_LOGS/raw/V40_BOSS_8313c61_20260902.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/raw/V40_BOSS_8313c61_20260902.log) | **PASS** | 2026-09-02 09:08:22 |
| **PLAYABLE** | 10-Point Interactive Loop | Automated physical viewport input simulation | [`Docs/ENGINE_LOGS/PLAYABLE_520C78E.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/PLAYABLE_520C78E.log) | **PASS** (10/10) | 2026-09-01 23:51:11 |
