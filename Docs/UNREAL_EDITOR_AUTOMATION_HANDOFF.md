# ASTRAWILD Unreal Engine 5.8 Editor Automation Handoff

**Project:** ASTRAWILD: Echoes of the First Dawn
**Target:** Unreal Engine 5.8 on Windows
**Project root:** `C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game`
**Branch:** `release/vertical-slice-v1`

> **Evidence boundary:** This document is an execution runbook, not evidence that the Unreal project has already compiled or played. Static Python validation does not prove UHT/C++ compilation. It does not prove C++ compilation. A C++ build does not prove DataTable import. DataTable import does not prove Blueprint compilation. These source-side steps do not prove PIE, do not prove Network PIE, and do not prove packaging. Until the Windows gates below produce their artifacts, the honest status remains **source/config contract prepared; UE binary content and runtime verification pending**.

## 0. Roles, interpreters and stop rules

The repository contains two kinds of scripts. Host-Python validators read CSV, C++, JSON and configuration files without Unreal. The importer and scaffold scripts require the `unreal` Python module and must run inside the Unreal Editor Python environment. Running an Editor script with ordinary Windows Python is expected to fail because that interpreter does not provide `unreal`.

| Execution surface | Scripts | What it can prove | What it cannot prove |
|---|---|---|---|
| Windows host Python | `Scripts/validate_*.py`, generators and `py_compile` | Source paths, CSV schema/reference integrity, manifests, static guard tokens, deterministic asset metadata and handoff consistency | UHT, C++ link, reflected row loading, Editor factories, asset import, Blueprint/PIE behavior |
| PowerShell | `Tools/Validate_Astrawild.ps1` | Required paths, host validators and optional command-line Blueprint compile invocation | A successful C++ module build, visual quality, gameplay, Network PIE or package launch unless the optional stages actually run and evidence is retained |
| Unreal Editor Python | `Scripts/import_all_datatables.py`, `Scripts/setup_project_assets.py` | Editor-created DataTables, imported source assets, scaffold objects and JSON reports | Final materials, rigs, AnimBPs, Niagara graphs, Widget Blueprints, authored map, runtime behavior or shipping readiness |
| UE 5.8 IDE/Editor | Project compile and Blueprint compile | Actual module/UHT/link result and Blueprint compilation result | Playability, multiplayer behavior, performance or package correctness by itself |
| PIE / Network PIE | Manual or automated Editor play sessions | Runtime behavior and authority observations for the tested scenarios | Untested systems and packaged-build behavior |
| RunUAT / packaged executable | `Tools/Package_Astrawild.ps1` | Cook/stage/archive and launch evidence if all commands return success | Art quality or untested gameplay paths |

**Stop immediately** when a required source validator fails, a PowerShell script cannot be parsed, a DataTable row struct cannot be loaded, an importer report contains unexpected failures, a Blueprint or C++ compile fails, an asset path is unresolved, or a PIE test produces a relevant runtime error. Do not continue to later gates to hide an earlier failure.

The validation runner returns exit code `2` when `-TryUnreal` or `-Package` is requested but no Unreal executable or RunUAT is found. Treat that as **not executed**, not as a successful UE check. Exit code `0` is required for the host-only validation invocation and for every optional UE/package invocation that is actually attempted. For the implementation details and limitations of every validator, use [`VALIDATION_CATALOG.md`](VALIDATION_CATALOG.md) alongside this runbook. For the staged design of the attached deep expansion, use [`NEXT_GEN_EXPANSION_ROADMAP.md`](NEXT_GEN_EXPANSION_ROADMAP.md). Sprint 2 execution details are in [`SPRINT_2_SPACE_GUILD_DYES_HANDOFF.md`](SPRINT_2_SPACE_GUILD_DYES_HANDOFF.md). Pillar 1 must clear its UE gate before Pillars 2–6 are widened.

## 1. Repository preflight

Open PowerShell in the project root. Preserve any local work before pulling. Do not use reset, clean, rebase or stash blindly on a dirty worktree.

```powershell
Set-Location 'C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game'
git status --short
git fetch origin release/vertical-slice-v1
git checkout release/vertical-slice-v1
git pull --ff-only origin release/vertical-slice-v1
git rev-parse HEAD
git rev-parse origin/release/vertical-slice-v1
git rev-list --left-right --count HEAD...origin/release/vertical-slice-v1
```

