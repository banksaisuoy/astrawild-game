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

## 6. WorldBootstrapper — The Shattered Vale (six zones, Batch 7)

`AAstrawildWorldBootstrapper` (spawned by the GameMode at world begin, server only). Deterministic via
`FRandomStream(GameState->WorldSeed)`.

| Parameter | Default | Note |
|---|---|---|
| `TerrainResolution` | **128** quads/side (≈ 6.25 m spacing per 800 m tile) | 16–256, ~196k tris world-wide |
| `ResourceNodeCount` | **21** | Dawn Fields camp ring: Wood/Stone/Fiber; outer zones use the per-zone table |
| `WildEchoCount` | **8** | Dawn Fields camp ring round-robin; outer zones use the per-zone table |
| `HostileCount` | **2** | Gloomfangs in the Dawn Fields wilds; Hollow Approach adds its own |

Build order (`BeginPlay`): `BuildLighting` → `BuildTerrain` (six zone tiles + camp `APlayerStart`) →
`ScatterResourceNodes` (camp ring + per-zone signature materials) → `SpawnWildEchoes` / `SpawnHostiles`
(camp ring + per-zone species) → `SpawnPointsOfInterest` (camp at Dawn Fields center; dungeon in the
Hollow Approach) → `BuildZoneLandmarks` (per-zone silhouettes + tinted flicker lights).

**Batch 7 — The Shattered Vale**: the world is now 2.4 km × 1.6 km split into six 800 m zones
(Dawn Fields / Dusk Marsh / Glimmerwood / Ember Ridge / Frostveil Expanse / Hollow Approach), each with
its own terrain profile (`AAstrawildTerrainTileActor` height field), wildlife, resources, landmarks and
signature colored light. See **`Docs/ASTRAWILD_ZONE_WORLD.md`** (the canonical zone doc) and
`UAstrawildZoneSubsystem` for the zone table, events and discovery persistence.

- **Lighting rig** (spawned engine actors): DirectionalLight (sun) at pitch −50°, yaw 30°, intensity 8 lux,
  movable; SkyLight intensity 1.5; SkyAtmosphere; ExponentialHeightFog.
- **Terrain**: six `AAstrawildTerrainTileActor` tiles (ProceduralMeshComponent, analytic normals,
  biome vertex tints, complex collision) built from the pure global height field
  `EvalWorldHeight(XY, Seed)`; zone weights (~60 m falloff) keep borders seam-continuous.
- **Camp PlayerStart** at the Dawn Fields center (0, −40000, GroundZ+120).
- **Resource nodes**: camp ring 1.6–6.8 km radius + per-zone signature table (marsh fiber/wood,
  Glimmerwood wood/crystal, ridge stone/ember-ash/crystal, snow stone/crystal, Hollow stone/crystal).
- **Wild Echoes**: camp ring + per-zone species (see ZONE_WORLD §2.3); Auroraling seeds deep in
  the Glimmerwood.
- **Hostiles**: Gloomfang camp wilds + 2 more in the Hollow Approach.
- **Camp layout** (radius 900 cm around the Dawn Fields center):

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
| Per-zone weather states | Global weather only — e.g. permanent Fog in the marsh is future work (Batch 7 closed the biome/terrain gap: six-zone procedural world, see `ASTRAWILD_ZONE_WORLD.md`) |
| Zone-conditional quest objectives (`VisitZone`) | Zone events publish on the bus; no objective type consumes them yet |
| Landscape / World Partition editor assets | OPTIONAL editor path provided: `Content/Heightmaps/*.r16` + import guide (runtime world needs none) |
| Rest points persisted through LoadWorld (v2 path) | v1 payload keeps rest-point data; `LoadWorld` (v2 orchestration) does not respawn rest points — bootstrapper recreates them per session |
| Dynamic spawn/despawn from population simulation | NOT IMPLEMENTED (fixed bootstrapper population) |
