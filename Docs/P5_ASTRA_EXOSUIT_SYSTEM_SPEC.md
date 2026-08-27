# ASTRAWILD: Astra Exosuit & Cybernetic Echo Fusion Specification

**Module:** `AstrawildCore`
**Primary components:** `UAstrawildMechaComponent`, `UAstrawildMechaAnimationComponent`
**Data tables:** `DT_MechaFrames.csv`, `DT_MechaWeapons.csv`, `DT_CyberneticEvolutions.csv`
**Target engine:** Unreal Engine 5.8

> This is an original ASTRAWILD high-tier system. It uses original exosuit, Echo, animation, VFX and audio concepts. No character, model, animation, sound, terminology or visual reference is copied from an external game or franchise.

## 1. System concept

An Astra Exosuit is a craftable, pilotable frame synchronized to a captured Echo through an Astra Core. The player can fight on foot, summon a cybernetic Echo companion, or enter a limited-duration exosuit mode. The system is designed around readable action-survival controls rather than a permanent transformation: energy, heat, shield integrity, hardpoint ammunition and cooldowns are all authoritative gameplay state.

The five original frame profiles are `Astra Striker`, `Aegis Heavy Assault`, `Zephyr Plasma Aerial`, `Cyber-Berserk Fusion` and `Solar Emperor Siege`. The frame classes are `LightStriker`, `HeavyAssault`, `AerialTactical`, `CyberBeast` and `SiegeFortress`.

## 2. Runtime state and authority

`UAstrawildMechaComponent` owns frame activation, flight state, overboost, energy, shield, heat and hardpoint requests. In network play, the server must validate frame ownership, technology prerequisites, energy cost, heat budget, fire interval and target data before mutating state. Clients may predict input presentation but must not authoritatively change energy, heat, shield, flight or damage results.

The current b7be0ee component is a source contract and requires a Windows UE compile review. Before calling the feature production-ready, add or verify server RPCs for equip, flight, overboost and hardpoint actions, clamp all percentages, route damage through the existing `UAstrawildCombatComponent`, and replace hardcoded weapon costs with `FAstrawildMechaWeaponRow` values.

## 3. Animation Blueprint contract

Create `ABP_AstraExosuit` as a child of the project's animation instance. The AnimBP should read the following native variables or equivalent Blueprint bindings:

| Variable | Source | Use |
|---|---|---|
| `bIsMechaActive` | `UAstrawildMechaComponent::IsInMechaMode` | Blend the exosuit locomotion set |
| `bIsFlying` | flight state delegate | Enter/exit jet flight state |
| `bIsOverboosting` | overboost state | Add thrust pose and additive camera shake |
| `CurrentHeatNormalized` | `GetHeatPercent` | Drive heat warning pose/FX threshold |
| `Velocity2D` / `VelocityZ` | character movement | Ground, hover and ascent blends |
| `AimPitch` / `AimYaw` | pilot aim source | Hardpoint and rifle aiming |
| `bBeamSaberAttack` | saber event | One-shot plasma-edge attack montage |
| `bBusterCannonFire` | hardpoint event | Cannon recoil and muzzle pose |

The state machine must contain `GroundLocomotion`, `FlightHover`, `FlightCruise`, `Overboost`, `BeamSaberAttack`, `HardpointFire`, `DamageReact` and `Shutdown`. The three required authoring targets are jet-thruster flight, plasma-edge melee and buster-cannon-style heavy firing; their final montages remain Windows Editor work and must use original assets.

Recommended sockets are `MuzzleSocket`, `ChestCoreSocket`, `ShoulderLeftSocket`, `ShoulderRightSocket`, `RearThrusterSocket` and `PlasmaEdgeSocket`. Socket names are contracts only until a real skeletal mesh is assigned.

## 4. Niagara contracts

Create original Niagara systems under `/Game/Astrawild/VFX/Exosuit/` with the following responsibilities:

| System | Contract | Runtime parameters |
|---|---|---|
| `NS_AstraBeamLine` | Beam ribbon/line with impact endpoint | `BeamStart`, `BeamEnd`, `BeamColor`, `BeamWidth`, `ChargeAlpha` |
| `NS_OverboostThrusterTrail` | Additive exhaust trail attached to rear thruster sockets | `ThrustAlpha`, `HeatAlpha`, `Velocity`, `TrailColor` |
| `NS_PlasmaEdgeSparks` | Short-lived plasma-edge arc/spark burst | `ImpactNormal`, `ArcLength`, `Intensity`, `ElementColor` |
| `NS_AstraMuzzleFlash` | Hardpoint fire burst | `MuzzleTransform`, `EnergyAlpha`, `HeatAlpha` |
| `NS_ExosuitShutdown` | Safe fallback smoke/steam cue during overheat lockout | `HeatAlpha`, `ShutdownDuration` |

The C++ layer should only publish parameters and effect tags. It must not assume that Niagara assets already exist. Missing assets must produce a rate-limited warning and a gameplay-safe fallback.

## 5. Cockpit UI contract

Create `WBP_AstraCockpit` as a child of the native cockpit widget base. It should contain:

| Element | Binding |
|---|---|
| Target lock reticle | target actor, lock state, distance and line-of-sight state |
| Energy bar | `GetEnergyPercent()` |
| Heat bar | clamped `GetHeatPercent()` |
| Shield bar | `GetShieldPercent()` |
| Overboost indicator | `bIsOverboosting` and cooldown |
| Hardpoint strip | active slot, weapon display name, ammo/energy cost |
| Flight mode badge | ground, hover, cruise, overboost, shutdown |
| Accessibility | color-safe heat warnings, text labels, scalable HUD size |

The native widget should expose state refresh functions and Blueprint events; the Widget Blueprint, reticle artwork, fonts and final animation remain Editor-authoring tasks.

## 6. Data and naming rules

`DT_MechaFrames.csv` and `DT_MechaWeapons.csv` must reference only original ASTRAWILD tags. Every `DefaultWeaponTags` entry must resolve to a row in `DT_MechaWeapons.csv`. `DT_CyberneticEvolutions.csv` must reference a real base Echo in the legacy or master Echo tables and use original result tags such as `Echo.AstraforgePyrelite`, `Echo.Voltstrider`, `Echo.SiegeTerradon`, `Echo.GalevoltVoltrix` and `Echo.TideframeLeviathan`.

Generated meshes, sounds, Niagara systems, skeletal meshes and animations are not considered production-complete until their source/provenance is recorded in `Docs/ThirdPartyLicenses.md` and the Windows Editor import/compile report is attached.

## 7. Acceptance gates

The source gate is satisfied only when Python validators, generated-header checks, importer mapping checks and `git diff --check` pass. The Unreal gate additionally requires a successful Development Editor compile, DataTable import, asset registry population, AnimBP compile with no warnings, Niagara compile with no warnings, cockpit Widget Blueprint compile, single-player PIE, two-player network PIE and a Development package smoke test. Until those logs exist, this document describes an implementation contract rather than proof of a finished playable exosuit.
