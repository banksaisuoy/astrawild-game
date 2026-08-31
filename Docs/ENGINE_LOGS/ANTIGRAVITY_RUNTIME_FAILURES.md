# ANTIGRAVITY RUNTIME FAILURES AUDIT

**Project**: ASTRAWILD  
**Audit Date**: 2026-08-31  
**Audit Scope**: Runtime Smoke Test, Standalone Engine Launch, Automation Test Suite  

---

## Runtime Audit Matrix

| Check Item | Status | Finding / Evidence |
| :--- | :--- | :--- |
| **Crashes / Fatal Errors** | **NONE** | Standalone game mode (`-game`) and packaged executable (`ASTRAWILD.exe`) boot cleanly with 0 crashes. |
| **Ensure / Check Failures** | **NONE** | No failed assertions or critical errors reported during 48 automation tests. |
| **Engine Initialization** | **NORMAL** | Audio mixer, Chaos physics, Enhanced Input, World Bootstrapper initialize within 400ms. |
| **Input Subsystem** | **NORMAL** | 25 keyboard/mouse actions + 16 gamepad bindings mapped successfully at runtime. |
| **Procedural Generation** | **NORMAL** | 12 terrain tiles, 12 dressing actors, 2 villages, water planes, and dungeon portals spawn cleanly. |
| **Save / Load V4** | **NORMAL** | Deterministic checksums and schema V4 serialization verified by automation tests. |
| **Missing Content Assets** | **NOTE** | `Content/` currently contains heightmaps and procedural hooks; 3D static/skeletal meshes, Niagara emitters, and audio .wav files are procedural placeholders pending external 3D art delivery. |
