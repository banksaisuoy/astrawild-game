# ASTRAWILD Production Gap Audit

**Project:** ASTRAWILD: Echoes of the First Dawn
**Engine target:** Unreal Engine 5.8
**Branch:** `release/vertical-slice-v1`
**Audit scope:** repository-side production pass through the current local review

> This audit deliberately separates **source/configuration evidence** from **Unreal Editor evidence**. A passing static validator is not proof of UHT, C++ compilation, asset import, PIE, Network PIE, profiling, cooking, or packaging.

## Executive summary

The repository now contains a coherent compact vertical-slice source package: a server-guarded prototype arena contract, first-loop data and quest bootstrap, transactional capture/build/food/inventory safeguards, source-level inventory/capture/mecha replication bridges, generated original source meshes for the complete current Echo catalogue, generated audio coverage, Editor importer/scaffold scripts, and regression validators. The branch is **not a finished playable Unreal build** because the sandbox does not contain Unreal Engine 5.8, Windows MSVC, the Unreal Python module, or the Editor-created binary content required by the final runtime.

The recommended production order remains: **compile first, import second, author the four-zone compact map and P0 assets third, run a deterministic single-player loop fourth, then verify only the network surfaces that have deliberate replication coverage**. Adding more catalogue breadth before those gates pass would increase review noise rather than increase playable quality.

## Repository-side completion matrix

| Area | Repository result | Verification | Remaining risk |
|---|---|---|---|
| Compact first-loop bootstrap | Server-only/idempotent PrototypeArena with stable IDs, Dawn Fiber, Storage Chest, Aquavine spring, Solarix Alpha phase data | Vertical-slice guard + source review | Runtime generation is a prototype aid, not an authored `.umap`; boss controller/DataTables still need Editor assignment |
| Inventory | Capacity preflight, replicated slots, autonomous-client Server RPC requests, server-side mutation wrappers, OnRep UI event | Vertical-slice guard + static source inspection | UHT/replication serialization and UI behavior require UE compile/Network PIE |
| Capture | Server-only projectile/capture outcome, failed-spawn item refund, replicated party/state, autonomous-client throw RPC | Vertical-slice guard + static source inspection | Projectile ownership, target validation under latency, and client presentation require Network PIE |
| Food/survival | Spoilage/buff/refrigeration save state; full-hunger consume preflight and rollback guard | Vertical-slice guard + runtime contract | Full-hunger and save/load behavior require PIE |
| Building/save | Placement material refund on spawn failure; generic placed-building restore; empty container restore clears stale contents | Vertical-slice guard + runtime contract | Blueprint subclass/material registry fallback and world persistence need Editor PIE validation |
| Quest/interactions | DataTable soft-path bootstrap, starting quest, dependent quest auto-start, collect/interact/reward flow | Vertical-slice guard + content contract | Imported DataTables and reward/UI presentation require Editor validation |
| Power grid | Timer-driven update and corrected battery discharge allocation | Vertical-slice guard + source review | Timing, recharge/discharge and save behavior require PIE |
| Mecha | Five frames, fourteen weapons, animation/VFX contracts, authority-gated input/state RPC bridge and replicated presentation fields | Mecha validator + vertical-slice guard | Final skeletal mesh, AnimBP, Niagara, cockpit Widget Blueprint, socket setup and Network PIE are pending |
| Character/Echo models | 218 deterministic original static OBJ source meshes plus Player and Alpha source meshes | Character/map asset validator | OBJ files are not rigged skeletal meshes, final materials, LODs, collision or `.uasset` assets |
| Map models | Four compact-map kit OBJ source meshes and manifest | Character/map asset validator | No authored World Partition map, navigation, landscape, lighting, HLOD or Data Layers in repository |
| Audio | 31 SFX WAV, 9 ambience WAV, 2 original MP3 music tracks and codec/duration manifest | Audio validator + ffprobe-backed manifest | Sound Cues, attenuation, concurrency, adaptive music, mix and runtime event routing require Editor |
| Import/scaffold | 32 CSV mappings plus generated OBJ/audio discovery and reports | Importer coverage validator | Actual execution requires Unreal Editor Python and reflected classes |
| Originality | ASTRAWILD names/data and generated source assets use original direction; guard lists are validator data only | Static content/originality review | Any future third-party asset must be recorded in `Docs/ThirdPartyLicenses.md` before commit |

## High-priority remaining work

### P0 — Establish a real playable slice in Unreal Editor

Compile `ASTRAWILDEditor Win64 Development` with UE 5.8 and MSVC 2022. Fix every UHT, link, Blueprint nativization, and reflected-property issue before importing or authoring content. Run the importer inside the Unreal Editor Python environment and retain its JSON reports. Verify that all 32 DataTables resolve to the expected reflected row structs and that the generated asset registry is saved.

Author or replace the source placeholders with original production assets for the Player, Pyrelite, Thornback, Aquavine and Solarix Alpha. Each Echo needs a valid skeletal mesh, skeleton, physics asset, material instances, collision, LOD policy and Animation Blueprint. The static OBJ catalogue is a coverage scaffold, not the final character art pass.

