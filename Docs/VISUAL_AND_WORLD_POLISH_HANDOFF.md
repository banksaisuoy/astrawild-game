# ASTRAWILD Visual and World Polish Handoff

**Target:** Unreal Engine 5.8, private branch `release/vertical-slice-v1`  
**Scope:** Lumen lighting, stylized post process, landscape material contract, procedural foliage rules, ecosystem dressing, ambience and boss music routing.

> This pass supplies production-oriented source contracts, configuration presets, data tables, and Editor automation. It does **not** claim that final meshes, textures, Niagara graphs, Sound Cues, Material Graphs, Widget Blueprints, maps, or packaged builds exist until the Windows Editor report proves them.

## 1. Lighting and atmospheric baseline

`Config/DefaultEngine.ini` enables the project-level Lumen, Nanite, SkyAtmosphere, volumetric fog, volumetric cloud, bloom, and anti-aliasing baseline. `Config/DefaultScalability.ini` supplies low-to-epic tiers for global illumination, reflections, volumetric fog, post process, and foliage. Low tiers disable Lumen reflections and volumetrics so the vertical slice remains testable on lower hardware; Epic enables hardware ray-traced Lumen reflections only when the platform profile supports them.

The intended Editor level dressing is a movable Directional Light with Atmosphere Sun Light enabled, one Sky Atmosphere, a Sky Light with real-time capture, Exponential Height Fog with volumetric fog, and a Volumetric Cloud actor. The cloud material should expose a time-driven cloud coverage parameter, while the world clock drives the day/night directional-light rotation. Keep the stylized grade in a Post Process Volume: saturated but controlled color grade, restrained bloom, soft depth of field for focal scenes, and motion blur disabled for gameplay readability.

The config entries are defaults, not a substitute for authoring these actors in a `.umap`. The Editor acceptance gate is a clean map compile with no missing material, shader, Niagara, or audio references.

## 2. Landscape auto-material contract

`UAstrawildLandscapeMaterialComponent` is the C++ runtime bridge. Assign the Editor-created `MPC_AstrawildLandscape` to the component on the persistent world actor or landscape manager. The material graph must use the following scalar parameters exactly:

| Parameter | Contract | Intended graph use |
|---|---:|---|
| `AW_LandscapeWetness` | 0–1 | Rain-driven wetness, puddle mask, roughness reduction and reflection response |
| `AW_RainIntensity` | 0–1 | Rain accumulation and wetness response weighting |
| `AW_WindStrength` | 0–1+ | World-aligned grass/tree wind amplitude |
| `AW_GrassSlopeMaxDegrees` | 18 | Grass mask upper slope limit |
| `AW_RockSlopeStartDegrees` | 48 | Rock/cliff mask start |
| `AW_MeadowHeightMeters` | 100 | Meadow-to-mountain height transition |
| `AW_MountainHeightMeters` | 300 | Mountain-to-snow transition |

The graph should calculate slope from the world normal, blend grass into soil/rock as slope increases, and blend meadow, rock, and snow using absolute world height in meters. Nanite cliff meshes remain a separate authored layer or landscape proxy; the C++ contract does not manufacture Nanite geometry. The component subscribes to `UAstrawildWeatherSubsystem::OnWeatherChanged`, interpolates wetness, and pushes the values to the MPC each frame. When the MPC is unassigned it logs a visible warning and does not pretend that the material is connected.

## 3. Foliage and ecosystem dressing

`DT_FoliageRules.csv` is the source of truth for 15 biome-specific placement rules. It covers wind-swayed ground cover, Sunwood, rainforest pines, berry shrubs, Lumen Stone nodes, and volcanic Obsidian Spines across the four canonical biomes. Each row defines a biome tag, rule kind, Editor asset contract ID, density, slope band, height band, wind response, character response, and optional Nanite intent.

Import it with the other source tables through `Scripts/import_all_datatables.py`, which maps it to `FAstrawildFoliageRuleRow`. In the Editor, create the corresponding Procedural Foliage Spawner/Volume assets under `/Game/Astrawild/World/Foliage`, assign original meshes, and use each table row to author a foliage type and procedural distribution. The `bRespondsToCharacters` flag is a gameplay/visual contract: the final implementation may use World Position Offset, a local interaction mask, or a dedicated interaction component, but the table alone does not create the effect.

