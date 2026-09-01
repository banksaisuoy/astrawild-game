# ASTRAWILD — MASTER AI AGENT HANDOFF & SYNCHRONIZATION GUIDE

> **ATTENTION ALL AI AGENTS (GLM 5.3, Qwen, Antigravity, Claude, Codex, etc.)**:  
> Read this document FIRST before writing or modifying any code in this repository.  
> This file contains the complete, up-to-date state of the project, verified baseline, architecture, and exact task assignments.

---

## 1. Project Overview & Verified Baseline

- **Engine Version**: Unreal Engine 5.8.2 (`E:\Epic Games\UnrealEngine`)
- **Host System**: Windows 11 / MSVC 14.44.35207 / NVIDIA GeForce GTX 1660 Ti (6 GB VRAM)
- **Active Working Branch**: `agent/antigravity-ue5-v2`
- **Main Repository**: `banksaisuoy/astrawild-game`
- **Current Status**: **100% GREEN & FULLY PLAYABLE**
  - **C++ Build**: `ASTRAWILDEditor Win64 Development` compiles with **0 errors** (76.62s).
  - **Automation Test Suite**: **54 / 54 tests PASSED (100% Green)** via `Test.ps1` in 100.40s.
  - **ArtPack Ingestion**: **115 / 115 .uasset files verified** in `Saved/AwPipelineReport/import_report.json` (0 missing).
  - **Packaged Executable**: Cooked 493 packages into monolithic Win64 executable (`E:\Astrawild_Packaged\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe`).
  - **Runtime Performance**: **90 – 130 FPS** at 1080p on GTX 1660 Ti across 6 benchmark regions.
  - **Multiplayer Replication (V-30)**: 2-Client Listen Server test verified (`Test_V30_Replication.ps1`).
  - **Gamepad Support (V-31)**: 16-button full dual-stick mapping verified via Enhanced Input.
  - **Boss Encounter (V-40)**: Multi-phase Underlight Warden (3 phases, telegraphs, weak-point, adds) verified.
  - **Save/Load System**: Schema v4 with 3-cycle roundtrip verification (`ASTRAWILD.Save.SchemaV4`).

---

## 2. Workspace & Asset Layout

```text
E:\AstrawildGame\
├── Source\AstrawildCore\         # C++ Core Module (45 headers + cpp implementations)
│   ├── Public\                  # Classes: Player, Echo, Building, Combat, Dialogue, Save, etc.
│   └── Private\                 # Implementations & Subsystems
├── Content\                     # Unreal Engine Ingested Assets
│   ├── Characters\              # SK_Survivor_Exosuit, SK_Echo_* (10 species)
│   ├── Weapons\                 # SM_Weapon_* (5 archetypes)
│   ├── Environment\             # SM_Tree_*, SM_Rock_*, SM_Crystal_*
│   ├── Materials\               # M_Master_Surface, M_Landscape_SciFiFrontier, MI_*
│   ├── Audio\                   # A_Weapon_*, A_Footstep_*
│   ├── FX\                      # NS_AW_MuzzleFlash, NS_AW_Weap_Trail, NS_AW_Weap_Impact
│   └── Python\AwPipeline\       # Automation import scripts (import_all.py)
├── ArtSource\                   # Raw Assets (32 GLB + 44 PNG + 36 WAV, ~53 MB)
├── Docs\                        # Documentation & Source of Truth
│   ├── CONTENT_PACK\            # CP-00 to CP-10 Specifications
│   ├── ENGINE_LOGS\             # Antigravity verification reports
│   └── ASTRAWILD_AI_HANDOFF_BIBLE.md # Deep technical architecture blueprint
├── Build.ps1                    # One-click compile script (Editor Win64 Development)
├── Test.ps1                     # One-click automation test runner (54 tests)
├── Build_Package.ps1            # One-click packaging script (Cook & Stage Win64 EXE)
└── Verify_Runtime.ps1           # Automated runtime playtest & telemetry verification
```

