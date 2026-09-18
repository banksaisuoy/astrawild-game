# ASTRAWILD — Echoes of the First Dawn

> [!NOTE]
> **Current status (MASTER DIRECTIVE v1 MACHINE-READY PACK — 2026-09-18)**: the repository is source-complete on branch
> `final-completion` — **134 world-free automation contracts**, full 12-zone world, 229 Echo
> species, MQ-01..17 + 5 post-game side quests + two endings (each now with a pure-C++ staged cinematic) + NEW GAME PLUS,
> save schema V5 (+ additive NGPlusCycle) — and the staged art is now **109 REAL
> unique CC0 meshes** (no cylinders, no recolors): 3 survivor armor tiers (T1 Scavenger /
> T2 Astraite / T3 Singularity Exosuit, rigged 62-bone humanoids with gun anims), 42 Echo
> species + 16 base archetypes + 14 showcase bosses (Quaternius creatures; 9 of them now bound as direct species bodies — direct-mesh coverage 51/229), 5
> geometry-distinct weapons, 4 vehicles (Dawn Skiff hover + ground rover), 4 ore nodes,
> 21 environment props — manifest **189/189 present / 0 pending**, 1:1 source uniqueness
> enforced, per-asset CC0 provenance (`Docs/ASTRAWILD_REAL_ASSET_CREDITS.json`).
> **FIRST DAY ON THE MACHINE: follow `Docs/ASTRAWILD_ON_PC_TASKS.md`** — the
> hour-by-hour runbook (clone → build → orchestrated source-side flow → asset import →
> 134-test gate → PIE feel grading → package), each step with exact commands, expected
> results, and fail routes. **Came back to a broken/unknown state? `Docs/ASTRAWILD_REVIVAL_PLAN.md`** triages from ANY state.
> The single-command source-side flow is `Tools\Python\first_day_orchestrator.py`
> (verify → prototype map → input assets, idempotent).
> **One-click on a prepared machine: run `Setup_And_Play.bat`** — it imports everything, builds the
> showcase map (`/Game/Maps/L_Showcase_ArtOverhaul`) and opens the editor; press Play.
> Design numbers are never hand-typed: every value lives traced in `Design/design_data.json`
> (`Scripts/extract_design_data.py` + `Scripts/validate_design_data.py` round-trip).
> **Everything engine-side is ENGINE-UNVERIFIED at this tip** — the one-time integration
> run (build, 134 tests, PIE, package, V2-29..V2-36 queue) belongs to the Windows UE 5.8
> machine; risks are ranked with a binding fix-forward protocol in
> `Docs/ASTRAWILD_COMPILE_RISK.md`. The canonical control doc is
> **`Docs/ASTRAWILD_MASTER_CONTROL.md`**; live execution truth is
> **`Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md`**; the cinematic charter + traced
> character-feel contract is **`Docs/ASTRAWILD_CINEMATIC_IDENTITY.md`**. The narrative text below this note is the
> Vertical-Slice-era README, retained as history (its "Batch 7 / 54/54 tests" snapshot
> refers to the 2026-09-01 Antigravity V2 session).

**Current world (Batch 7): The Shattered Vale** — six 800 m zones (Dawn Fields, Dusk Marsh, Glimmerwood, Ember Ridge, Frostveil Expanse, Hollow Approach) over a 2.4 km × 1.6 km procedural terrain, each zone with its own wildlife, resources, landmarks and signature light. Runtime world needs zero assets; an optional editor Landscape path ships in `Content/Heightmaps/`.

ASTRAWILD is a third-person cooperative survival adventure prototype for Unreal Engine. The first milestone is a playable Vertical Slice: one small region, three prototype Echo creatures, exploration, combat, capture, crafting, a small base, and reliable save/load.

## Download & Play

**Option 1 — packaged build (no Unreal Engine required on the playing PC)**

1. Open the **Releases** page of this repository.
2. Download `ASTRAWILD-Win64-Shipping.zip` from the latest release.
3. Unzip it anywhere and run `ASTRAWILD\Binaries\Win64\ASTRAWILD.exe`.

Requirements: Windows 10/11 64-bit, a DirectX 12 capable GPU, 8 GB RAM.
The zip is self-contained — no Unreal Engine install, no prerequisites.

**Option 2 — package it yourself (one click, on the dev machine)**

```bat
Tools\package_windows.bat
```

