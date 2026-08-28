# ASTRAWILD Build Status — Honest Verification Report

**Project:** ASTRAWILD: Echoes of the First Dawn
**Project file:** `ASTRAWILD.uproject`
**Target engine:** Unreal Engine 5.8
**Primary module:** `AstrawildCore`
**Target platform:** Windows PC
**Active branch:** `release/vertical-slice-v1`
**Repository head:** verify the current branch commit with `git rev-parse HEAD`; the latest reviewed source/status commit is recorded in the evidence log below.
**Repository:** [private GitHub repository](https://github.com/banksaisuoy/astrawild-game)

> **Important boundary:** Source/static validation is not Unreal C++ compilation. Unreal compilation is not PIE. PIE is not a packaged shipping build. This file intentionally records only evidence that has actually been produced.

## Current verification status

| Layer | Status | Evidence or limitation |
|---|---|---|
| Git branch and repository sync | **PASS** | The branch was fetched and checked against `origin/release/vertical-slice-v1` before the latest documentation push; run `git rev-parse HEAD` and the remote comparison after pulling to reproduce the check. |
| Python content-contract validation | **PASS** | Baseline Windows checks passed; the expanded local suite also passes content, runtime, generated-header, and editor-automation validators. |
| Git whitespace/diff gate | **PASS** | `git diff --check` passes for the current local source/config/data pass; staged and final commit checks are required before push. |
| Generated-header presence scan | **PASS** | Static scan found no reflected header missing a `generated.h` include. |
| Vertical-slice source guard validation | **PASS** | `validate_vertical_slice_guards.py` checks map bootstrap, quest chain, capture/build rollback, inventory capacity/RPC, save/food persistence and rollback, power-grid timer/battery, mounted-partner skill gating, and Gameplay Tag registry. |
| Unreal 5.8 C++ compile | **NOT RUN HERE** | Requires the user’s Windows machine with UE 5.8 and MSVC 2022. |
| DataTable import in Unreal Editor | **NOT RUN HERE** | CSV sources exist; derived `.uasset` DataTables must be imported in Editor. |
| PIE smoke test | **NOT RUN HERE** | Requires authored map, Blueprint children, imported assets, and Editor execution. |
| Network PIE/co-op test | **NOT RUN HERE** | Requires a real multiplayer session. Inventory slots, capture state/party and mecha state now have source-level replication/RPC bridges; broader combat/building/quest/attribute replication and runtime authority behavior still require Windows Network PIE evidence. |
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
| Visual/world polish contracts | Lumen/scalability presets, landscape MPC parameter contract, weather-driven wetness bridge, 15 foliage distribution rows, editor scaffold expansion, original audio registry, and asset production bible; final material graphs, foliage meshes, Niagara graphs, Sound Cues, map actors, skeletal rigs, and binary assets remain Windows Editor work. |
| Astra exosuit integration | Originalized 5 frame profiles and 14 weapon rows, cybernetic evolution data, 5 AnimBP profile rows, 5 Niagara VFX bindings, native AnimInstance bindings, authority/data-driven MechaComponent hardpoint state, source-level replicated input/state RPC bridge, MechaVFXComponent and CockpitWidget contracts. Final skeletal meshes, AnimBPs, Niagara graphs, Widget Blueprint and UE runtime tests remain pending. |
| Compact vertical-slice bootstrap | PrototypeArena now has server-only/idempotent generation, stable resource/building IDs, three Dawn Fiber nodes, Solarix Alpha source encounter data, Storage Chest, Aquavine spring hydration/reward interaction, and GameMode soft-path quest table bootstrap. Final authored map and controller/DataTable assignment still require Editor verification. |
| Underwater expansion source contract | `FAstrawildUnderwaterZoneRow`, `DT_UnderwaterZones.csv`, and `UAstrawildUnderwaterSubsystem` provide Abyssal Trench depth/pressure/oxygen/buoyancy calculation, gradual surface refill, pressure-emergency mode, hazard/spawn tags and optional zone-table lookup. Full 6-DOF swimming, water volumes, oxygen/damage equipment binding, underwater presentation, aquatic AI and submerged base actors remain UE Editor/runtime work. |
| Generated character/map asset coverage | Deterministic original static source meshes for 218 unique Echo rows, Player placeholder, Alpha Solarix, and four compact-map kits, with OBJ/MTL manifests and character/map validator. These are source placeholders, not final skeletal meshes or `.uasset` content. |
| Generated audio coverage | 24 additional gameplay/Echo/boss/exosuit/UI SFX WAVs, 9 zone ambience WAV loops and 2 original encounter/exploration MP3 tracks, with a master audio manifest and codec/duration validator. Sound Cue routing, attenuation, adaptive music and mix remain Editor work. |
| First-loop runtime hardening | Capture/build rollback on failed operations, source-level replicated inventory/capture request bridges, inventory capacity preflight, auto-start dependent quests, food spoilage/buff save state, food-consumption preflight/rollback, placed-building restore, PowerGrid timer/battery correction, mounted partner-skill gating, and project Gameplay Tag registry. These are source contracts pending UE compile/runtime verification. |

## Source data inventory

The reviewable source-of-truth CSVs are under `Content/Astrawild/Data/Source/`. The current inventory is 33 tables, including world-event, ecosystem, cooking, progression, underwater and exosuit animation/VFX tables. `Scripts/validate_handoff_contracts.py` checks that this runbook, the PowerShell runners, importer, scaffold and validator order remain synchronized. Detailed validator behavior is documented in `Docs/VALIDATION_CATALOG.md`.

The repository deliberately does not pretend that CSV files are Unreal DataTable assets. Import them into `Content/Astrawild/Data/Imported/` with the row structs named in the relevant handoff documents. Keep the CSV sources under review and commit derived `.uasset` files with Git LFS when they are authored and tested.

## Unreal Editor work still required

The Windows owner must create or import the Open World/World Partition map, Data Layers, HLOD setup, four biome landscape/material regions, spawner volumes, the 16 spire actors, final player/Echo skeletal meshes and Animation Blueprints, Niagara systems, Sound Cues and adaptive music routing, UI Widget Blueprints, weapon meshes/animations, breeding pen and egg presentation, tower maps, boss actors, original icons, and final lighting/post-processing. The repository now includes static source placeholders for every catalogue row and the compact map kit, but they still require retopology/rigging/material authoring or replacement before production use. All external assets must be recorded in `Docs/ThirdPartyLicenses.md` before commit.

The native code is intentionally defensive around missing assets, but default engine primitives and empty soft references are not a production art pass. Do not mark P0–P4 or the master UI complete until the assets are imported, assigned, opened without errors, and exercised in PIE.

## Windows verification sequence

Run these commands from the repository root after pulling the active branch:

```powershell
git fetch origin release/vertical-slice-v1
git checkout release/vertical-slice-v1
git pull --ff-only origin release/vertical-slice-v1
python Scripts/generate_gameplay_tag_registry.py
python Scripts/validate_content_contracts.py
python Scripts/validate_runtime_contracts.py
python Scripts/validate_generated_headers.py
python Scripts/validate_editor_automation.py
python Scripts/validate_master_echodex.py
python Scripts/validate_generated_assets.py
python Scripts/validate_mecha_contracts.py
python Scripts/validate_vertical_slice_guards.py
python Scripts/validate_character_map_assets.py
python Scripts/validate_audio_pack.py
python Scripts/validate_importer_coverage.py
python Scripts/validate_handoff_contracts.py
python -m py_compile Scripts/*.py
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location)
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location) -TryUnreal
```

After the Editor opens successfully, import the source DataTables, create the derived Blueprint/UI/map assets, compile `ASTRAWILDEditor Win64 Development`, run the element automation test `Astrawild.Systems.Elements.Compatibility`, and record the exact compiler output. Then run PIE for movement, dodge, harvest, capture, summon, mount/dismount, breeding save/load, SAN work assignment, technology unlock, crafting rollback, ranged fire, spire travel, and tower completion. Run a two-player network PIE session for authority boundaries after the inventory, capture and exosuit native replication/RPC bridges compile; broader character/combat/building/quest state remains an explicit co-op gap until it is covered and observed in Network PIE.

Only after those checks pass should the owner run:

```powershell
.\Tools\Package_Astrawild.ps1 -ProjectRoot (Get-Location) -PackageDirectory .\Builds\WindowsDevelopment
```

The wrapper invokes source validation, the command-line Blueprint compile check, and Unreal `RunUAT BuildCookRun` for a Development package. Inspect the archive, launch the executable, and append the package path, executable hash, map name, engine version, warnings/errors, and screenshots to this file before claiming a working build.

## Next: Unreal Engine 5.8 Editor Integration plan

The next phase is an evidence-driven Editor pass. Each stage must produce the listed artifact before the following stage begins; an Output Log line without the corresponding report, asset inspection, or runtime result is not an acceptance signal.

| Stage | Windows action | Required evidence | Stop condition |
|---|---|---|---|
| 1. Repository and source preflight | Pull `release/vertical-slice-v1`, verify `git status`, run every Python validator and `Tools\\Validate_Astrawild.ps1` without `-TryUnreal` | Local/remote hash, clean status, validator output, PowerShell transcript | Any validator failure, missing path, or unparsed PowerShell script |
| 2. Unreal module compile | Open `ASTRAWILD.uproject` in UE 5.8, allow project regeneration, compile `ASTRAWILDEditor Win64 Development` | IDE/Editor Output Log with engine version, target, compiler result, warnings/errors | Any compile/link/UHT/Build.cs error; fix source before importing assets |
| 3. DataTable and generated-asset import | Run `py "Scripts/import_all_datatables.py"` from the UE Python environment | `DataTableImportReport.json`, `GeneratedAssetImportReport.json`, `GeneratedAssetRegistry.json`, imported asset count, no failed rows | Missing reflected row class, failed import, or registry asset not saved |
| 4. Safe scaffold and asset assignment | Run `py "Scripts/setup_project_assets.py"`; inspect landscape, audio, Niagara, animation/VFX folders | `AssetScaffoldReport.json`, screenshots of created/existing/skipped/failed assets, no unexpected failures | Treat placeholders or skipped factories as incomplete; do not proceed as if final art exists |
| 5. Exosuit Blueprint integration | Author original skeletal meshes, AnimBPs, locomotion/flight/overboost montages, Niagara systems, Widget Blueprint, sockets and DataTable references | Asset paths match profile/VFX CSV rows; every Blueprint compiles; cockpit bars/target text and AnimBP variables display in Editor | Missing asset references, compile warnings/errors, invalid socket/path, or placeholder-only presentation |
| 6. Single-player runtime gate | PIE equip/eject, flight, overboost, heat/energy/shield, hardpoint fire, hit trace, cockpit target/LOS lock, VFX delegate events | PIE session notes plus screenshots/video and Output Log with map name and no relevant runtime errors | Any state desync, invalid target lock, damage trace failure, runaway heat/energy, or broken widget binding |
| 7. Multiplayer readiness gate | Verify source replication/RPC coverage for inventory, capture and exosuit state; then run 2-player Network PIE | Server/client logs, replicated state observations, authority checks, client VFX/UI behavior | Do not mark co-op complete from source alone; broader character/combat/building/quest replication still needs deliberate coverage and runtime evidence |
| 8. Development package gate | Run `Tools\\Package_Astrawild.ps1` only after stages 1–7 pass; launch the packaged build | Package path, executable hash, engine version, map, warnings/errors, launch smoke-test result | Cook/package failure, missing cooked asset, startup/map error, or unrecorded warnings |

The Editor pass should update this file after each completed gate, not at the end from memory. Keep source/static results, Editor import results, runtime PIE results, network results, and packaging results as separate evidence rows. The present repository status remains **source/config integration passed; Unreal Editor integration and runtime evidence pending**.

## Performance evidence protocol

No FPS, frame-time, memory, or GPU numbers are claimed in this revision because no Windows profiling capture was supplied. Measure one representative outdoor cell, one worker-heavy base, and one VFX-heavy tower arena. Record game-thread, render-thread, GPU, memory, and network-role behavior at the chosen scalability preset. Use the existing AI distance-LOD and World Partition contracts as hypotheses to verify, not as evidence of a finished performance budget.

## License and originality gate

ASTRAWILD uses original names and data contracts. Do not copy or import characters, models, maps, textures, UI, sounds, or other protected content from Pokémon, ARK, Palworld, Nintendo, Pocketpair, Studio Wildcard, or related properties. For every third-party asset, record the source URL, creator, license, modifications, and redistribution terms in `Docs/ThirdPartyLicenses.md`.

## Evidence log

| Date | Operator | Commit | Engine/build | Evidence | Result |
|---|---|---|---|---|---|
| 2026-08-27 | Windows owner | `a90a83f` | Windows static validation | Content/runtime/generated-header validators, `Tools/Validate_Astrawild.ps1`, 108-file MCP source audit with zero errors; Unreal binary assets explicitly reported as 0 | Baseline source package **PASS**; UE compile/PIE/package pending |
| 2026-08-27 | Manus | `c5e3953` | Sandbox static checks | Expanded boss/visual source pass; 19 CSV mappings, content/runtime/generated-header/editor-automation validators, Python compile and diff check passed | Source pass **PASS**; Windows UE evidence pending |
| 2026-08-27 | Manus | `edca9b9` | Sandbox static checks | Originalized exosuit data; 32 CSV mappings; authoritative mecha hit trace, cockpit target/LOS validation, strengthened mecha validator, all content/runtime/generated-header/editor-automation/master-asset validators and Python compile passed; source commit pushed to `release/vertical-slice-v1` | Source integration **PASS**; UE compile/AnimBP/Niagara/UI/PIE pending |
| 2026-08-27 | Manus | `6ae86be` | Documentation/status sync | BUILD_STATUS corrected to the final pushed hash and the staged Unreal Editor Integration plan | Documentation **PASS**; UE compile/Editor import/PIE/package pending |
| 2026-08-28 | Manus | `5ac0da3` | Repository review/status sync | Current repository review completed; historical audit explicitly labeled; staged UE 5.8 Editor Integration gates documented | Review **PASS**; UE compile/Editor import/PIE/package pending |
| 2026-08-28 | Manus | `7c85c7f` | Status hash correction | BUILD_STATUS latest hash synchronized with the pushed branch head | Documentation **PASS**; UE compile/Editor import/PIE/package pending |
| 2026-08-28 | Manus | `e36dc26` | Status hash correction | BUILD_STATUS synchronized with the final review commit and branch head | Documentation **PASS**; UE compile/Editor import/PIE/package pending |
| 2026-08-28 | Manus | current source pass | Sandbox static checks | Added compact-map bootstrap, first-loop runtime hardening, Gameplay Tag registry, deterministic source meshes for 218 Echo rows plus Player/Alpha/map kits, 24 SFX/9 ambience/2 music sources, importer/scaffold wiring, inventory/capture/mecha source replication bridges, food-consumption rollback, concrete importer coverage checks, handoff contract validator, validation catalog and expanded operational runbook; full static suite and `git diff --check` passed | Source/config/assets **PASS**; UE compile/Editor import/PIE/package pending |
|  | Windows owner |  | UE 5.8 / MSVC 2022 | Add module compile log, DataTable/scaffold reports, automation result, PIE/network screenshots, and package path here | **PENDING** |