For the reviewed source pass, the local and remote hashes should be equal and the divergence count should be `0 0`. If the worktree is dirty, stop and review every changed path before any pull or merge operation.

Start a transcript so the exact Windows evidence is retained:

```powershell
$evidence = Join-Path (Get-Location) 'Saved\Astrawild\WindowsEvidence'
New-Item -ItemType Directory -Force $evidence | Out-Null
Start-Transcript -Path (Join-Path $evidence '01-preflight-transcript.txt') -Force
```

The source package intentionally uses original ASTRAWILD terminology and assets. Do not introduce copied characters, names, models, maps, textures, UI, VFX, sounds or music from external games. Any third-party asset must be recorded in `Docs\ThirdPartyLicenses.md` before it enters the project.

## 2. Host-Python validation matrix

Run the following commands before opening the Editor. The command list is explicit so a future validator cannot silently disappear from the handoff.

```powershell
python -m py_compile Scripts\*.py
python Scripts\validate_content_contracts.py
python Scripts\validate_runtime_contracts.py
python Scripts\validate_generated_headers.py
python Scripts\validate_editor_automation.py
python Scripts\validate_master_echodex.py
python Scripts\validate_generated_assets.py
python Scripts\validate_mecha_contracts.py
python Scripts\validate_vertical_slice_guards.py
python Scripts\validate_character_map_assets.py
python Scripts\validate_audio_pack.py
python Scripts\validate_importer_coverage.py
python Scripts\validate_vehicle_contracts.py
python Scripts\validate_handoff_contracts.py
```

The expected terminal results are the following. The exact output should be copied into the transcript or attached as a separate text artifact rather than summarized from memory.

| Validator | Required scope | Expected result |
|---|---|---|
| `validate_content_contracts.py` | Required source paths, 38 CSV schemas, row counts, numeric ranges, canonical biome/spire/Echo/recipe/evolution/weather/dungeon/boss/ranged/mecha data, reflected-header presence and basic brace/parenthesis balance | `ASTRAWILD content contract validation passed.` |
| `validate_runtime_contracts.py` | Cross-table references for quests, objectives, traits, breeding, mounts, spawns, evolutions, recipes, dungeons, boss encounters/attacks and fast-travel spires | `ASTRAWILD runtime contract validation passed.` |
| `validate_generated_headers.py` | `generated.h` include guard for reflected headers | `Generated-header validation passed.` |
| `validate_editor_automation.py` | AST parse of `TABLE_MAPPING`, exact source CSV set, expected row structs, importer/scaffold markers and visual config markers | `ASTRAWILD Editor automation contract validation passed (38 CSV mappings).` |
| `validate_master_echodex.py` | 200 unique master Echo rows, 3 active skills and 12 work levels per row | `ASTRAWILD master Echo validation passed ...` |
| `validate_generated_assets.py` | Nine prop OBJ and seven core SFX WAV legacy manifests, topology/material references and WAV metadata/hashes | `ASTRAWILD generated asset validation passed ...` |
| `validate_mecha_contracts.py` | Five frame rows, fourteen weapon rows, five animation profiles, five VFX bindings, data references, target/LOS logic and replicated input/state source bridge | `ASTRAWILD mecha contract validation passed ...` |
| `validate_vertical_slice_guards.py` | Map bootstrap, quest chain, interaction, capture/build rollback, inventory capacity/RPC, food preflight/rollback, save, power-grid, mounted-partner, underwater, Fishing and Racing guards | `ASTRAWILD vertical-slice guard validation passed ...` |
| `validate_character_map_assets.py` | 218 Echo source meshes, Player/Alpha source meshes, four map-kit meshes and manifest coverage/originality | `ASTRAWILD character/map asset validation passed ...` |
| `validate_audio_pack.py` | 24 extended SFX, nine ambience loops and two MP3 music files; file existence, codec/duration and manifest uniqueness | `ASTRAWILD audio pack validation passed ...` |
| `validate_importer_coverage.py` | Mesh/audio destinations, OBJ/WAV/MP3 discovery and generated registry report markers | `ASTRAWILD importer coverage validation passed ...` |
| `validate_vehicle_contracts.py` | Exactly 12 vehicle rows and 12 modular part rows, vehicle types/slots/tuning, reflected API names and server-authority bridge | `ASTRAWILD vehicle contract validation passed ...` |
| `validate_handoff_contracts.py` | Handoff/runners/importer/scaffold alignment, 38 CSV count, validator order, report names, UE command gates and explicit evidence boundaries | `ASTRAWILD handoff contract validation passed ...` |

