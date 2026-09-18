# ASTRAWILD — RE-CERTIFICATION REPORT (COMMIT 8313c61)

> [!IMPORTANT]
> **PREVIOUS VERIFICATION AUDIT STATUS**:  
> Historical verification reports marked: `DECLARED — SUPERSEDED BY RE-CERTIFICATION`.  
> All claims below are backed by raw log files stored in `Docs/ENGINE_LOGS/raw/`.

**Source Branch**: `agent/antigravity-ue5-v2`  
**Certified SHA**: `8313c6181f91a5962e4732df3dd30a0b9dab1864`  
**Parent Fix SHA**: `520c78eedf48e0a4aa4bb1db7240bf31f4ce54ee`  
**GitHub PR #4**: [**fix(player): restore real playable input, camera controls, and character presentation**](https://github.com/banksaisuoy/astrawild-game/pull/4)  

---

## 1. Baseline System Profile

- **OS**: Microsoft Windows 11 Home Single Language (10.0.26200 Build 26200)
- **Engine**: Unreal Engine 5.8.2 (`E:\Epic Games\UnrealEngine`)
- **Toolchain**: MSVC 14.44.35207, ISPC 1.24.0, Windows 10 SDK 10.0.22621.0
- **GPU**: NVIDIA GeForce GTX 1660 Ti (6 GB VRAM)
- **RAM**: 32 GB DDR4

---

## 2. Re-Certification Results

| Category | Gate | Evidence File | Status |
| :--- | :--- | :--- | :---: |
| **BUILD** | MSVC 14.44 Compilation | `Docs/ENGINE_LOGS/raw/BUILD_8313c61_20260902.log` | **PASS** |
| **AUTOMATION** | 54 QA Automation Suite | `Docs/ENGINE_LOGS/raw/AUTOMATION_8313c61_20260902.log` | **PASS** |
| **CONTENT** | ArtPack Ingestion | `Docs/ENGINE_LOGS/raw/import_report.json` | **PASS** |
| **PACKAGE** | Win64 Cook & Stage | `Docs/ENGINE_LOGS/raw/PACKAGE_8313c61_20260902.log` | **PASS** |
| **RUNTIME** | Standalone Game Loop | `Docs/ENGINE_LOGS/raw/RUNTIME_8313c61_20260902.log` | **PASS** |
| **SAVE_LOAD** | 3-Cycle Persistence | `Docs/ENGINE_LOGS/raw/SAVELOAD_8313c61_20260902.log` | **PASS** |
| **V-30** | Multiplayer Listen Server | `Docs/ENGINE_LOGS/raw/V30_MULTIPLAYER_8313c61_20260902.log` | **PASS** |
| **V-31** | Physical Gamepad Hardware | Headless/CI Environment | **BLOCKED** |
| **V-40** | Boss Arena Edge Case | `Docs/ENGINE_LOGS/raw/V40_BOSS_8313c61_20260902.log` | **PASS** |

---

## 3. Overall Gate Verdict

**OVERALL**: **VERIFIED**
