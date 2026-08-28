# ASTRAWILD Sprint 2 — Space, Guild and Dye Editor Handoff

## Purpose

This handoff covers the repository contracts added for Sprint 2. It separates server-side rules and data contracts from the Unreal Engine 5.8 work required to make the features playable and presentable.

The three source-side deliverables are `UAstrawildSpaceFlightSubsystem` with `AAstrawildLaunchPad`, `UAstrawildGuildSubsystem` with `AAstrawildGuildTotem`, and `DT_Dyes.csv` with reflected row struct `FAstrawildDyeRow`. The source package is original ASTRAWILD content. It is not a replacement for authored maps, final art, movement components, UI, VFX or network runtime evidence.

## Preflight

Run the repository validation from the project root before opening the Editor:

```powershell
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
python Scripts/validate_vehicle_contracts.py
python Scripts/validate_handoff_contracts.py
```

The expected source inventory is **38 CSV mappings**, including `DT_Dyes.csv`, plus 218 Echo source meshes, 2 character source meshes, 4 map-kit source meshes, 31 SFX, 9 ambience loops and 2 music files. These counts do not prove Unreal import or runtime behavior.

Compile the native target before importing tables or creating Blueprint assets:

```powershell
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location) -TryUnreal *>&1 | Tee-Object 'Saved\Astrawild\WindowsEvidence\03-Validate_Astrawild-try-unreal.txt'
```

Stop if UHT, compiler, linker, reflected-property or module-load errors occur. Do not continue by treating a static validator pass as a compile pass.

## Space Flight and Lunar Zenith

### Source contract

`UAstrawildSpaceFlightSubsystem` is a server-authoritative `UWorldSubsystem`. It registers launch pads, validates pilot distance on the server, tracks per-pilot flight state, advances launch progress, exposes `LowGravityScale` with a configured low-gravity scale of `0.16f`, drains cabin pressure in vacuum and raises `VacuumEmergency` when pressure drops below the configured safe threshold.

`AAstrawildLaunchPad` registers its tag, destination biome, world location, interaction radius and launch duration during `BeginPlay`, and unregisters on `EndPlay`. Its `RequestLaunch` method forwards only server-side requests to the world subsystem.

### Required Editor integration

Create an authored Lunar Zenith level or sublevel with a landing zone, launch-pad mesh, collision, interaction prompt, sky/lighting setup, navigation boundaries and a destination travel contract. Place at least one `AAstrawildLaunchPad` and configure a valid `PadTag` and `DestinationBiomeTag`. The launch pad must not rely on the runtime prototype arena for final presentation.

Create a Character or vehicle bridge that consumes `GetFlightState`, calls `UpdateFlightInput` on the server, and applies the returned velocity/state through an appropriate movement component. Do not use repeated `SetActorLocation` as the final vehicle physics implementation. For a production pass, use a custom movement mode or dedicated flight pawn with swept movement, collision response, input ownership and replicated movement.

Implement pressure equipment and damage routing through the existing Attribute/Damage systems. The subsystem reports pressure state; it does not create an oxygen tank, suit inventory item, health damage event, camera effect or UI by itself.

### Acceptance tests

| Test | Expected evidence | Stop condition |
|---|---|---|
| Launch pad registration | Output Log and Content Browser show the placed pad and valid tags | Missing tag, duplicate registration or missing destination |
| Launch proximity | Pilot outside radius cannot launch; pilot inside radius can request launch | Client-only launch succeeds without server validation |
| Launch progression | State changes `Docked → Launching → InOrbit` at the configured duration | State changes only on client or timer does not advance |
| Low gravity | Flight bridge visibly uses `0.16` gravity scale and has deterministic movement | CharacterMovement remains in walking mode or movement tunnels through collision |
| Vacuum | Pressure decreases on server and enters `VacuumEmergency` below threshold | Client can refill or suppress pressure without authority |
| Return | `InOrbit`/`VacuumEmergency` can return to `Returning` and restore cabin pressure | Return is possible from invalid state or does not reset state |
| Network PIE | Server/client observe the same authoritative flight result | Any divergence in launch, pressure or state replication |

### Boundary

The repository pass does not prove 0.16G physics, six-degree-of-freedom flight, vacuum damage, travel between maps, Lunar Zenith art, final collision, Niagara, audio, cockpit UI, PIE, Network PIE or packaging.

## Guild Warfare and 4v4 Arena

### Source contract

`UAstrawildGuildSubsystem` is a server-authoritative `UWorldSubsystem`. It registers guilds, buff nodes, guild buff levels and territory totems. Buff levels are clamped to their node maximum and prerequisite buff tags are checked before enabling dependent nodes.

The arena contract requires exactly two teams, exactly four valid members per team, server-owned match state, score updates and explicit match completion. `AAstrawildGuildTotem` registers a territory location and forwards capture requests to the subsystem after server authority checks.