The validators are intentionally complementary. They do not replace one another: the legacy generated-asset validator covers the original nine props and seven core SFX, while the newer character/map and audio-pack validators cover the broader source coverage. A complete host-Python pass requires all rows above to pass.

Run the repository wrapper in host-only mode and capture its output:

```powershell
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location) *>&1 | Tee-Object (Join-Path $evidence '02-Validate_Astrawild-host.txt')
if ($LASTEXITCODE -ne 0) { throw "Host validation failed" }
```

The wrapper checks required paths, runs the same validator families, reports the number of `.uasset`/`.umap` files, and reports dirty status. A warning that no Unreal binary assets exist is expected for this source-only branch and must not be converted into a production-ready claim.

## 3. Source coverage and LFS checks

The current source coverage is deliberately recorded as source coverage, not final UE content.

| Family | Expected source coverage | Production interpretation |
|---|---:|---|
| Echo meshes | 218 OBJ/MTL sources | Static silhouettes for naming, catalogue coverage and greybox; not rigged skeletal meshes |
| Character meshes | 2 OBJ/MTL sources | Player and Alpha Solarix placeholders; not final rigs or Animation Blueprints |
| Compact map kit | 4 OBJ/MTL sources | Zone-blocking source geometry; not an authored `.umap`, landscape or World Partition setup |
| Legacy props | 9 OBJ sources | Static prop candidates requiring collision, material and LOD review |
| Extended SFX | 24 WAV sources | Source audio requiring Sound Cue, attenuation, concurrency and mix setup |
| Ambience | 9 WAV loops | Source loops requiring zone routing and loop/volume review |
| Music | 2 MP3 files | Original source tracks; actual codec is MP3 and must remain named `.mp3` |

Verify the manifests and LFS state on Windows or in the repository clone:

```powershell
python Scripts\validate_character_map_assets.py
python Scripts\validate_audio_pack.py
git lfs status
git lfs fsck
```

`git lfs fsck` is an integrity check for objects tracked by LFS. It is not an Unreal import check. If a legacy asset is reported as a regular blob despite the `.gitattributes` policy, normalize it with a reviewed follow-up commit rather than rewriting history.

## 4. Unreal Editor startup and C++ compile gate

Open `ASTRAWILD.uproject` with Unreal Engine 5.8. Allow project files to regenerate if prompted. Compile the `AstrawildCore` module using the normal UE 5.8/MSVC 2022 development target. Record the engine version, target, compiler configuration, UHT result, warnings, errors, link result and final timestamp.

This step must be performed in the Windows UE environment. There is no Unreal Editor or MSVC in the sandbox, so no sandbox result can close this gate. Do not use a static brace scan or MCP source audit as a substitute for the compile result.

After the module is compiled, run the command-line Blueprint check only as an additional gate:

```powershell
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location) -TryUnreal *>&1 | Tee-Object (Join-Path $evidence '03-Validate_Astrawild-try-unreal.txt')
if ($LASTEXITCODE -ne 0) { throw "Unreal command-line check failed or was not executed" }
```

The command uses `UnrealEditor-Cmd.exe` and `-run=CompileAllBlueprints`. It does not compile the native C++ module. Treat any UHT, link, Blueprint, asset-load or reflected-property warning that affects ASTRAWILD as a stop condition until reviewed.

## 5. Unreal Editor Python import gate

Run the importer only after the native module is available in the Editor. Use the Output Log command line or the Editor Python Console, not host Python:

```text
py "Scripts/import_all_datatables.py"
```

The importer’s source contract is:

