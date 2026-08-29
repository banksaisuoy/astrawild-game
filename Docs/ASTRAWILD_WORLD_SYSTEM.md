# ASTRAWILD — World System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary sources:** `AstrawildGameState.h/.cpp`, `AstrawildTimeSubsystem.cpp`,
`AstrawildWeatherSubsystem.cpp`, `AstrawildEcosystemSubsystem.cpp`, `AstrawildEventBusSubsystem.cpp`,
`AstrawildWorldBootstrapper.cpp`

The world layer runs **server-only simulation** writing **replicated state** on `AAstrawildGameState`.
Clients read it for rendering, audio and UI.

---

## 1. Replicated World State — `AAstrawildGameState`

| Property | Default | Replication | Meaning |
|---|---|---|---|
| `TimeOfDayMinutes` | 480 (08:00) | `ReplicatedUsing=OnRep_TimeOfDayMinutes` | Minutes since midnight, 0–1439 |
| `DayNumber` | 1 | Replicated | Campaign day counter |
| `WeatherState` | Clear | `ReplicatedUsing=OnRep_WeatherState` | Current weather enum |
| `WorldSeed` | 1337 | Replicated | Procedural generation seed (client-side procedural logic can match) |

Server-only setters (`SetTimeOfDayMinutes`, `AdvanceDay`, `SetWeatherState`, `SetWorldSeed`) reject
non-authority calls with a `LogAstrawildNetwork` warning. Helpers: `GetTimeOfDayNormalized()`,
`GetTimeOfDayHours()`, `IsNight()` (hour < 5.5 or ≥ 19.5), `GetSunCycleAlpha()` (06:00→0.0, 19:00→1.0),
`GetTimeOfDayText()` ("HH:MM"). OnRep hooks are no-op placeholders for client presentation (skybox/audio).

---

## 2. Time Subsystem — Day/Night

- **Rate: `MinutesPerRealSecond = 1.0` → 1 real second = 1 in-world minute → a full day is 24 real minutes.**
- Server tick accumulates fractional minutes; whole minutes are applied to the GameState with day-carry
  (1440-minute wrap increments `DayNumber`).
- Broadcasts (server): `OnHourChanged(hour)` at each in-world hour boundary; `OnDayChanged(day, bAutomatic)`
  on wrap (or via `AdvanceDays` debug).
- Debug/cheat: `SetTimeOfDay(hour, minute)` (clamped 0–23 / 0–59), `AdvanceDays(n)` — both server-only.
- Consumers: Echo activity windows (Diurnal 05:30–19:30, Nocturnal otherwise, Crepuscular 05–08 & 17–20:30),
  capture activity bonus, sun cycle, hostile spawn theming (Gloomfang = nocturnal).

---

## 3. Weather Subsystem

- **8 states** (`EAstrawildWeatherState`): Clear, Cloudy, Rain, HeavyRain, Storm, Fog, Heat, Cold.
- **Transition cadence: every `WeatherChangeIntervalMinutes = 90` in-world minutes** (= 90 real seconds at
  default time rate) a weighted roll picks the next state.
- **Weighted pick, repeat-suppressed**: each state's `SelectionWeight` below; the *current* state's weight
  is multiplied by **0.25** to discourage back-to-back repeats.
- State writes go through `GameState->SetWeatherState` (server); `OnWeatherChanged(new, old)` broadcasts.
- Debug/cheat: `ForceWeather(state)` (`AW.SetWeather <name>`).

### Profiles (`FAstrawildWeatherProfile`, from AstrawildWeatherSubsystem.cpp)

| State | Temperature offset (°C) | Selection weight | Visibility multiplier |
|---|---|---|---|
| Clear | **+2.0** | 3.0 | 1.00 |
| Cloudy | 0.0 | 2.5 | 1.00 |
| Rain | **−4.0** | 1.8 | 0.85 |
| HeavyRain | **−7.0** | 1.0 | 0.70 |
| Storm | **−9.0** | 0.6 | 0.55 |
| Fog | −3.0 | 1.2 | **0.45** |
| Heat | **+10.0** | 1.0 | 1.10 |
| Cold | **−12.0** | 0.8 | 1.00 |

Consumers of the profile: player temperature (survival = 20 °C + offset), capture bonuses (species
`PreferredWeather`, e.g. Lumewisp +0.10 in Clear/Cloudy), and `GetVisibilityMultiplier()` (exposed for
future perception scaling — **not yet wired into sight radii**; honest gap).

---

## 4. Event Bus

`UAstrawildEventBusSubsystem` (world subsystem, Game/PIE only). Payload
`FAstrawildGameplayEvent { EventTag, Instigator, TargetId, Amount, Location }`.
`Publish` rejects invalid tags; `OnGameplayEvent` broadcasts to all subscribers (quests today; journal,
ecosystem, future UI/audio can subscribe). See Architecture V2 §6 for the publisher list.

