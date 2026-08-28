# ASTRAWILD Sprint 3 Handoff: Disasters, World Kaiju and Multi-Terrain Vehicles

**Target:** Unreal Engine 5.8, `AstrawildCore`, branch `release/vertical-slice-v1`

> This document is an operational source-to-Editor handoff. Passing repository validation does not prove C++ compilation, Blueprint compilation, asset import, PIE, Network PIE, profiling, cooking or packaging.

## Scope delivered in the repository

The source pass adds `UAstrawildDisasterSubsystem` with server-authoritative registration, deterministic random selection, duration/cooldown tracking and active-state delegates for Meteor Showers, Tornadoes, Volcanic Ash and Aurora. It is an event/rules contract; it does not spawn final Niagara, weather, damage or landscape actors by itself.

`FAstrawildWorldKaijuBossRow` and `DT_WorldKaijuBosses.csv` define exactly three original world threats: **Magmatitan**, **SkyColossus** and **AbyssalLeviathan**. Each row carries biome, level, health, phases, disaster affinity, arena tag, reward tags, encounter radius and a world-event requirement. The table does not prove a final skeletal boss, AI controller, attack graph, arena map or multiplayer encounter.

`UAstrawildVehicleComponent` and `AAstrawildVehicleBase` provide an authority-gated vehicle contract for driver ownership, replicated controls, modular Engine/Armor/Weapon/Utility slots, fuel, durability, boost, speed calculation and passenger-capable Pawn extension. `DT_Vehicles.csv` contains **12 unique vehicle types** across land/hover, armored rover, marine/submersible and atmospheric craft. `DT_VehicleParts.csv` contains exactly 12 original modular part rows covering Engine, Armor, Weapon and Utility slots. The source component is not a replacement for Chaos vehicle physics, custom CharacterMovement, water simulation, flight collision, weapon firing, inventory consumption or final vehicle meshes.

## Ordered Windows procedure

### 1. Preflight

Pull the branch, record the commit hash and confirm a clean tree before opening the Editor. Run `Tools\\Validate_Astrawild.ps1` in host-only mode. Do not proceed if a required-file gate, Python validator or generated-header check fails. Retain the complete PowerShell transcript.

### 2. Native compile gate

Compile `ASTRAWILDEditor Win64 Development` using UE 5.8 and the installed MSVC toolchain. Treat UHT errors, reflected-property errors, unresolved symbols, replication warnings, include-order failures and module load failures as blockers. In particular, inspect `FAstrawildVehicleRuntimeState`, replicated `AActor* Driver`, `TArray<FAstrawildVehicleInstalledPart>`, dynamic multicast delegate parameters and `UWorldSubsystem` lifecycle. The repository has no Windows compile evidence yet.

### 3. DataTable import gate

Run the Unreal Editor Python importer after native compilation. Confirm the report contains **38 DataTables**, including `DT_WorldKaijuBosses`, `DT_Vehicles` and `DT_VehicleParts`, each using the expected reflected row struct. Inspect `DataTableImportReport.json`, `GeneratedAssetImportReport.json`, `GeneratedAssetRegistry.json` and `AssetScaffoldReport.json`. A skipped entry must be investigated; it is not proof of a working runtime asset.

### 4. Disaster and Kaiju integration gate

Create authored event definitions or Blueprint registrations for the four disaster types. Use a server-only test map with visible debug markers first. Verify that a registered definition cannot start on a client, duplicate active events are rejected, cooldown prevents immediate restart, duration expires, and `OnDisasterStarted`/`OnDisasterEnded` fire once. Then bind each event to original Niagara, audio, post-process and gameplay effects.

For each Kaiju, create an original boss actor or Blueprint using the imported row, assign its AI controller and arena tag, and bind its disaster affinity. Test spawn distance, phase transitions, reward transaction and despawn/respawn behavior. Do not claim a Kaiju encounter is complete merely because its CSV row exists.

### 5. Vehicle integration gate

Create Blueprint children of `AAstrawildVehicleBase` for at least one representative Hoverbike, Mini-Submarine, heavy rover and aircraft. Assign the correct `VehicleTag`, tuning values and component. Build the final skeletal/static meshes, collision, sockets, camera, seat positions, VFX and audio in Editor.

Wire driver possession through server authority. Test that only the server changes `RuntimeState.Driver`, that an autonomous client sends a request rather than directly mutating state, and that clearing a stale or non-owner driver is rejected. Test all seven control fields, fuel drain, boost expiry, durability zero, repair and replicated state updates under latency.

Implement the actual movement layer separately. A Hoverbike needs hover raycasts, ground/water clearance, steering, braking and collision response. A Mini-Sub needs a water volume, buoyancy/drag, depth limit, pressure/oxygen integration and underwater camera. Aircraft needs flight-mode transitions and ceiling handling. The component’s source-level speed calculation alone is not physics evidence.

### 6. Modular Garage gate

Import `DT_VehicleParts`, create an Editor-facing garage UI and enforce slot replacement transactionally. Verify that only one part occupies each slot, incompatible vehicle tags are rejected, removed parts are returned or consumed according to the chosen economy rule, and server state replicates to all viewers. Bind Engine/Armor/Weapon/Utility parts to dynamic materials, mesh attachments, sound/VFX and actual movement/combat effects.

### 7. PIE and Network PIE evidence

Run single-player PIE for disaster lifecycle, one Kaiju encounter and four representative vehicles. Then run two-player Network PIE. Test driver handoff, controls, installed parts, fuel/durability, disaster start/stop and Kaiju event authority. Capture Output Log, screen recording or screenshots, replication observations and any packet-loss/latency settings. Do not infer multiplayer completeness from static RPC declarations.

### 8. Performance and package gate

Profile one disaster-heavy outdoor cell, one Kaiju arena and one vehicle traversal segment. Record game thread, render thread, GPU, memory and network values. Only after those tests pass, run Development cook/package with `BuildCookRun`, record package output and executable hash, then perform a clean launch smoke test.

## Stop conditions

Stop and fix the issue before expanding content if any compile/UHT gate fails, a DataTable row struct does not resolve, an event can be started or resolved by a client, a vehicle driver or part state can be mutated locally, fuel/durability diverges between server and client, a checkpoint or arena uses unvalidated client coordinates, or a package contains missing maps/assets.

## Evidence checklist

| Evidence | Required artifact | Status before Windows run |
|---|---|---|
| Source validation | Full validator transcript and clean hash | Repository-side only |
| Native compile | UE 5.8/MSVC Output Log and build result | Pending |
| DataTables | `DataTableImportReport.json` with 38 mappings | Pending |
| Disaster | Server/client lifecycle log and VFX/audio bindings | Pending |
| Kaiju | Three imported rows, AI/phase test and reward log | Pending |
| Vehicles | Four representative vehicle PIE tests | Pending |
| Garage | Part install/remove/incompatibility/replication evidence | Pending |
| Network | Two-player Network PIE transcript/video | Pending |
| Performance | Profiling captures | Pending |
| Package | Development archive, executable hash and smoke test | Pending |