| Import group | Source | UE destination |
|---|---|---|
| DataTables | 38 CSV files under `Content/Astrawild/Data/Source` | `/Game/Astrawild/Data/Imported` |
| FishDex | `DT_FishDex.csv`, row struct `FAstrawildFishRow`, 30 fish rows | `/Game/Astrawild/Data/Imported/DT_FishDex` |
| Racing source | `FAstrawildRacingData` and `UAstrawildRacingSubsystem`; register track/checkpoints/boost pads from authored level code or Blueprint | `/Game/Astrawild/Racing` |
| Props | 9 explicit OBJ files | `/Game/Astrawild/Meshes/Props` |
| Echoes | Every `*.obj` under `Content/Astrawild/Meshes/Echoes` | `/Game/Astrawild/Meshes/Echoes` |
| Characters | Player and Alpha source OBJ files | `/Game/Astrawild/Meshes/Characters` |
| Map kit | Four compact map-kit OBJ files | `/Game/Astrawild/Meshes/MapKit` |
| Core/extended audio | WAV and MP3 files under `Audio/SFX` | `/Game/Astrawild/Audio/SFX` |
| Ambience | WAV loops under `Audio/Ambience` | `/Game/Astrawild/Audio/Ambience` |
| Music | MP3 files under `Audio/Music` | `/Game/Astrawild/Audio/Music` |

The importer uses explicit reflected row-struct names for all 38 CSVs, replaces existing import tasks, saves created assets, and fails loudly when a source file, reflected struct or expected asset is missing. It also builds or updates the source-side `DA_GeneratedAssetRegistry` when the compiled registry class is available.

Inspect these files after the run:

```text
Saved/Astrawild/DataTableImportReport.json
Saved/Astrawild/GeneratedAssetImportReport.json
Saved/Astrawild/GeneratedAssetRegistry.json
```

The required report conditions are:

| Report field/check | Required condition |
|---|---|
| `DataTableImportReport.expected_count` | `38` |
| `DataTableImportReport.imported_count` | Equals `38` |
| `DataTableImportReport.failed_count` | `0` |
| `GeneratedAssetImportReport.failed_count` | `0` for the source groups actually present |
| Generated registry | Asset exists at `/Game/Astrawild/Data/Imported/DA_GeneratedAssetRegistry` when the compiled reflected class is available |
| Imported destinations | Match the table above; no accidental project-root imports |
| Output Log | No relevant import, factory, row-struct, save or asset-load errors |

A report with `skipped` entries is not automatically a failure because the importer and registry are source-side bridges. A skipped or missing final material, skeletal rig, Sound Cue, AnimBP, Niagara system or Blueprint reference still means the corresponding production asset is incomplete.

Save the JSON reports and capture the Content Browser showing the 38 DataTables and all generated-asset destination folders. Do not treat the Output Log sentence alone as evidence.

## 6. Underwater Editor integration gate

The repository now includes `DT_UnderwaterZones.csv`, `FAstrawildUnderwaterZoneRow` and `UAstrawildUnderwaterSubsystem`. After the native module compiles, import the table with `py "Scripts/import_all_datatables.py"` and confirm `DT_UnderwaterZones` appears under `/Game/Astrawild/Data/Imported` with row `Underwater_AbyssalTrench`.

The subsystem provides deterministic depth/pressure/oxygen/buoyancy evaluation. It is not a complete swimming implementation. Author and test an original underwater volume or water-body setup, a CharacterMovement swimming/free-dive mode, camera post-process, collision/navigation, oxygen UI, pressure damage application, pressure-suit/mecha insulation, surface refill, low-visibility lighting, bioluminescent coral, hydrothermal-vent VFX/audio and submerged-base actors. Bind the volume/component to the subsystem without allowing clients to write authoritative oxygen or damage state directly.

| Underwater check | Required Editor evidence | Failure meaning |
|---|---|---|
| DataTable | `DT_UnderwaterZones` asset, expected row/columns and no import errors | Reflection or importer contract is not usable in UE |
| Depth range | 100 m enters the Abyssal Trench contract; 1000 m remains clamped/valid; values beyond the configured range are handled | Zone/depth boundaries are wrong or unsafe |
| Oxygen | Submerged state drains at the configured rate; surface refills gradually; zero oxygen enters `PressureEmergency` | State evaluation is not connected to runtime movement/UI |
| Pressure | Unprotected diver receives the configured pressure hazard; protected diver does not receive that pressure damage | Equipment/authority binding is incomplete |
| Movement | Surface, swimming, diving and emergency modes transition without teleport/jitter or client-only state | CharacterMovement/volume integration is incomplete |
| Presentation | Camera, fog, coral/vent Niagara and audio respond to depth/zone | Visual/audio pass is incomplete |
| Base building | Pressurized dome, airlock, generator core and aquarium display have collision, oxygen/pressure rules and save/load identity | Submerged base content is not production-ready |