- Runs RunUAT `BuildCookRun` for **Win64 Shipping** (`-cook -allmaps -stage -pak -package -archive`) and writes the build to `Build\Windows\ASTRAWILD\` — the game executable lands at `Build\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe`.
- Requires Unreal Engine 5.8 at `E:\Epic Games\UnrealEngine` (or set the `UE_ROOT` environment variable to your engine root) and a clone made with **Git LFS**.

**How releases are produced** — `.github/workflows/release.yml` builds, zips and attaches the Shipping build to a GitHub Release whenever a `v*` tag is pushed. It targets a **self-hosted Windows runner** (the dev PC) on purpose: GitHub-hosted runners have no UE 5.8 install, and the engine is far too large to provision per run. The workflow header documents the one-time runner setup (Settings → Actions → Runners → New self-hosted runner → install as a service).

## Current repository status

> [!TIP]
> **LATEST STATUS**: **SOURCE-COMPLETE / REAL-ART-CATALOG / DEFERRED-PACK-COMPLETE /
> FRESH-MACHINE-PLAYBOOK SHIPPED / ENGINE-UNVERIFIED (v9.6 FMP)** —
> `final-completion` @ the v9.6 tip (see `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` §1),
> **133** automation contracts, 229 species, Sci-Fantasy mutation
> system wired (incl. runtime theme-material swap), LFS 586/586 pointers resolved.
> Blank Windows machine → `Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md` +
> `Scripts/fresh_machine_preflight.ps1` + `Docs/ASTRAWILD_FRESH_MACHINE_CHECKLIST.json`.
> The live task-state truth is **`Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md`** (canonical,
> created 2026-09-06); the product rulebook is
> **`Docs/ASTRAWILD_MASTER_CONTROL.md` (v9.6)**;
> engine-run history (the 54/54 + packaged-exe evidence at SHA 8313c61) is
> [**`Docs/PROJECT_STATUS_SUMMARY.md`**](Docs/PROJECT_STATUS_SUMMARY.md) (HISTORICAL)
> and [**`Docs/ENGINE_LOGS/ANTIGRAVITY_EVIDENCE_MANIFEST.md`**](Docs/ENGINE_LOGS/ANTIGRAVITY_EVIDENCE_MANIFEST.md).
> **The old claim below ("compiles with 0 errors across 54/54 tests") is the
> 8313c61-era record, NOT the current tip** — the re-run queue is
> `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` (V2-29..V2-36 — V2-36 is the one-click `Setup_And_Play.bat` real-mesh import + showcase gate).

This repository contains the verified Unreal Engine 5.8.2 C++ core, data contracts, and ArtPack ingestion pipeline. The core playable loop (Spawn -> Move -> Look -> Jump -> Sprint -> Interact -> Inventory -> Build -> Scan -> Attack) is physically verified and compiles with 0 errors across 54/54 automated tests and packaged standalone binaries.

## Recommended environment

Use the Unreal Engine version specified in `ASTRAWILD.uproject` and a Windows development machine with Visual Studio configured for Unreal C++ development. The project is designed for PC first, with controller support planned after the keyboard/mouse loop is stable.

## First launch

Read `ANTIGRAVITY_START_HERE.md` before making changes, then read `PLAN.md`, `STRUCTURE.md`, `ASSETS.md`, `MEMORY.md`, and `Docs/GAME_DEV_WORKFLOW_UNREAL.md`. The C++ source is ready for the target machine to compile, but Unreal binary assets still need to be created in the Editor.

1. Clone this private repository with Git LFS enabled.
2. Open `ASTRAWILD.uproject` with the matching Unreal Engine version.
3. Allow Unreal to generate project files and compile the `AstrawildCore` module.
4. Create a test map under `Content/ASTRAWILD/Maps/Prototype`.
5. Implement the Vertical Slice backlog in `Docs/astra_wild_production_roadmap.md`.

## Large-file archive

The project archive and large-file backups are stored in the [ASTRAWILD Google Drive folder](https://drive.google.com/drive/folders/1hkCl5lYaDu8Cn_uikzdc5kbrAkWbLlon). GitHub remains the source-of-truth for code, configuration, documentation, and actively versioned assets; Drive stores dated archives, raw source assets, and review packages.

## Repository policy

Unreal-generated directories such as `Binaries`, `Intermediate`, `Saved`, and `DerivedDataCache` are ignored. Binary assets are configured for Git LFS through `.gitattributes`. Large source assets that are not required for source control should be archived separately in the project Google Drive folder and referenced from `Docs/ASSET_STORAGE.md`.

## Design documents

| Document | Purpose |
|---|---|
| `Docs/ASTRAWILD_PROJECT_MASTER_PLAN_v1.md` | Consolidated project plan |
| `Docs/astra_wild_game_design.md` | Vision, world and scope |
| `Docs/astra_wild_gameplay.md` | Core gameplay systems |
| `Docs/astra_wild_architecture.md` | Unreal and multiplayer architecture |
| `Docs/astra_wild_art_content.md` | Art, world, audio and content bible |
| `Docs/astra_wild_performance_qa.md` | Performance, security and QA plan |
| `Docs/astra_wild_production_roadmap.md` | Production roadmap and operating plan |
| `Docs/astra_wild_architecture.png` | Architecture diagram |

## Working agreement

Keep gameplay rules in C++ or reusable data-driven systems. Use Blueprint for assembly and rapid iteration. Do not commit generated folders. Do not add third-party assets without a clear license. Any new save data must include a schema version and migration path.