---

## 3. Clear Agent Division of Labor

To prevent conflicting edits and redundant work, each AI agent has strict ownership:

### 🛡️ Agent 1: Antigravity (Local Windows UE5 Host)
- **Role**: Build Engineer / Packaging / QA & Runtime Verification / Local Hardware Execution
- **Ownership**:
  - Compiling `ASTRAWILDEditor` and running `Test.ps1`
  - Running Python scripts in Unreal Engine host (`UnrealEditor-Cmd.exe`)
  - Standalone packaging (`ASTRAWILD.exe`) and performance benchmarking
  - Updating `Docs/ENGINE_LOGS/` and `Docs/BUILD_STATUS.md`
- **Rule**: Do NOT rewrite gameplay architecture without evidence; only fix blocking compilation/integration bugs.

### 🎨 Agent 2: Qwen (Visual & Technical Art Engineer)
- **Role**: Visual Engineering / Shader Enhancement / Material Pipelines / Animation Blueprints
- **Next Tasks (P0)**:
  1. Write `Content/Python/setup_master_materials.py` to create/enhance:
     - `M_Echo_Master` (PBR, Dynamic Emissive Tint per Element: Ember/Volt/Rime/Gloom/Astra, Dissolve effect)
     - `M_Terrain_Master` (4-layer landscape blend: Grass, Granite, Sand, Snow)
     - `M_Environment_Master` (Foliage wind WPO, Roughness/AO PBR)
  2. Write Material Instance Generator for 408 `MI_*` variations.
  3. Write Animation Blueprint setup scripts for the 8 Echo body plans.

### ⚔️ Agent 3: GLM 5.3 (Gameplay Systems & Content Expansion)
- **Role**: Core Gameplay / Dialogue Trees / Quest Design / Boss Mechanics / Deep Lore
- **Next Tasks (P0)**:
  1. **Village Dialogue Trees (CP-05 & P12)**:
     - Expand `UAstrawildDialogueTreeDefinition` for all 11 NPCs in Dawnstead and Driftwood Landing (Rowan, Borin, Wren, Perry, Nima, etc.) with conditional branches (`bHasDiscoveredZone`, `bQuestCompleted`, item rewards).
  2. **Boss #2 Encounter Design — Vault Colossus (CP-03 / CP-04)**:
     - Design 3-phase mechanics, telegraph rings, weak-point timings, and hazard pools for Sunken Vault dungeon boss (Tidebreaker Isles).
  3. **Tier 3-4 Tech Tree & Skiff Upgrades (CP-08 / CP-09)**:
     - Design advanced crafting recipes, automation robots, and Skiff engine booster upgrades.

---

## 4. How Incoming Agents Should Pick Up Work

1. **Check Out Latest Branch**:
   ```bash
   git fetch origin
   git checkout agent/antigravity-ue5-v2
   ```
2. **Read the Technical Blueprint**:
   Read [`Docs/ASTRAWILD_AI_HANDOFF_BIBLE.md`](file:///E:/AstrawildGame/Docs/ASTRAWILD_AI_HANDOFF_BIBLE.md) to inspect exact struct definitions and APIs.
3. **Write Targeted Changes**:
   - For C++ changes: Make surgical edits; ensure headers match `UCLASS()`, `USTRUCT()`, `UPROPERTY()` UHT macros.
   - For Visual/Python changes: Put scripts in `Content/Python/`.
   - For Docs/Content Pack: Update files in `Docs/CONTENT_PACK/`.
4. **Push to Dedicated Branch**:
   - Qwen branches: `qwen/<feature-name>`
   - GLM branches: `glm/<feature-name>` or push to `main`/`agent/antigravity-ue5-v2` as coordinated.
5. **Antigravity Auto-Verification**:
   Antigravity will automatically fetch, compile, run `Test.ps1`, verify on UE 5.8.2, and report back!