Do not claim “full 6-DOF swimming” from the subsystem alone. The runtime acceptance gate requires a playable diver/vehicle implementation in PIE with server authority and save/load evidence.

## 7. Safe scaffold gate

Run the scaffold from the Unreal Editor Python environment:

```text
py "Scripts/setup_project_assets.py"
```

The scaffold creates folders and conservative placeholder/contract assets. It does not author the final map, materials, skeletal rigs, Animation Blueprints, Niagara graphs, Widget Blueprints or Sound Cue routing.

Inspect:

```text
Saved/Astrawild/AssetScaffoldReport.json
```

The report must contain these arrays and must be reviewed individually:

| Report array | Meaning | Acceptance rule |
|---|---|---|
| `created` | New Editor objects saved by the script | Record paths and inspect them |
| `existing` | Idempotent assets found already present | Confirm they are the intended assets, not stale placeholders |
| `configured` | Objects whose supported properties were assigned | Confirm the MPC scalar names and values |
| `skipped` | Unsupported factory/API or an optional requirement not created | Record every reason; no final-art claim can depend on a skipped item |
| `failed` | Exception or null asset from an attempted operation | Must be empty for the accepted scaffold run, or explicitly fixed and rerun |

Expected scaffold contracts include the landscape MPC and scalar names `AW_LandscapeWetness`, `AW_RainIntensity`, `AW_WindStrength`, `AW_GrassSlopeMaxDegrees`, `AW_RockSlopeStartDegrees`, `AW_MeadowHeightMeters` and `AW_MountainHeightMeters`; eight day/night ambience Sound Cue placeholders; fifteen boss phase/ultimate placeholders; the Solar/Geo/Torrent Niagara contract names; and the exosuit VFX contract names under `/Game/Astrawild/FX/Exosuit`.

The `PhysicsAssetFactory` may be skipped when no suitable skeletal mesh is available. That is an expected source-scaffold limitation, not evidence that the Player/Echo physics assets are done. The same rule applies to any unavailable Niagara or Sound Cue factory.

## 7. Manual asset integration gate

### 7.1 Compact map

Author the acceptance map as an Editor-authored level rather than relying on runtime PrototypeArena generation. The map must contain the four zones and the route described in `Docs/VERTICAL_SLICE_MAP_20MIN_SPEC.md`:

| Zone/route element | Required Editor evidence |
|---|---|
| Dawn Spire start | Spawn point, first quest context, readable landmark and navigation |
| Resource Grove | Dawn Fiber/resource nodes, harvesting collision, route to camp |
| Rest Sanctuary | Camp/building cluster, Storage Chest and safe return point |
| Aquavine spring | Interaction actor, hydration restore, collect objective/reward path |
| Echo route | Spawn/encounter volumes for the P0 species and capture space |
| South-East Danger Pit | Blocking/arena boundary, navigation, boss staging and telegraph space |
| Return/reward path | Exit route, completion feedback, reward placement and save checkpoint |

Disable `AAstrawildPrototypeArena.bAutoGenerateOnBeginPlay` or remove the prototype actor from the final acceptance map when authored actors are present. The source Alpha phase arrays do not prove that the table-driven boss flow is active. Assign `AAstrawildBossAIController`, `DT_BossEncounters` and `DT_BossAttacks`, then verify the actual controller possession and attack/telegraph transitions in PIE.

### 7.2 Player and Echo assets

Replace or refine the source OBJ silhouettes with original production skeletal meshes. The minimum P0 set is the Player, Pyrelite, Thornback, Aquavine and Alpha Solarix. For each asset, inspect the skeleton, physics asset, material instances, collision, LOD/Nanite policy, sockets, animation retargeting and asset naming. Create Animation Blueprints for ground locomotion, attack, hit reaction, dodge/capture presentation and Alpha phase transitions. Verify that all DataTable asset paths resolve to actual Editor assets.

The full 218-row OBJ set is a catalogue coverage aid. It does not require 218 bespoke final rigs before the first compact slice is tested, but it must not be represented as final character content.

### 7.3 Materials, foliage and world presentation

