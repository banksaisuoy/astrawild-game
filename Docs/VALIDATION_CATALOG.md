# ASTRAWILD Validation Catalog

**Project:** ASTRAWILD: Echoes of the First Dawn
**Target:** Unreal Engine 5.8 on Windows
**Purpose:** อธิบายสิ่งที่สคริปต์ validation ตรวจจริง และสิ่งที่ไม่สามารถพิสูจน์ได้

> **Core rule:** Validator ที่รันผ่านหมายถึง source/configuration contract ผ่านตาม checks ที่เขียนไว้เท่านั้น ไม่ใช่หลักฐานว่า Unreal Header Tool, C++ linker, Editor Python, DataTable factory, Blueprint, PIE, Network PIE, profiler, cooker หรือ packaged executable ผ่านแล้ว

## 1. Validation layers

ระบบ validation แบ่งเป็นสี่ชั้นเพื่อไม่ให้ check แบบหนึ่งถูกเข้าใจแทน check อีกแบบหนึ่ง

| ชั้น | เครื่องมือ | Input หลัก | ผลลัพธ์ |
|---|---|---|---|
| Syntax and file safety | `python -m py_compile`, `validate_generated_headers.py` | Python และ reflected headers | Syntax Python และ `generated.h` include guard |
| Content and runtime contracts | `validate_content_contracts.py`, `validate_runtime_contracts.py`, `validate_master_echodex.py` | CSV, C++, config | Schema, ranges, row counts, cross-table references และ master Echo shape |
| Asset/import contracts | `validate_generated_assets.py`, `validate_character_map_assets.py`, `validate_audio_pack.py`, `validate_importer_coverage.py` | OBJ/WAV/MP3, manifests, importer source | Source asset existence, metadata, count, manifest coverage และ importer destinations |
| High-risk integration and handoff | `validate_mecha_contracts.py`, `validate_vertical_slice_guards.py`, `validate_editor_automation.py`, `validate_handoff_contracts.py` | Mecha CSV/source, gameplay C++, importer/scaffold, docs/runners | Authority/RPC source markers, first-loop guards, 34 CSV mappings, report/gate alignment และ explicit UE evidence boundaries |

## 2. Required host-Python suite

