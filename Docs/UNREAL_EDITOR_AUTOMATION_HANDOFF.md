# ASTRAWILD Unreal Editor Automation Handoff

**Target:** Unreal Engine 5.8 on Windows  
**Project root:** `C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game`  
**Branch:** `release/vertical-slice-v1`

> These scripts must be run inside the Unreal Editor Python environment after the `AstrawildCore` module has compiled. A successful static Python syntax check in Linux is not proof that Unreal's Python API accepted the calls.

## 1. Pull and preflight

From the project root, pull the branch and run the source validators before opening the Editor:

```powershell
git fetch origin release/vertical-slice-v1
git checkout release/vertical-slice-v1
git pull --ff-only origin release/vertical-slice-v1
python Scripts/validate_content_contracts.py
python Scripts/validate_runtime_contracts.py
python Scripts/validate_generated_headers.py
python Scripts/validate_editor_automation.py
```

The editor scripts import the `unreal` module, so do not run them with a normal system Python interpreter. Use the Unreal Editor Python Console or execute them from the Editor's Output Log command line.

## 2. Import source DataTables

Run:

```text
py "Scripts/import_all_datatables.py"
```

The importer reads `Content/Astrawild/Data/Source/` and maps every current CSV explicitly to its reflected native row struct. The current mapping contains **19 tables**: the original source tables plus `DT_BossEncounters.csv`, `DT_BossAttacks.csv`, and `DT_FoliageRules.csv`.

| Check | Expected result |
|---|---|
| Destination | `/Game/Astrawild/Data/Imported` |
| Source files | 19 mapped CSVs, no missing file errors |
| Row structs | Native `FAstrawild...` reflected names, including boss and foliage rows |
| Reimport behavior | Existing DataTables are replaced through `AssetImportTask.replace_existing=True` |
| Save behavior | Imported assets are saved after the task completes |
| Report | `Saved/Astrawild/DataTableImportReport.json` |

The script fails loudly when the source directory is missing, a CSV is missing, a reflected row struct cannot be loaded, or an expected asset is not created. Inspect `imported_count`, `failed_count`, and the `failed` array in the JSON report. Do not report success from an Output Log line alone.

## 3. Create the safe asset scaffold

After the module is loaded, run:

```text
py "Scripts/setup_project_assets.py"
```

The script creates project folders and conservatively attempts the following Editor objects:

| Family | Expected scaffold |
|---|---|
| Landscape | `MPC_AstrawildLandscape`, `MI_AstrawildLandscape`, and placeholder material |
| MPC scalar parameters | `AW_LandscapeWetness`, `AW_RainIntensity`, `AW_WindStrength`, `AW_GrassSlopeMaxDegrees`, `AW_RockSlopeStartDegrees`, `AW_MeadowHeightMeters`, `AW_MountainHeightMeters` |
| Physics | `PA_AstrawildPlaceholder`; may be skipped because a real skeletal mesh is required |
| Niagara | `NS_LandscapeEmissive`, `NS_SolarSparks`, `NS_GeoDust`, `NS_TorrentSplash`, plus a generic placeholder |
| Ambient audio | Eight day/night Sound Cue placeholders under `/Game/Astrawild/Audio/Ambience` |
| Boss audio | Fifteen phase-one/phase-two/ultimate placeholders under `/Game/Astrawild/Audio/Boss` |
| Report | `Saved/Astrawild/AssetScaffoldReport.json` |

The script is idempotent for existing assets: it records them in `existing` rather than creating duplicates. It records unsupported factories in `skipped` and exceptions in `failed`. A placeholder `SoundCue`, Niagara system, or material instance is not final production content and should not be used as proof of completed visuals or audio.

## 4. Manual Editor integration after scaffold

Assign `/Game/Astrawild/Art/Materials/Landscape/MPC_AstrawildLandscape` to a world or landscape manager actor containing `UAstrawildLandscapeMaterialComponent`. Author the landscape material graph using the exact scalar parameter names in the table above. Verify that slope masks blend grass to rock and height masks blend meadow to mountain to snow. Bind wetness to roughness, puddle/reflection response, and a rain accumulation mask without making the surface fully metallic.

Create original Procedural Foliage Spawner/Volume assets under `/Game/Astrawild/World/Foliage`, assign original meshes to the `FoliageAssetId` contracts in `DT_FoliageRules`, and preview each canonical biome. The table provides placement bands and response intent; it does not manufacture meshes or final WPO interaction by itself.

Replace the audio placeholders with original or license-documented Sound Waves/Cues. Ensure all registry paths in `UAstrawildAudioSubsystem` resolve. Connect authoritative boss transitions to `EnterBossCombat`, phase changes to the appropriate phase index, and encounter completion/reset to `ExitCombat`.

## 5. Evidence required from Windows

Attach the exact Output Log and screenshots showing the imported DataTables under `/Game/Astrawild/Data/Imported`, the MPC parameters, the foliage preview, and the resolved ambience/boss audio assets. Then compile the module and all Blueprint children. Run standalone PIE for at least one rainy outdoor cell and one tower encounter. Run two-player network PIE to confirm boss state/telegraph authority and audio transitions on each client. Finally run Development cook/package and record the executable/archive path, map, warnings/errors, and hash in `Docs/BUILD_STATUS.md`.

## 6. Known boundaries

No source script can create final original 3D meshes, textures, authored landscape material graphs, finished Niagara emitters, recorded music, or a complete World Partition map in this repository. The source package is ready for Editor authoring, and the reports make skipped/failed operations visible. Until the Windows evidence exists, the honest status remains **source/config contract prepared; UE binary content and runtime verification pending**.