---

## 5. Ecosystem Subsystem — LOD + Population

- Echoes **register on spawn / unregister on EndPlay** (server). Weak-pointer array with stale-entry sweep.
- **Tier sweep every `TierUpdateIntervalSeconds = 1.0 s`**: distance to the nearest player pawn → tier.

| Tier | Distance | Simulates | Recommended interval (`GetRecommendedUpdateInterval`) |
|---|---|---|---|
| Tier 0 — Full | ≤ **3000 cm** | Full AI/movement/needs/combat | 0.0 s |
| Tier 1 — Reduced | ≤ **8000 cm** | Reduced-rate needs + AI | 0.25 s |
| Tier 2 — Statistical | ≤ **20000 cm** | Needs decay + bookkeeping only (movement disabled) | 1.0 s |
| Tier 3 — World | beyond | Population bookkeeping | 5.0 s |

- **Population tracking** (`FAstrawildSpeciesPopulation`): per-species `WildCount` / `CapturedCount` /
  `DefeatedCount`, updated by `NotifyCaptured`/`NotifyDefeated`, with `OnPopulationChanged(DefinitionId,
  WildCount)` broadcasts. `GetWildPopulation(id)` / `GetPopulations()` for UI/world simulation.
- Echo needs decay consumes the recommended interval via an accumulator on the character; AI think LOD is
  computed (see AI doc §5 for the honest per-frame reschedule note).

---

## 6. WorldBootstrapper — Procedural Dawn Fields

`AAstrawildWorldBootstrapper` (spawned by the GameMode at world begin, server only). Deterministic via
`FRandomStream(GameState->WorldSeed)`.

| Parameter | Default | Note |
|---|---|---|
| `ArenaSize` | **8000 cm** (half-size → 160 m × 160 m arena) | ground plane scale 80× |
| `ResourceNodeCount` | **26** | cycled Wood/Stone/Fiber; 2 per harvest, 3 harvests, respawn timer |
| `WildEchoCount` | **9** | round-robin Lumewisp / Stonehide / Duskmoth |
| `HostileCount` | **2** | Gloomfangs |

Build order (`BeginPlay`): `BuildLighting` → `BuildGround` → `ScatterResourceNodes` → `SpawnWildEchoes` →
`SpawnHostiles` → `SpawnPointsOfInterest`.

- **Lighting rig** (spawned engine actors): DirectionalLight (sun) at pitch −50°, yaw 30°, intensity 8 lux,
  movable; SkyLight intensity 1.5; SkyAtmosphere; ExponentialHeightFog.
- **Ground**: engine `/Engine/BasicShapes/Plane` scaled 80×, QueryAndPhysics collision, at origin; plus a
  fallback `APlayerStart` at (0, 0, 120).
- **Resource nodes**: seeded scatter within ±0.85 × ArenaSize, Z 100.
- **Wild Echoes**: scatter within ±0.8 × ArenaSize, Z 150, `InitializeFromDefinition` per species.
- **Hostiles**: scatter within ±0.9 × ArenaSize.
- **Camp layout** (radius 900 cm):

| Object | Position | Configuration |
|---|---|---|
| Rest point | (+900, 0) | full restore on interact |
| Crafting station | (0, +900) | `StationId = Station_Workbench` |
| Crafting station | (0, −900) | `StationId = Station_Campfire` |
| Work site — Gathering | (−900, 0) | outputs `Item_Fiber`, 10 s/output, no power |
| Work site — Farming | (−630, +630) | outputs `Item_Berry`, 14 s/output, no power |

- **Sun cycle tracking** (bootstrapper tick, 0.25 s): pitch lerp −5° (06:00 sunrise) → −175° (19:00 sunset)
  on `GetSunCycleAlpha()`; intensity = 0.4 at night, else lerp(0.8 → 9.0) across the arc. Runs on authority
  or standalone.

---

## 7. Not Implemented (honest)

| Feature | Status |
|---|---|
| Weather visibility → perception scaling | API exists (`GetVisibilityMultiplier`), NOT yet applied to sight radii |
| Weather-driven VFX/audio | NOT IMPLEMENTED (state replicates; presentation hook is a no-op OnRep) |
| Landscape / World Partition / real biomes | NOT IMPLEMENTED (bootstrapper arena) |
| Rest points persisted through LoadWorld (v2 path) | v1 payload keeps rest-point data; `LoadWorld` (v2 orchestration) does not respawn rest points — bootstrapper recreates them per session |
| Dynamic spawn/despawn from population simulation | NOT IMPLEMENTED (fixed bootstrapper population) |