Create the landscape material graph using the MPC parameter names. Verify slope and height masks, rain wetness/roughness response, puddle/reflection response, foliage interaction and scalability behavior. Create original Procedural Foliage Spawner/Volume assets and assign meshes to `DT_FoliageRules`. Inspect at least one outdoor cell in each canonical biome and record the map, Data Layer/HLOD/World Partition settings and screenshots.

### 7.4 Niagara and audio

Create original Niagara systems for Solar sparks, Geo dust, Torrent splash, capture resonance, damage feedback, boss telegraphs, exosuit beam line, overboost thruster trail, plasma-edge sparks, muzzle flash and shutdown. Bind them through the native VFX delegate boundary and verify socket names and parameter names.

Create or assign Sound Cues for melee, harvest, capture, building, cooking, level-up, Echo calls, boss transitions, exosuit actions, UI feedback and zone ambience. Configure attenuation, concurrency, looping, compression, platform settings and adaptive music routing. The imported WAV/MP3 source files and scaffold Sound Cues are not final audio integration.

### 7.5 UI and exosuit

Author the Inventory, Crafting, master HUD and Cockpit Widget Blueprints. Bind replicated inventory slots to the owning UI. Bind cockpit energy, heat, shield and target-lock state to `FAstrawildCockpitState`. Verify empty/error/loading states, controller/mouse navigation, readable contrast, safe areas and no stale state after dismount/eject.

## 8. Single-player PIE acceptance gate

Run PIE only after the native module, DataTables, map and required P0 assets are available. Record map name, PlayerController, Output Log start/end, screenshots or video, and any relevant warnings.

| Sequence | Action | Acceptance observation |
|---:|---|---|
| 1 | Spawn at Dawn Spire | Correct map spawn, HUD and `Quest.Awakening` assignment |
| 2 | Collect Dawn Fiber and harvest resources | Nodes respond once, yield is correct, capacity preflight prevents overflow |
| 3 | Craft the first Astra Resonator | Recipe/station/technology gates work and output enters inventory |
| 4 | Attempt capture | Resonator is consumed once, projectile launches, outcome resolves on the server path |
| 5 | Summon, switch and recall companion | Party state, health snapshot, follow behavior and UI remain consistent |
| 6 | Build a camp piece | Grid snap, ingredient consumption, failure refund and collision are correct |
| 7 | Use Storage Chest and save | Empty saved inventory clears stale contents; restored building identity is stable |
| 8 | Interact with Aquavine spring | Hunger/thirst/quest/reward behavior matches the configured objective |
| 9 | Food regression | In the **full-hunger** state, consuming tracked food must leave the item; below max hunger consumes exactly one item and applies thirst/buff/freshness state |
| 10 | Save/load food state | Spoilage, tracked quantity, refrigeration and active buff state restore once |
| 11 | Travel to Danger Pit | Route, navigation, encounter volumes and zone audio/VFX are active |
| 12 | Trigger Alpha encounter | Boss controller owns the encounter, telegraphs and phase thresholds are table-driven |
| 13 | Return and save | Reward/quest state persists and no duplicate bootstrap actors appear |

A single successful start or a screenshot of a map is not enough to close the PIE gate. The full first-loop sequence must be recorded, and any skipped step must remain marked pending.

## 9. Network PIE acceptance gate

Network PIE is a separate gate and must not be inferred from static RPC declarations. Start a **two-player network PIE** session after the single-player gate. Use one server and at least one autonomous client. Record server/client logs, role/authority observations and screenshots of both views.

| Surface | Test | Required observation |
|---|---|---|
| Inventory | Client requests add/remove/move/split/clear | Client does not mutate authoritative slots locally; server validates and replicated slots update the owning UI |
| Capture | Client throws Resonator, summons and recalls | Item consumption, projectile spawn, party state and outcome are server-owned; no duplicate capture or client-only companion exists |
| Mecha | Client equips/ejects, flies, overboosts and fires | Server-owned state reaches the client; energy/heat/shield and presentation do not diverge |
| Boss | Client joins an active telegraph/phase transition | Boss attack selection and phase state are server-authoritative and telegraph/audio presentation reaches both clients |
| Persistence | Save/load from the authoritative context | No client writes a competing save state and restored state is consistent |

The inventory, capture and mecha components now contain source-level replication/RPC bridges, but this does not make combat, attributes, buildings, quests or generic character state co-op-complete. Test those systems separately and keep them marked as gaps until deliberate replication and runtime evidence exist.

