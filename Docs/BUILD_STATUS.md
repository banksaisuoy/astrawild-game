# ASTRAWILD Build Status — Honest Verification Report

**Project:** ASTRAWILD: Echoes of the First Dawn
**Project file:** `ASTRAWILD.uproject`
**Target engine:** Unreal Engine 5.8
**Primary module:** `AstrawildCore`
**Target platform:** Windows PC
**Active branch:** `release/vertical-slice-v1`
**Latest repository commit:** `c5e3953` (local source pass; push pending remote recheck)
**Repository:** [private GitHub repository](https://github.com/banksaisuoy/astrawild-game)

> **Important boundary:** Source/static validation is not Unreal C++ compilation. Unreal compilation is not PIE. PIE is not a packaged shipping build. This file intentionally records only evidence that has actually been produced.

## Current verification status

| Layer | Status | Evidence or limitation |
|---|---|---|
| Git branch and repository sync | **PENDING PUSH** | The source pass is committed locally at `c5e3953`; remote recheck and push are the next gate. Windows baseline `a90a83f` was clean/up to date. |
| Python content-contract validation | **PASS** | Baseline Windows checks passed; the expanded local suite also passes content, runtime, generated-header, and editor-automation validators. |
| Git whitespace/diff gate | **PASS** | `git diff --check` passes for the current local source/config/data pass. |
| Generated-header presence scan | **PASS** | Static scan found no reflected header missing a `generated.h` include. |
| Unreal 5.8 C++ compile | **NOT RUN HERE** | Requires the user’s Windows machine with UE 5.8 and MSVC 2022. |
| DataTable import in Unreal Editor | **NOT RUN HERE** | CSV sources exist; derived `.uasset` DataTables must be imported in Editor. |
| PIE smoke test | **NOT RUN HERE** | Requires authored map, Blueprint children, imported assets, and Editor execution. |
| Network PIE/co-op test | **NOT RUN HERE** | Requires a real multiplayer session. Client RPC/prediction hardening remains a later verification task. |
| Windows Development package | **NOT RUN HERE** | `Tools/Package_Astrawild.ps1` is prepared, but no package was produced in this environment. |
| Shipping readiness | **NOT READY** | No UE binary assets or executable are tracked in this repository at this time. |

## Delivered production source contracts

The release branch now contains the following source/data preparation slices. These are implementation contracts and are not presented as finished visual content.

| Slice | Delivered preparation |
|---|---|
| World and survival | 4.096 km square / 8×8 cell contract, four biome rows, spawn rules, 16 spire rows, environment-hazard component, data-driven weather rows, persistent day/night clock, World Partition handoff, default-spire initialization, and discovered-spire save field. |
| EchoDex | 30 original species rows, append-only elemental expansion with legacy Aether preservation, multi-element affinity fields, trait/work/mount/breeding metadata, and element automation-test source. |
| Mount and breeding | Native mount eligibility/attachment framework, 21 mount profiles, breeding-group and trait tables, deterministic egg inheritance/incubation, and additive egg save state. |
| Colony and SAN | Echo SAN component, work suitability and efficiency fields, building-hosted colony work queues, authority guards, completion events, and SAN save fields. |
| Technology | Technology DataTable schema, 20-node research tree, prerequisite/cost validation, authority-gated unlocks, and save fields for unlocked tags/research points. |
| Crafting | Recipe DataTable schema, 64 source recipes, station and technology gates, DataTable registration, ingredient rollback when output insertion fails, and appended `HeatForge` station enum value. |
| Ranged combat | Eight weapon rows across bow/repeater/beam classes, elemental ammo fields, cooldown/magazine/reload contract, and a hitscan path routed through the existing combat damage pipeline. |
| Tower dungeons | Five tower rows, required-key consumption, co-op participant tracking, server-side time limit and boss-completion lifecycle contract. |
| UI and packaging | Native master HUD, EchoDex, technology, dungeon-status widget bases; expanded validation script; opt-in compile/package PowerShell workflow. |
| Evolution and integration | Twelve data-driven evolution paths, target-asset handoff, generic quest progress bridges for craft/capture/collect/interact/reach/defeat, DataTable row-name fallback, save sanitization, and reusable runtime/generated-header validators. |
| Visual/world polish contracts | Lumen/scalability presets, landscape MPC parameter contract, weather-driven wetness bridge, 15 foliage distribution rows, editor scaffold expansion, and original audio registry; final material graphs, foliage meshes, Niagara graphs, Sound Cues, map actors, and binary assets remain Windows Editor work. |

## Source data inventory

The reviewable source-of-truth CSVs are under `Content/Astrawild/Data/Source/`. The current inventory is 19 tables, including `DT_BossEncounters.csv`, `DT_BossAttacks.csv`, and `DT_FoliageRules.csv` in addition to the earlier production tables.

The repository deliberately does not pretend that CSV files are Unreal DataTable assets. Import them into `Content/Astrawild/Data/Imported/` with the row structs named in the relevant handoff documents. Keep the CSV sources under review and commit derived `.uasset` files with Git LFS when they are authored and tested.

## Unreal Editor work still required

The Windows owner must create or import the Open World/World Partition map, Data Layers, HLOD setup, four biome landscape/material regions, spawner volumes, the 16 spire actors, player/Echo skeletal meshes and Animation Blueprints, Niagara systems, sound cues, UI Widget Blueprints, weapon meshes/animations, breeding pen and egg presentation, tower maps, boss actors, original icons, and final lighting/post-processing. All external assets must be recorded in `Docs/ThirdPartyLicenses.md` before commit.

The native code is intentionally defensive around missing assets, but default engine primitives and empty soft references are not a production art pass. Do not mark P0–P4 or the master UI complete until the assets are imported, assigned, opened without errors, and exercised in PIE.

## Windows verification sequence

Run these commands from the repository root after pulling the active branch:

```powershell
git fetch origin release/vertical-slice-v1
git checkout release/vertical-slice-v1
git pull --ff-only origin release/vertical-slice-v1
python Scripts/validate_content_contracts.py
python Scripts/validate_runtime_contracts.py
python Scripts/validate_generated_headers.py
python Scripts/validate_editor_automation.py
python -m py_compile Scripts/import_all_datatables.py Scripts/setup_project_assets.py
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location)
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location) -TryUnreal
```

After the Editor opens successfully, import the source DataTables, create the derived Blueprint/UI/map assets, compile `ASTRAWILDEditor Win64 Development`, run the element automation test `Astrawild.Systems.Elements.Compatibility`, and record the exact compiler output. Then run PIE for movement, dodge, harvest, capture, summon, mount/dismount, breeding save/load, SAN work assignment, technology unlock, crafting rollback, ranged fire, spire travel, and tower completion. Run a two-player network PIE session for authority boundaries.

Only after those checks pass should the owner run:

```powershell
.\Tools\Package_Astrawild.ps1 -ProjectRoot (Get-Location) -PackageDirectory .\Builds\WindowsDevelopment
```

The wrapper invokes source validation, the command-line Blueprint compile check, and Unreal `RunUAT BuildCookRun` for a Development package. Inspect the archive, launch the executable, and append the package path, executable hash, map name, engine version, warnings/errors, and screenshots to this file before claiming a working build.

## Performance evidence protocol

No FPS, frame-time, memory, or GPU numbers are claimed in this revision because no Windows profiling capture was supplied. Measure one representative outdoor cell, one worker-heavy base, and one VFX-heavy tower arena. Record game-thread, render-thread, GPU, memory, and network-role behavior at the chosen scalability preset. Use the existing AI distance-LOD and World Partition contracts as hypotheses to verify, not as evidence of a finished performance budget.

## License and originality gate

ASTRAWILD uses original names and data contracts. Do not copy or import characters, models, maps, textures, UI, sounds, or other protected content from Pokémon, ARK, Palworld, Nintendo, Pocketpair, Studio Wildcard, or related properties. For every third-party asset, record the source URL, creator, license, modifications, and redistribution terms in `Docs/ThirdPartyLicenses.md`.

## Evidence log

| Date | Operator | Commit | Engine/build | Evidence | Result |
|---|---|---|---|---|---|
| 2026-08-27 | Windows owner | `a90a83f` | Windows static validation | Content/runtime/generated-header validators, `Tools/Validate_Astrawild.ps1`, 108-file MCP source audit with zero errors; Unreal binary assets explicitly reported as 0 | Baseline source package **PASS**; UE compile/PIE/package pending |
| 2026-08-27 | Manus | `c5e3953` | Sandbox static checks | Expanded boss/visual source pass; 19 CSV mappings, content/runtime/generated-header/editor-automation validators, Python compile and diff check passed | Source pass **PASS**; Windows UE evidence pending |
|  | Windows owner |  | UE 5.8 / MSVC 2022 | Add module compile log, DataTable/scaffold reports, automation result, PIE/network screenshots, and package path here | **PENDING** |