Create the compact four-zone map described by `VERTICAL_SLICE_MAP_20MIN_SPEC.md`: Dawn Spire, Resource Grove, Rest Sanctuary and South-East Danger Pit. Add blocking, navigation, spawn volumes, the resource route, water interaction, camp/building cluster, encounter route, boss boundary, return path and reward placement. Prefer authored level actors over runtime generation for the acceptance map; keep PrototypeArena disabled or isolated when final actors are present.

### P1 — Make combat and presentation readable

Create original Niagara systems for Solar sparks, Geo dust, Torrent splash, capture resonance, damage feedback, and the exosuit beam/overboost/plasma contract rows. Create Sound Cues for the generated WAV sources, set attenuation/concurrency/looping/compression, and connect authoritative damage, capture, boss phase and mecha events through the existing presentation boundaries.

Author the Player/Echo locomotion, attack, hit-react, dodge, capture and Alpha phase animation graphs. Author the cockpit Widget Blueprint with target lock, energy, heat and shield values bound to `FAstrawildCockpitState`. A placeholder scaffold should be reported as incomplete, not promoted to production status.

### P2 — Prove the 20–30 minute loop

Run a deterministic single-player PIE checklist: spawn at Dawn Spire, collect Dawn Fiber, craft the first Resonator, harvest while capacity is constrained, follow the quest chain to the camp, interact with the Aquavine spring, use the Storage Chest, catch an Echo, summon and dismiss the companion, place and save a building, reload, travel through the route, enter Danger Pit, and validate the Alpha encounter handoff. Include the food regression: full hunger must not consume a tracked food item; below-max hunger must consume exactly one item and persist spoilage/buff state through save/load.

Do not claim boss completion merely because `AAstrawildAlphaEcho` creates phase arrays. Assign `AAstrawildBossAIController` and the imported boss encounter/attack tables, then record the actual PIE result and Output Log.

### P3 — Verify network scope honestly

After source compilation, run two-player Network PIE for replicated inventory slots, capture party/state and mecha state/input. Confirm that autonomous clients cannot mutate inventory or resolve capture outcomes locally, that server state reaches the owning UI, and that failed operations do not duplicate or delete items. Separately test combat, attributes, buildings, quests and generic character state; these systems are not automatically co-op-complete merely because the inventory/capture/mecha bridges exist.

### P4 — Performance and package evidence

Measure one representative outdoor cell, one worker-heavy camp and one VFX-heavy Danger Pit at the chosen scalability preset. Record game-thread, render-thread, GPU, memory and network observations. Then run Development cook/package, launch the executable, verify the map and save path, and record the package path, executable hash, engine version, warnings and errors. No FPS, memory, package or shipping claim is valid without this evidence.

## Explicit non-goals of this repository pass

The sandbox pass does not create or certify final `.uasset`/`.umap` files, skeletal rigs, Animation Blueprints, Niagara graphs, materials, landscape layers, foliage assets, Sound Cues, Widget Blueprints, navigation data, World Partition/HLOD configuration, PIE results, Network PIE results, profiling captures or packaged executables. These require the supplied Windows machine and Unreal Editor 5.8.

The generated OBJ assets are intentionally original, deterministic, low-complexity source silhouettes. They should be used for coverage, naming, importer validation and greybox blocking only. They must be retopologized, rigged, materialized and reviewed in Editor before being treated as final game models.

The generated WAV/MP3 files are original source audio coverage. The two music files are stored with their actual MP3 extensions because the generation service returned MP3 payloads. They still require Sound Cue routing, loudness/mix review, looping or stinger configuration, attenuation and runtime event verification.

## Evidence required to close the audit

| Gate | Required artifact | Current status |
|---|---|---|
| Source preflight | Clean branch/hash, all validator output, PowerShell transcript | Static checks pass locally; GitHub remote authentication currently blocks fetch/push verification |
| UE compile | UE 5.8 Output Log/IDE result for `ASTRAWILDEditor Win64 Development` | Pending Windows |
| Import | DataTable and generated-asset JSON reports with zero unexpected failures | Pending Windows Editor |
| Asset authoring | Screenshots/paths for skeletal meshes, AnimBPs, Niagara, materials, Widgets, Sound Cues and map | Pending Windows Editor |
| Single-player | PIE notes/video/screenshots and Output Log for the first-loop checklist | Pending Windows Editor |
| Network | Two-player Network PIE logs and replicated state observations | Pending Windows |
| Performance | Profile captures for outdoor/camp/Danger Pit | Pending Windows |
| Package | Cook/stage/archive output, executable hash and launch smoke test | Pending Windows |

## Decision

The repository-side production pass should be considered **source/configuration ready for the Windows Editor handoff, but not production-playable or shipping-ready**. The next highest-value action is to restore GitHub authentication, safely fetch the branch, push the reviewed source pass, and then execute the Windows evidence gates in order. If any UE compile/import gate fails, stop and fix it before increasing content scope.