Do not import external game assets. Every `FoliageAssetId` is an internal ASTRAWILD content key that must resolve to original or license-documented assets. Record any third-party source in `Docs/ThirdPartyLicenses.md` before adding binary content.

## 4. Audio ambience and boss battle matrix

`UAstrawildAudioSubsystem` owns a soft-asset registry and safe fallback behavior. It registers day/night ambience for Dawn Meadows, Sylvan Rainforest, Scorched Obsidian Caldera, and Glacial Zenith, plus phase-one, phase-two, and ultimate theme slots for the five original tower encounters. The registered asset paths are:

| Audio family | Registry path |
|---|---|
| Ambient day/night | `/Game/Astrawild/Audio/Ambience/SC_<Biome>_<Day|Night>` |
| Solarix Alpha | `/Game/Astrawild/Audio/Boss/Boss_SolarixAlpha_<PhaseOne|PhaseTwo|Ultimate>` |
| Miremaw | `/Game/Astrawild/Audio/Boss/Boss_Miremaw_<PhaseOne|PhaseTwo|Ultimate>` |
| Terradon | `/Game/Astrawild/Audio/Boss/Boss_Terradon_<PhaseOne|PhaseTwo|Ultimate>` |
| Stormshell | `/Game/Astrawild/Audio/Boss/Boss_Stormshell_<PhaseOne|PhaseTwo|Ultimate>` |
| First Dawn Dragon | `/Game/Astrawild/Audio/Boss/Boss_FirstDawnDragon_<PhaseOne|PhaseTwo|Ultimate>` |

`EnterBossCombat` selects the correct theme from encounter ID and phase, while `ExitCombat` fades battle audio and restores ambient mode. `PlayAmbientForBiome` selects day/night cues. Missing soft assets produce an `OnAudioFallback` event and a visible log warning; they are not silently treated as complete. The intended gameplay bridge is to call `EnterBossCombat` from the authoritative dungeon/boss state transition and call `ExitCombat` when the encounter ends or resets. The audio subsystem itself does not invent final music.

## 5. Editor automation

Run the following only after the C++ module has compiled in Unreal Editor's Python environment:

```text
py "Scripts/import_all_datatables.py"
py "Scripts/setup_project_assets.py"
```

The importer should report **19 source DataTables** after this pass: the original 16 tables plus `DT_BossEncounters.csv`, `DT_BossAttacks.csv`, and `DT_FoliageRules.csv`. The exact expected count is the mapping count printed in the Output Log; the script fails if a mapped CSV, reflected row struct, or resulting DataTable asset is missing. It writes `Saved/Astrawild/DataTableImportReport.json`.

The scaffold creates safe folders, `MPC_AstrawildLandscape`, landscape material instances, optional physics/Niagara/Sound Cue placeholders, eight ambient cue placeholders, and fifteen boss theme cue placeholders. It writes `Saved/Astrawild/AssetScaffoldReport.json`. A `skipped` or `failed` entry is actionable and must not be converted into a success claim.

## 6. Verification gates

The following offline checks are required before commit:

```text
python Scripts/validate_content_contracts.py
python Scripts/validate_runtime_contracts.py
python Scripts/validate_generated_headers.py
python -m py_compile Scripts/import_all_datatables.py Scripts/setup_project_assets.py
```

Then use the Windows runner:

```powershell
powershell -ExecutionPolicy Bypass -File Tools/Validate_Astrawild.ps1
```

The static checks validate schemas, IDs, cross-table references, reflection includes, configuration markers, and balanced source delimiters. They do not prove that Unreal can compile the module or load an asset. Windows Editor evidence must additionally include: successful module compile, the two JSON reports, imported DataTables visible in `/Game/Astrawild/Data/Imported`, MPC parameters visible in the asset, a compiled landscape material, a map with lighting actors, foliage spawner preview, audio assets resolving without warnings, standalone PIE, network PIE, and Development package/cook logs.

## 7. Current honest status

At the baseline Windows report for commit `a90a83f`, static validation was clean and Unreal binary asset count was explicitly reported as zero. This document and the associated source/config pass are preparation for the next Windows Editor run; they do not retroactively change that evidence. The branch must be re-fetched before push because Windows/Antigravity may have advanced it independently.