### Required Editor integration

Create a Guild Warfare test map with at least two original totem actors, readable territory radii, neutral/owned material states, capture feedback, and a respawn boundary. Register two guild tags and configure a small buff tree with one prerequisite node and at least two derived effects. The source struct exposes attack, defense and gathering multipliers; the owning gameplay components must explicitly consume these values rather than assuming automatic attribute changes.

Create a 4v4 arena with two team spawn areas, barriers, spectator/reset boundary, round start gate, match timer display, score display and a server-controlled win condition. Register exactly four valid actors per team before starting the match. Route combat elimination or objective scoring into `AddArenaScore` only on the server. Do not let a client submit arbitrary score deltas.

### Acceptance tests

| Test | Expected evidence | Stop condition |
|---|---|---|
| Totem registration | Two totems appear with distinct tags and territory radii | Totem registration depends on client BeginPlay |
| Territory capture | Instigator outside radius fails; inside radius changes owner and broadcasts event | Client can capture from any distance |
| Buff prerequisite | Dependent buff cannot activate before prerequisite; levels clamp to max | Negative/over-max level accepted or prerequisite bypassed |
| Team registration | Only two teams with exactly four valid members can enter match | 3/5 members accepted or duplicate actor accepted |
| Match start | Arena starts only after both teams are complete | Match begins with one team or incomplete roster |
| Scoring | Server accepts positive score for registered active team only | Client-controlled arbitrary score or eliminated team scoring |
| Match finish | Match state closes and winner event contains the server result | Winner is inferred from untrusted client state |
| Network PIE | Both clients see the same territory/match result | Guild or arena state exists only on one client |

### Boundary

The repository pass does not prove guild persistence, account identity, party matchmaking, anti-cheat beyond the source authority guards, replicated HUD, replicated scoreboards, final PvP damage rules, respawn logic, arena navigation, VFX, UMG, PIE or Network PIE.

## Cosmetic Dyes

### Source contract

`DT_Dyes.csv` contains exactly 16 original dye rows. Each row has a unique dye tag, display name, primary and secondary linear colors, material parameter name, optional technology unlock tag, craft cost and default-unlock flag.

The table is imported through `FAstrawildDyeRow`. The row contract is data only: it does not automatically recolor a mesh or persist an equipped dye.

### Required Editor integration

Import `DT_Dyes.csv` into `/Game/Astrawild/Data/Imported`. Create a dye application component or Blueprint function library that resolves the row, validates unlock state, creates or reuses dynamic material instances, and writes the configured material parameters to armor, exosuit and saddle material slots. Keep material instances per actor and avoid modifying shared parent materials.

Create a preview widget with primary/secondary swatches, locked/unlocked state, craft cost and an apply/cancel flow. Persist the selected dye tag in the appropriate equipment/save schema only after the application transaction succeeds. Verify that an unavailable dye cannot be applied by changing a client-side widget value.

### Acceptance tests

| Test | Expected evidence | Stop condition |
|---|---|---|
| Import | `DT_Dyes` resolves to `FAstrawildDyeRow` with 16 rows | Wrong row struct or missing color values |
| Preview | Primary and secondary swatches match the row | UI uses hardcoded colors unrelated to table |
| Apply | Armor, exosuit and saddle use separate dynamic material instances | Shared material asset is permanently mutated |
| Unlock | Technology requirement blocks locked colors | Client can bypass requirement |
| Save/load | Equipped dye survives save/load and missing tags fail safely | Save contains raw material pointers or stale references |
| Performance | Preview instances are reused and do not allocate every frame | Dynamic material creation occurs in Tick |

### Boundary

The repository pass does not prove final dye materials, skeletal mesh slots, exosuit/saddle material compatibility, UMG, save migration, multiplayer cosmetic replication or packaged build.

## Evidence package

Store the following under `Saved/Astrawild/WindowsEvidence/`:

| Artifact | Purpose |
|---|---|
| `03-Validate_Astrawild-try-unreal.txt` | Native/optional command-line gate output |
| `DataTableImportReport.json` | 38-table import results, including `DT_Dyes` |
| `GeneratedAssetImportReport.json` | Mesh/audio import results |
| `AssetScaffoldReport.json` | Created/existing/configured/skipped/failed scaffold entries |
| `04-SpaceFlight-PIE.txt` | Launch, low-G, pressure and return observations |
| `05-GuildArena-NetworkPIE.txt` | Two-client territory and 4v4 observations |
| `06-Dyes-PIE.txt` | Import, preview, apply and save/load observations |
| Screenshots/video | Content Browser, DataTables, UI, map, PIE and Network PIE state |

A successful source validator or an Output Log sentence alone is not evidence that the Editor assets or runtime features are complete. Update `Docs/BUILD_STATUS.md` with exact commands, hashes, timestamps and attached artifacts only after each Windows gate is actually run.