## 10. Performance and package gate

No FPS, frame-time, memory or GPU number is valid without a Windows profiling capture. Measure at least one outdoor cell, one worker-heavy Rest Sanctuary base and one VFX-heavy Danger Pit. Record scalability preset, resolution, game thread, render thread, GPU, memory, actor count, network role and any hitches.

Only after source validation, C++/Blueprint compile, import, asset integration, single-player PIE and the required Network PIE scope pass should the **Development cook/package** run:

```powershell
.\Tools\Package_Astrawild.ps1 -ProjectRoot (Get-Location) -PackageDirectory (Join-Path (Get-Location) 'Builds\WindowsDevelopment') *>&1 | Tee-Object (Join-Path $evidence '04-package-transcript.txt')
if ($LASTEXITCODE -ne 0) { throw "Development package failed" }
```

The package wrapper runs the host validation first, then the optional Unreal command-line Blueprint check and `RunUAT BuildCookRun` with Win64 Development, build, cook, stage, pak and archive options. Inspect the resulting archive, launch the executable, open the intended map, test save/load once, and record the package directory, executable/archive hash, engine version, map name, warnings, errors and launch result in `Docs/BUILD_STATUS.md`.

A package command completing without errors is not sufficient if the executable was not launched or the map was not inspected. A successful host validator run is not a package result.

## 11. Evidence folder and BUILD_STATUS update

Keep evidence under `Saved/Astrawild/WindowsEvidence` or another reviewed external evidence folder. At minimum, retain:

| Artifact | Purpose |
|---|---|
| `01-preflight-transcript.txt` | Hash, branch, clean/dirty state and host context |
| `02-Validate_Astrawild-host.txt` | PowerShell host-only validation result |
| `03-Validate_Astrawild-try-unreal.txt` | Optional command-line Blueprint check result |
| `DataTableImportReport.json` | 38 DataTable import result |
| `GeneratedAssetImportReport.json` | Generated mesh/audio import result |
| `GeneratedAssetRegistry.json` | Source-side imported asset registry |
| `AssetScaffoldReport.json` | Created/existing/configured/skipped/failed scaffold result |
| UE compile log | Native module/UHT/link result |
| PIE notes/screenshots/video | First-loop runtime evidence |
| Network PIE logs/screenshots | Server/client authority and replication evidence |
| Profile captures | Outdoor/base/Danger Pit performance evidence |
| `04-package-transcript.txt` and hashes | Cook/package/launch evidence |

Update `Docs/BUILD_STATUS.md` after each gate, preserving separate rows for source validation, C++ compile, Editor import, asset authoring, PIE, Network PIE, profiling and package. Never replace `NOT RUN HERE` or `PENDING` with `PASS` without the corresponding Windows artifact.

## 12. Known boundaries and troubleshooting

The importer can create DataTables and import source OBJ/WAV/MP3 assets, but it cannot infer final skeletal rigs, retargeted animation, material graphs, Niagara emitter design, Sound Cue routing, UMG layout or level composition. The scaffold can create placeholders and folders, but a placeholder object is not final production content.

If a reflected row struct cannot be loaded, stop and fix the native module/exported struct name before retrying import. If an OBJ imports with an unexpected scale, normals, collision or material slot, fix the source or import settings and record the result; do not silently accept a broken mesh. If a Sound Cue or Niagara factory is skipped, record the exact UE Python API limitation and author the asset manually in Editor.

If the boss does not enter the table-driven flow, inspect GameMode/controller possession, `DT_BossEncounters`, `DT_BossAttacks`, Alpha actor class and DataTable assignments. Do not claim automated boss combat from Alpha phase arrays alone.

If Network PIE shows duplicate items, client-side party changes, stale inventory UI or divergent mecha values, stop the co-op gate and capture the server/client logs. The source-level RPC bridge is a contract to verify, not a substitute for latency, ownership and replication testing.

The final status before Windows evidence is:

> **source/config contract prepared; UE binary content and runtime verification pending**

No source script can create final original 3D meshes, recorded/finished audio production, a complete World Partition map, final Animation Blueprints, Niagara graphs, UMG Widget Blueprints, PIE evidence, Network PIE evidence, profiling evidence or a packaged executable in this repository-only environment.
