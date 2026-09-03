# ASTRAWILD — AUTHORITATIVE MASTER CONTROL (CANONICAL SINGLE SOURCE OF TRUTH)

**Canonical Document Version**: 2.0 (Authoritative)  
**Host Target**: Windows 11 / Unreal Engine 5.8.2 / MSVC 14.44 / NVIDIA GeForce GTX 1660 Ti (6 GB VRAM)  
**Active Integration Branch**: `agent/antigravity-ue5-v2`  
**Latest Open PR**: [**PR #4: fix(player): restore real playable input, camera controls, and character presentation**](https://github.com/banksaisuoy/astrawild-game/pull/4)  
**Latest Verified SHA**: `c65d734`  
**Test Suite State**: 57 / 57 PASS (100% Green)  
**Last Updated**: September 2, 2026  

---

> [!IMPORTANT]
> **CANONICAL DOCUMENT DECLARATION**:  
> This file (`Docs/ASTRAWILD_MASTER_CONTROL.md`) is the **SOLE CANONICAL SOURCE OF TRUTH** for the entire ASTRAWILD project.  
> The following historical roadmap files are hereby declared **SUPERSEDED & ARCHIVED**:  
> - `Docs/ASTRAWILD_PROJECT_MASTER_PLAN_v1.md` (ARCHIVED)  
> - `Docs/ASTRAWILD_PRODUCTION_V2_MASTER_PLAN.md` (ARCHIVED)  
> - `Docs/ASTRAWILD_PRODUCTION_MASTER_PLAN_V2.md` (ARCHIVED)  
> - `Docs/ASTRAWILD_PLAYABLE_BUILD_MASTER_PLAN_V4.md` (ARCHIVED)  
> - `Docs/ASTRAWILD_PROJECT_MASTER_STATUS_AND_GLM_HANDOFF.md` (SUPERSEDED by MASTER_CONTROL)  
> No agent shall create or follow alternate roadmaps.

---

## 1. Permanent Agent Roles & Boundaries

| Agent | Permanent Role | Responsibilities | Output Rules |
| :--- | :--- | :--- | :--- |
| **🧠 GLM** | **Lead Programmer (Systems & Balance)** | C++, Architecture, Save/Load, Quests, AI logic, Economy, Monster Balance curves, Math models, Tests | Long-session deep focus; produces clean C++ or DataTables |
| **🎨 Qwen** | **Technical Art & 3D Specialist** | Materials (`M_Master_Surface`, `MI_*`), Meshes, Animations, Niagara VFX, UI layout, Shaders, Optimization | Atomic tasks (10–30 min sessions); commits modular visual assets |
| **🤖 Antigravity** | **Local UE5 Integration & QA Lead** | Live Windows Engine Workspace (`E:\AstrawildGame`), Pull, Build (MSVC), 57/57 Tests, Playtest, Package, Measure, Commit/Push | High-frequency verifier; produces raw machine evidence |
| **🔍 Sonnet** | **Independent Auditor** | Objective evidence audit, diff analysis, SHA cross-check, documentation integrity | Fact-checker; challenges unsubstantiated claims |

---

## 2. Standardized Task Status Lifecycle

All tasks in this project strictly follow this progression. **No loose status words allowed.**

```text
PLANNED ──> IN_PROGRESS ──> IMPLEMENTED ──> BUILT ──> TESTED ──> UE5_VERIFIED ──> ACCEPTED
                                                                         │
                                                                         └──> BLOCKED
```

- **PLANNED**: Task defined in Master Control with clear acceptance criteria.
- **IN_PROGRESS**: Assigned agent is currently executing the task.
- **IMPLEMENTED**: Code / Asset / Data written by the authoring agent.
- **BUILT**: Successfully compiled in UE 5.8.2 toolchain with 0 errors.
- **TESTED**: Automated tests passed (e.g. 57/57 QA suite).
- **UE5_VERIFIED**: Physically verified in live viewport / packaged runtime with machine log evidence.
- **ACCEPTED**: Audited by Sonnet / Merged into `main`.
- **BLOCKED**: Halted due to missing physical hardware or external prerequisite.

---

## 3. High-Level Game Architecture & Vision

```text
GAME VISION
    │
    ▼
FINAL GAMEPLAY LOOP: Boot ──> Explore ──> Gather ──> Craft ──> Fight ──> Capture ──> Build Base ──> Research ──> Skiff ──> Dungeons ──> Endgame
    │
    ▼
WORLD STRUCTURE: 12 Distinct Biome Zones (Starting Zone: Dawn Fields) ──> Ancient Observatories ──> Echo Habitats
    │
    ▼
CORE PROGRESSION:
    • First Light Quest ──> Taming First Echo (Bastionbeetle) ──> Crafting Scrap Rifle & Plasma Carbine
    • Tech Tree Progression (Basic Crafting ──> Power Grid ──> Advanced Metallurgy ──> Quantum Navigation)
    • Skiff Land Vehicle Exploration ──> Phased Boss Encounters ──> Final Ascendant Encounter
```

---

## 4. Master 7-Stage Roadmap (Locked Canon)

### STAGE 0 — CONTROL & TRUTH RECOVERY (Active)
- **Objective**: Reconcile master docs, establish MASTER_CONTROL, resolve GAP-A (commit 115 ArtPack `.uasset` files to Git LFS), merge PR #4.
- **Gate**: PR #4 merged into `main`; fresh clone outside local machine receives full 115 visual assets without procedural fallback.

### STAGE 1 — GAMEPLAY HARDENING (GLM Lead)
- **Objective**: Harden Save/Load V4, Quest State Machine, Inventory weight/stack contracts, Echo AI behavior trees, Combat hit-boxes, Dedicated Server replication.
- **Gate**: 65+ unit tests passing; zero desync in listen server test.

### STAGE 2 — VISUAL PRODUCTION (Qwen Lead)
- **Objective**: Material Instances (`MI_*`) on `M_Master_Surface`, Niagara VFX for weapon fire and creature skills, UI HUD polish.
- **Gate**: Dawn Fields terrain rendered with PBR textures (no whiteout); Survivor Exosuit and 3 starter Echoes rendered with emissive glow.

### STAGE 3 — WORLD & CONTENT EXPANSION (GLM + Qwen + Antigravity)
- **Objective**: 12 Biomes populating with Flora, Minerals, POIs, NPCs, Dialogue trees, and Skiff traversal.
- **Gate**: Seamless transition between Dawn Fields and adjacent zones at $\ge 60$ FPS.

### STAGE 4 — ENDGAME & BOSS ENCOUNTERS
- **Objective**: 5 Ancient Observatories, Phased Boss scaling, Late-game Singularity Cannon, Narrative climax.
- **Gate**: Full boss combat loop playable from encounter trigger to loot drop.

### STAGE 5 — GOLDEN PATH PLAYTHROUGH
- **Objective**: Complete end-to-end playthrough from New Game to Final Boss without debug cheats.
- **Gate**: Full playthrough log recorded; zero game-breaking softlocks.

### STAGE 6 — RELEASE QA & PACKAGING
- **Objective**: Standalone Win64 release build, physical gamepad validation (V-31), memory leak profiling, crash reporter setup.
- **Gate**: Release Candidate executable certified.

---

## 5. Live Task Registry

| TASK ID | Task Description | Owner | Status | Dependency | Branch | Tested Commit | Verified By | Blocker / Notes |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: | :--- |
| **CTL-001** | Canonical Master Control Document | Antigravity | **ACCEPTED** | None | `agent/antigravity-ue5-v2` | `c65d734` | Antigravity | Single source of truth active |
| **CORE-001** | Enhanced Input Lifecycle Fix (10/10) | Antigravity | **UE5_VERIFIED** | None | `agent/antigravity-ue5-v2` | `8313c61` | Antigravity | WASD, Look, Jump, Sprint, Interact, Attack live |
| **CORE-002** | GLM Source Hardening (SH-01..SH-04) | GLM / Antigravity | **UE5_VERIFIED** | CORE-001 | `agent/antigravity-ue5-v2` | `c65d734` | Antigravity | 57/57 tests PASS (100% green) |
| **ART-001** | Ingest 115 ArtPack Raw Assets | Qwen / Antigravity | **BUILT** | None | `agent/antigravity-ue5-v2` | `8313c61` | Antigravity | Assets on local disk; needs Git LFS commit |
| **M0-001** | Truth Recovery (Commit .uasset to Git LFS) | Antigravity | **IN_PROGRESS** | ART-001 | `agent/antigravity-ue5-v2` | Pending | Antigravity | Resolves GAP-A for fresh checkouts |
| **VIS-001** | Dawn Fields PBR Landscape Material | Qwen | **PLANNED** | M0-001 | `qwen/visual-dawnfields` | Pending | Antigravity | Fixes terrain whiteout on unbaked maps |
| **VIS-002** | Survivor Exosuit Material Instance (`MI_Survivor`) | Qwen | **PLANNED** | M0-001 | `qwen/visual-survivor` | Pending | Antigravity | Emissive visor & PBR roughness tuning |
| **VIS-003** | Starter Echoes Materials (3 Echoes) | Qwen | **PLANNED** | M0-001 | `qwen/visual-echoes` | Pending | Antigravity | Bastionbeetle, Terraquill, Cindermule |
| **GAME-001** | Dawn Fields Balance Table & Quest Flow | GLM | **PLANNED** | CORE-002 | `glm/gameplay-dawnfields` | Pending | Antigravity | Monster stats, drops, `Quest_FirstLight` |
| **QA-031** | Physical Gamepad Controller Actuation (V-31) | Antigravity | **BLOCKED** | Hardware | `agent/antigravity-ue5-v2` | Pending | Antigravity | Blocked until physical controller attached |
| **REL-001** | Standalone Packaged Executable (`ASTRAWILD.exe`) | Antigravity | **UE5_VERIFIED** | CORE-001 | `agent/antigravity-ue5-v2` | `8313c61` | Antigravity | 320 MB standalone EXE verified playable |

---

## 6. Machine Evidence Log Catalog

- `Docs/ENGINE_LOGS/raw/BUILD_8313c61_20260902.log` — MSVC C++ Build Output (0 errors)
- `Docs/ENGINE_LOGS/raw/AUTOMATION_8313c61_20260902.log` — 57/57 Automation Test Suite Output
- `Docs/ENGINE_LOGS/raw/import_report.json` — 115/115 ArtPack Asset Ingestion Record
- `Docs/ENGINE_LOGS/raw/PACKAGE_8313c61_20260902.log` — Cook & Package Stage Output
- `Docs/ENGINE_LOGS/raw/RUNTIME_8313c61_20260902.log` — Standalone Packaged EXE Runtime Output
- `Docs/ENGINE_LOGS/raw/SAVELOAD_8313c61_20260902.log` — 3-Cycle Persistence Verification Output
- `Docs/ENGINE_LOGS/PLAYABLE_520C78E.log` — Physical Player Input Runtime Output