Run the commands from the repository root. The order is deliberate: syntax and content checks run before integration checks, while the handoff validator runs after importer/scaffold and all validator files exist.

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
python Scripts\validate_handoff_contracts.py
```

The command list must remain synchronized with the `$pythonValidators` array in `Tools/Validate_Astrawild.ps1`. `validate_handoff_contracts.py` checks that synchronization, the source CSV count, report names, validator order, UE command markers and explicit pending/evidence language.

## 3. Validator-by-validator contract

### 3.1 Syntax and reflected-header guards

`python -m py_compile Scripts\*.py` parses all Python scripts, including host generators, validators and UE Editor scripts. It catches Python syntax errors but does not import `unreal` and therefore does not prove that an Editor API exists in UE 5.8.

`validate_generated_headers.py` recursively scans reflected C++ headers. A header containing `UCLASS`, `USTRUCT`, `UENUM` or `UINTERFACE` must include a generated header. This is an include hygiene guard only; it does not run UHT and cannot detect every Unreal reflection rule.

### 3.2 `validate_content_contracts.py`

This is the broad static content validator. It requires the source/config/docs paths, confirms that the current source set contains **34 CSV files**, validates required columns, nonempty and unique row names, and checks key counts/ranges for biomes, foliage, spawns, spires, EchoDex, mounts, breeding, traits, recipes, evolutions, weather, dungeons, bosses, ranged weapons, technology and mecha data.

It also verifies the four canonical biomes, one default fast-travel spire, Echo order 1–30, recipe/evolution/weather/dungeon/ranged row counts, boss phase thresholds, reward/ingredient array lengths, and basic C++ brace/parenthesis balance. These checks are intentionally conservative. Balanced braces are not equivalent to compiling C++.

**Pass condition:** The script prints `ASTRAWILD content contract validation passed.` and exits `0`.

### 3.3 `validate_runtime_contracts.py`

This validator checks references between tables rather than merely checking individual CSV shape. It verifies quest prerequisites/objectives, Echo traits/breeding/mount references, spawn-rule Echo references, evolution source/target/DataAsset paths, recipe technology links, dungeon reward arrays, boss encounter-to-dungeon/Echo links, boss attack encounter/phase links and fast-travel target uniqueness.

**Pass condition:** All referenced tags/IDs resolve within the source tables and the script prints `ASTRAWILD runtime contract validation passed.`

### 3.4 `validate_editor_automation.py`

This validator parses `Scripts/import_all_datatables.py` with Python AST and reads the `TABLE_MAPPING` literal. It requires an exact match between the mapped CSV filenames and the 34 files in `Content/Astrawild/Data/Source/`, then checks the expected reflected row struct for each high-risk mapping. It also checks importer markers, scaffold factory markers, Niagara/Sound Cue contract names and visual configuration markers.

It does not run `unreal.AssetImportTask`, `DataTableFactory`, `EditorAssetLibrary` or any other Editor API. Its purpose is to catch drift before the Editor is opened.

**Pass condition:** The script prints `ASTRAWILD Editor automation contract validation passed (34 CSV mappings).`

### 3.5 `validate_master_echodex.py`

This validator checks the 200-row master Echo source catalogue, uniqueness and row structure. Each row must carry the expected active-skill and work-suitability shape, including three active skills and twelve work levels according to the repository contract.

**Pass condition:** The script reports 200 unique rows with the expected skill/work structure.

### 3.6 `validate_generated_assets.py`

This is the legacy source-asset guard for the original prop/SFX set. It checks the nine explicit prop OBJ names and seven core WAV SFX names, OBJ topology/material references, WAV metadata and manifest hashes.

It does not cover the full generated Echo/map/audio expansion; those checks belong to the newer validators below.

**Pass condition:** The legacy prop and core-SFX manifest/file checks pass with exit `0`.

### 3.7 `validate_character_map_assets.py`

This validator unions the legacy and master Echo species tags and requires one static source OBJ per unique row. The current expected coverage is **218 Echo meshes**, **2 character source meshes** for Player and Alpha Solarix, and **4 compact map-kit source meshes**. It checks basic OBJ geometry/material records, manifest kind/coverage and forbidden terms in runtime asset filenames.

These are static OBJ source silhouettes. Passing this validator does not mean that the assets have skeletons, skin weights, physics assets, materials, LODs, Nanite settings, collision or `.uasset` imports.

**Pass condition:** The script prints the 218/2/4 coverage result and exits `0`.

### 3.8 `validate_audio_pack.py`

This validator checks `ASTRAWILD_AudioPack_Manifest.json` and the expanded source pack: **24 extended SFX**, **9 ambience loops** and **2 music files**. It verifies unique manifest paths, file existence, nontrivial size, positive duration, WAV metadata and MP3 metadata through `ffprobe`.

The repository also has the seven legacy core SFX files, making the current SFX directory total **31 WAV files**. The 24 count is the expanded-pack contract, while 31 is the full source-directory count.

Passing this validator does not create Sound Cues, attenuation, concurrency, adaptive music, mixing or runtime event routing.

**Pass condition:** The manifest and all expected files pass metadata checks.

### 3.9 `validate_importer_coverage.py`

This validator performs concrete checks rather than marker-only checks. It parses the importer literals, confirms the 34 CSV mapping set, checks every destination constant, counts source files for props/Echoes/characters/map kit/SFX/ambience/music, verifies the nine explicit prop and seven explicit core-SFX files, validates Echo/map manifests and confirms the expanded audio manifest counts.

It also requires the importer implementation markers for OBJ/WAV/MP3 discovery, `AssetTools.import_asset_tasks`, generated import reports, and failure handling.

**Pass condition:** The script prints the current 34 CSV, 218 Echo, 2 character, 4 map-kit, 31 SFX, 9 ambience and 2 music source counts.

### 3.10 `validate_mecha_contracts.py`

This high-risk validator checks five mecha frames, fourteen weapons, five animation profiles and five VFX bindings. It verifies duplicate/tag/reference rules, default weapon coverage, `/Game/Astrawild/VFX/` path rules, cockpit target/LOS logic, source-level authority/RPC and replicated input/state markers, and originality constraints.

Passing it means the mecha source/data contract is internally coherent. It does not prove a skeletal exosuit mesh, AnimBP, Niagara graph, Widget Blueprint, socket setup or Network PIE behavior.

### 3.11 `validate_vertical_slice_guards.py`

This is a source regression guard for the compact first loop. It checks server-only/idempotent map bootstrap, stable IDs, quest/bootstrap tags, interaction/survival restoration, inventory capacity and replicated inventory RPC markers, capture refund/server outcome/party RPC markers, building rollback, food preflight/rollback, save/load guards, PowerGrid timer/battery correction and mounted-partner skill gating.

It also checks the shared `Item.Water`, `Location.AquavineSpring` and `Echo.SolarixAlpha` tags, the Campwater objective target and required map-spec language.

It intentionally uses source tokens for some guards because no Unreal runtime exists in the host environment. Treat it as a regression tripwire, not a behavior simulation.

### 3.12 `validate_handoff_contracts.py`

This validator is the final documentation/automation consistency gate. It checks that the handoff, importer, scaffold, PowerShell runners and all validators exist; that the source directory contains exactly 34 CSVs; that `TABLE_MAPPING` equals the source CSV set; that every validator appears in both the handoff and PowerShell runner; that all four JSON reports are produced/referenced; that importer/scaffold operational markers exist; that UE compile/package command markers exist; and that the validator order is deterministic inside `$pythonValidators` rather than being confused by required-file lists.

It also checks for explicit statements that source validation does not prove C++ compilation, Blueprint compilation, PIE, Network PIE or packaging, and for explicit full-hunger, two-player network PIE and Development cook/package test language. The check rejects unqualified UE success claims in the handoff.

**Pass condition:** The script prints `ASTRAWILD handoff contract validation passed ...` and exits `0`.

## 4. Underwater source contract

The Underwater expansion adds `FAstrawildUnderwaterZoneRow`, `DT_UnderwaterZones.csv` and `UAstrawildUnderwaterSubsystem`. The source contract covers the Abyssal Trench depth range of 100–1000 meters, pressure damage without protection, depth-scaled oxygen drain, gradual surface refill, buoyancy multiplier, pressure-emergency movement mode, hazard/spawn tags and optional active-zone DataTable lookup.

The subsystem is deliberately a calculation/data contract rather than a hidden character-movement implementation. It does not replace a CharacterMovementComponent swimming mode, water-volume detection, physical buoyancy simulation, damage application, oxygen inventory/tank item, pressure-suit equipment, underwater camera, navigation, aquatic AI, submerged base actors, or Editor-authored bioluminescent/hydrothermal assets. Those responsibilities require explicit actor/component integration and UE 5.8 runtime tests. The content, Editor-automation, importer-coverage and handoff validators now require the Underwater header, source, CSV and mapping.

## 5. Fishing and Racing source contracts

`FAstrawildFishRow` and `DT_FishDex.csv` define exactly 30 fish entries with habitat/depth bounds, bait tags, catch item tags, sell price, pull strength, reel duration, safe tension window and rarity weight. `UAstrawildFishingComponent` keeps the active minigame state authoritative, selects only fish valid for the bait/depth/habitat context, clamps client reel input, breaks the line at maximum tension and checks inventory capacity before committing a catch. The component does not itself create fishing rods, water volumes, fish meshes, catch animations, sell UI or economy transactions; those remain Editor/gameplay integration work.

`FAstrawildRacingData` and `UAstrawildRacingSubsystem` define server-owned track checkpoints, lap state, participant timing and boost pads. Checkpoints require the expected order and server actor location; the reported client location is only a consistency check. Boost pads require a registered pad, server distance validation and an active participant. The subsystem does not replace CharacterMovement speed application, mount physics, race UI, track collision, respawn rules, matchmaking or leaderboard persistence.

## 5. PowerShell runner behavior

`Tools/Validate_Astrawild.ps1` checks required source/config/docs/assets, runs the Python validator array, runs Python syntax checks for the Editor/generator scripts, counts `.uasset`/`.umap` files and reports dirty Git status. It can optionally invoke `UnrealEditor-Cmd.exe -run=CompileAllBlueprints` with `-TryUnreal` and can invoke `RunUAT BuildCookRun` through `-Package`.

The runner has three meaningful outcomes:

| Outcome | Meaning |
|---|---|
| Exit `0` | The requested host checks, or requested UE/package command that was actually found and completed, returned success |
| Exit `1` or terminating PowerShell error | A required path, Python validator, compile command or package command failed |
| Exit `2` for optional UE/package mode | Unreal executable or RunUAT was not found; the UE/package gate was not executed |

A warning that the Content folder contains zero `.uasset`/`.umap` files is expected for this source-only repository pass. It is not a failure of the static source package and it is not proof of Editor readiness.

`Tools/Package_Astrawild.ps1` runs the host validation first, then invokes the optional Unreal command-line Blueprint check and Development `BuildCookRun`. It must only be used after the manual Editor asset/import and PIE gates are ready. A successful command still requires archive inspection, executable launch and BUILD_STATUS evidence.

## 5. Unreal-only evidence matrix

| Gate | Required evidence | Not replaceable by |
|---|---|---|
| Native compile | UE 5.8/MSVC Output Log or IDE result for `ASTRAWILDEditor Win64 Development` | Python, brace scans or source audits |
| DataTable import | `DataTableImportReport.json`, Content Browser paths and Output Log | CSV count alone |
| Generated asset import | `GeneratedAssetImportReport.json`, registry asset and destination inspection | OBJ/WAV manifest alone |
| Scaffold | `AssetScaffoldReport.json`, created/existing/configured/skipped/failed review | `setup_project_assets.py` syntax pass |
| Blueprint/AnimBP/Niagara/UI | Compiled assets and screenshots/paths | Contract names or placeholder factories |
| Single-player PIE | Full first-loop notes/video/screenshots and Output Log | Map screenshot or spawn success |
| Network PIE | Server/client logs and replicated state observations | RPC declarations or static mecha validator |
| Profiling | Windows captures with preset, resolution, thread/GPU/memory data | AI distance-LOD or config values |
| Package | Cook/stage/archive output, executable hash and launch smoke test | `RunUAT` exit code without launch |

## 6. Change-control rules

When a CSV schema, reflected struct, importer destination, source asset family, RPC contract or handoff command changes, update the narrow validator first and then update `validate_handoff_contracts.py` if the operational surface changed. Run the full host suite before staging. Do not weaken a validator merely to accommodate an Editor failure; fix the source contract or document a deliberate exception.

When an Editor run produces `skipped` scaffold entries, record the exact UE Python API reason and classify whether manual authoring is required. Treat `failed` entries, missing imported assets, missing row structs, wrong destinations and stale registry data as stop conditions.

Keep `BUILD_STATUS.md` separated by evidence layer. A static pass may be recorded as **Source/configuration PASS**, but C++ compile, Editor import, Blueprint, PIE, Network PIE, profiling and package rows remain **NOT RUN HERE** or **PENDING** until their Windows artifacts are attached.

## 7. Recommended one-command entry point

For routine source checks, run:

```powershell
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location) *>&1 | Tee-Object 'Saved\Astrawild\WindowsEvidence\02-Validate_Astrawild-host.txt'
```

For a deliberate UE command-line check, run only after the native module is compiled:

```powershell
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location) -TryUnreal *>&1 | Tee-Object 'Saved\Astrawild\WindowsEvidence\03-Validate_Astrawild-try-unreal.txt'
```

For the final Development package, use:

```powershell
.\Tools\Package_Astrawild.ps1 -ProjectRoot (Get-Location) -PackageDirectory (Join-Path (Get-Location) 'Builds\WindowsDevelopment') *>&1 | Tee-Object 'Saved\Astrawild\WindowsEvidence\04-package-transcript.txt'
```

Do not run the importer or scaffold with ordinary Python. Run them inside Unreal Editor after C++ compilation:

```text
py "Scripts/import_all_datatables.py"
py "Scripts/setup_project_assets.py"
```

The companion operational sequence is documented in [`UNREAL_EDITOR_AUTOMATION_HANDOFF.md`](UNREAL_EDITOR_AUTOMATION_HANDOFF.md).
