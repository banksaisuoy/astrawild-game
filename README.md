# ASTRAWILD — Echoes of the First Dawn

> [!NOTE]
> **Current status (v9.3 ASSET OVERHAUL — NO PLACEHOLDERS / NO PALETTE SWAPS, 2026)**: the repository is source-complete on branch
> `final-completion` — 126 world-free automation contracts, full 12-zone world, 229 Echo
> species, MQ-01..17 + two endings, save schema V5 — and the staged art is now **109 REAL
> unique CC0 meshes** (no cylinders, no recolors): 3 survivor armor tiers (T1 Scavenger /
> T2 Astraite / T3 Singularity Exosuit, rigged 62-bone humanoids with gun anims), 42 Echo
> species + 16 base archetypes + 14 showcase bosses (Quaternius creatures), 5
> geometry-distinct weapons, 4 vehicles (Dawn Skiff hover + ground rover), 4 ore nodes,
> 21 environment props — manifest **189/189 present / 0 pending**, 1:1 source uniqueness
> enforced, per-asset CC0 provenance (`Docs/ASTRAWILD_REAL_ASSET_CREDITS.json`).
> **One-click on Windows: run `Setup_And_Play.bat`** — it imports everything, builds the
> showcase map (`/Game/Maps/L_Showcase_ArtOverhaul`) and opens the editor; press Play.
> **Everything engine-side is ENGINE-UNVERIFIED at this tip** — the one-time integration
> run (build, 126 tests, PIE, package, V2-29..V2-36 queue) belongs to the Windows UE 5.8
> machine per `Docs/ASTRAWILD_MASTER_CONTROL.md` v9.3 §8 and
> `Docs/ASTRAWILD_FINAL_BUILD_HANDOFF.md`. The canonical control doc is
> **`Docs/ASTRAWILD_MASTER_CONTROL.md`**. The narrative text below this note is the
> Vertical-Slice-era README, retained as history (its "Batch 7 / 54/54 tests" snapshot
> refers to the 2026-09-01 Antigravity V2 session).

**Current world (Batch 7): The Shattered Vale** — six 800 m zones (Dawn Fields, Dusk Marsh, Glimmerwood, Ember Ridge, Frostveil Expanse, Hollow Approach) over a 2.4 km × 1.6 km procedural terrain, each zone with its own wildlife, resources, landmarks and signature light. Runtime world needs zero assets; an optional editor Landscape path ships in `Content/Heightmaps/`.

ASTRAWILD is a third-person cooperative survival adventure prototype for Unreal Engine. The first milestone is a playable Vertical Slice: one small region, three prototype Echo creatures, exploration, combat, capture, crafting, a small base, and reliable save/load.

## Current repository status

> [!TIP]
> **LATEST STATUS**: **SOURCE-COMPLETE / REAL-ART-CATALOG / ENGINE-UNVERIFIED (v9.3 ASSET OVERHAUL)** —
> `final-completion`, 126 automation contracts, 229 species, Sci-Fantasy mutation
> system wired (incl. runtime theme-material swap), LFS 491/491.
> The complete current-state truth is **`Docs/ASTRAWILD_MASTER_CONTROL.md` (v9.2)**;
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
