# ASTRAWILD — The Shattered Vale (World & Zones, Batch 7)

> Status: **CODE COMPLETE — compile NOT_RUN** (sandbox has no UE engine; verify on UE 5.8 +
> Antigravity target machine). This doc describes the six-zone surface world added in Batch 7,
> closing the long-standing "one flat arena" biome gap (ULTIMATE GAP `Biome/zoning`,
> GAP_ANALYSIS `Biomes` row).

## 1. What Changed

Before Batch 7 the world was a single flat 160 m × 160 m plane at the origin. Batch 7 replaces it
with **The Shattered Vale**: a 2.4 km × 1.6 km (3.84 km²) surface split into six 800 m × 800 m
zones, each with its own terrain profile, signature landmarks, colored ambient light, wildlife,
resource mix and threat rating — plus the Hollow Underlight dungeon approach integrated into the
world grid.

```
        X −120 m          X −40 m           X +40 m          X +120 m
Y +80 m ┌─────────────────┬─────────────────┬─────────────────┐
        │ FROSTVEIL       │ GLIMMERWOOD     │ EMBER RIDGE     │
        │ snow ridges     │ crystal forest  │ volcanic spires │
        │ threat 3        │ threat 2        │ threat 3        │
        ├─────────────────┼─────────────────┼─────────────────┤
Y 0     │ DUSK MARSH      │ DAWN FIELDS     │ HOLLOW APPROACH │
        │ muck wetland    │ camp + meadows  │ ash wilds +     │
        │ threat 2        │ threat 1        │ dungeon gate    │
        └─────────────────┴─────────────────┴─────────────────┘   threat 4
Y −80 m                                     ★ dungeon at (800, −40) m
```

## 2. New Systems

### 2.1 `UAstrawildZoneSubsystem` (world subsystem)

- **Static zone table** (`GetAllZones()`): six `FAstrawildZoneDescriptor`s — id, display name,
  subtitle, bounds rect, ground tint, ambient light color, terrain base/amplitude/ridge, threat level.
- **`GetZoneAt(FVector)`** — pure rect-containment lookup (first-match wins on shared borders).
  Static → HUD clients resolve locally with **zero replication**.
- **`ComputeZoneWeights(FVector2D, out float[Count])`** — smooth partition-of-unity weight field
  (~60 m falloff past each rect). This is the continuity glue of the terrain height function.
- **Server sweep** (0.5 s, mirrors the journal sweep): tracks each player pawn's zone; on
  transition publishes `Event.ZoneEntered` / `Event.ZoneLeft` (zone id as `TargetId`) and records
  **zone discovery** (`OnZoneDiscovered` broadcast + log).
- **Save/Load**: `ExportForSave` / `ImportFromSave` of the discovered-zone list through the
  additive `FAstrawildZoneSaveData` payload (schema v2, no bump).

### 2.2 `AAstrawildTerrainTileActor` (procedural terrain)

One tile per zone: `UProceduralMeshComponent`, 128×128 quads (≈ 6.25 m spacing, ~16.6k verts /
32k tris per tile, ~100k verts world-wide), engine basic-shape plugin module enabled in
`ASTRAWILD.uproject` + `AstrawildCore.Build.cs`.

**The height field is one pure global function** (`EvalWorldHeight(XY, Seed)`):

```
base    = FBM(value-noise, 512 m wavelength, 4 octaves) ∈ [−1, 1]
shapedₓ = lerp(base, 1 − 2·|base|, ridgeₓ)          // per-zone ridge blend
height  = Σᵤ wᵤ(P) · (baseᵤ + ampᵤ · shapedᵤ)       // w = zone weight field
        + 70 cm · FBM(90 m wavelength, 2 octaves)   // micro ripple
```

Because the weights form a partition of unity and the base noise is world-continuous, **adjacent
tiles sample identical border heights — no seams by construction** (verified by
`ASTRAWILD.Terrain.SeamContinuity`).

Per-vertex **biome tint** (`EvalGroundTint`): zone-tint blend → slope→rock mix (2.2× slope) →
Frostveil snow above 18 m → Ember char above 24 m → dark muck-water below Z=0 (the marsh pools).
Material: engine `DebugMeshMaterial` (vertex colors) with `DefaultMaterial` fallback — zero
project assets required.

| Zone | Base | Amp | Ridge | Result |
|---|---|---|---|---|
| Dawn Fields | 220 | 520 | 0.0 | gentle meadows (camp sits at ≈ 85 cm) |
| Dusk Marsh | −60 | 260 | 0.0 | low wetland, dips below Z 0 → muck pools |
| Ember Ridge | 500 | 2600 | 0.9 | sharp volcanic ridge, charred crests |
| Frostveil Expanse | 900 | 2200 | 0.8 | snow mountains (center ≈ 23 m) |
| Glimmerwood | 300 | 900 | 0.15 | rolling crystal hills |
| Hollow Approach | 260 | 1300 | 0.5 | jagged ash wilds before the dungeon |

### 2.3 WorldBootstrapper expansion

- `BuildTerrain()` — six tiles + camp `APlayerStart` at the Dawn Fields center **(0, −40000)**.
- `SpawnPointsOfInterest()` — camp ring (rest point / workbench / campfire / gathering / farm /
  Warden Maren / Trader Tam) relocated to the Dawn Fields center, every Z from `GroundZ()`;
  the **Hollow Underlight dungeon + portal pair** moved to the Hollow Approach center
  (dungeon at (80000, −40000, +300), entrance pad at (52000, −40000) — walkable 1.2 km east of camp).
- Per-zone wildlife table (`ZoneWildlife`): Duskmoth/Sprigling in the marsh, Voltmaw/Sprigling in
  Glimmerwood, Emberfang/Stonehide on the ridge, Rimefang/Stonehide in the snow, Gloomfang×2 in
  the Hollow Approach. **The Ancient-rare Auroraling now seeds deep in the Glimmerwood** (20 km
  center-distance rule — an earned encounter). Legacy `WildEchoCount`/`HostileCount`/
  `ResourceNodeCount` knobs keep populating the Dawn Fields camp ring exactly as before.
- Per-zone resource table (`ZoneResources`): fiber/wood in the marsh, wood/crystal in Glimmerwood,
  stone/ember-ash/crystal on the ridge, stone/crystal in the snow, stone/crystal in the Hollow.
- `BuildZoneLandmarks()` — signature silhouettes per zone, all engine basic shapes:
  dawnwood groves + glowcaps (Dawn), dead trees + muck pools + reed clusters + flickering
  teal wisps (Marsh), obsidian spires + lava mounds with **flickering ember light** (Ridge),
  ice pillars + cool rim light + snow drifts (Frostveil), crystal spires with pulsing violet
  light + glimmer trees (Glimmerwood), ash spires + charred trees + dim blood-ash light (Hollow).
  ~20 point lights world-wide, ~8 animated (`UpdateFlickerLights` in the bootstrapper tick).
- Toggles for perf/testing: `bBuildTerrain`, `bPopulateWildlife`, `bBuildLandmarks`,
  `TerrainResolution` (16–256).

### 2.4 HUD zone banner

`UAstrawildHudWidget` gained a top-center banner (title + threat, subtitle + flavor +
`Zones discovered: n/6`), refreshed at 0.3 s via the pure static lookup. First visit fires a
**"Region discovered: …"** notification. Session-local discovery on the client; the server-side
discovered set persists through the save system.

## 3. Zone Table (canonical)

| Zone id | Enum | Center (cm) | Threat | Signature light |
|---|---|---|---|---|
| `Zone_DawnFields` | `DawnFields` | (0, −40000) | 1 | warm gold (glowcaps) |
| `Zone_DuskMarsh` | `DuskMarsh` | (−80000, −40000) | 2 | teal-green (marsh wisps) |
| `Zone_Glimmerwood` | `Glimmerwood` | (0, 40000) | 2 | violet (crystal pulse) |
| `Zone_EmberRidge` | `EmberRidge` | (80000, 40000) | 3 | ember orange (lava flicker) |
| `Zone_FrostveilExpanse` | `FrostveilExpanse` | (−80000, 40000) | 3 | ice blue (rim light) |
| `Zone_HollowApproach` | `HollowApproach` | (80000, −40000) | 4 | ash red (dim) |

## 4. Save System

`FAstrawildZoneSaveData { TArray<EAstrawildZone> DiscoveredZones }` — additive payload on the
save game object (same pattern as the Batch-6 dungeon payload, schema stays v2).
`SaveWorld` exports the server subsystem's discovered list; `LoadWorld` re-imports and
re-broadcasts `OnZoneDiscovered` per zone.

## 5. Optional Editor Path — Landscape Heightmaps

`Scripts/export_landscape_heightmaps.py` (pure Python 3) ports the height function 1:1 and exports
one **505×505 .r16** per zone into `Content/Heightmaps/` (committed, seed 1337). Its self-check
verifies determinism, partition of unity, seam continuity and zone personality —
`PASS (camp h=85cm, marsh h=117cm, frost h=2347cm)`. Import per zone: Landscape, Section Size 63,
Sections Per Side 8, Import from file, actor scaled to the zone's world rect. See
`Content/Heightmaps/README.md`. **The runtime world does not require this step.**

## 6. Automation Tests (5 new — 20 total)

| Test | Verifies |
|---|---|
| `ASTRAWILD.Zones.TableIntegrity` | 6 zones, unique ids/enums, non-overlapping 800 m squares tiling the 2400×1600 m world rect |
| `ASTRAWILD.Zones.LookupCorrectness` | every zone center resolves to itself; camp outskirts = Dawn Fields; far outside = None; portal site = Hollow Approach |
| `ASTRAWILD.Zones.BlendPartitionOfUnity` | weights ≥ 0 and Σ = 1 (±0.001) at camp / four-corner meet / world corner / far outside; Dawn Fields dominates (>0.9) at camp |
| `ASTRAWILD.Terrain.HeightDeterministic` | same seed+point → identical height; different seed → different; marsh low / Frostveil high personality |
| `ASTRAWILD.Terrain.SeamContinuity` | <50 cm height delta 1 cm either side of the X=40000 and Y=0 borders at multiple sample points |

## 7. Playtest Script (target machine)

1. **First frame**: rolling meadows, camp structures on terrain (no floating/sunk props), six
   distinct horizon silhouettes; HUD banner reads `Dawn Fields · Threat 1`.
2. Walk 1.2 km east → banner flips to `Hollow Approach · Threat 4`, notification
   `Region discovered: Hollow Approach (2/6)`; ash spires + dim red light; portal pad reachable.
3. Ember Ridge at dusk: lava mounds **flicker** (animated intensity); obsidian spires block sightlines.
4. Frostveil: terrain rises ~23 m at center; ice pillars with cool light; snow-tinted crests.
5. Glimmerwood: violet pulsing crystal light; look for the Auroraling (single spawn, deep zone).
6. Dusk Marsh: terrain dips below Z 0 → dark muck pools at dips; teal wisps flicker over pools.
7. Save → quit → load: discovered-zone count survives (server list), banner state correct.
8. Console `Stat Unit` / `Stat FPS` across zones — terrain is ~196k tris world-wide; flag if
   the flicker lights or complex collision hurt frame time on the target GPU.

## 8. Honest Limitations

- Terrain material: engine debug/default material — **biome tints need the vertex-color material
  to load** (`DebugMeshMaterial`); with the fallback the terrain renders plain gray while the
  zone lighting still carries the mood. Asset pass (see ASSET_MANIFEST) replaces this.
- ~20 point lights are unshadowed by default (prototype perf choice); Lumen will pick them up.
- Dungeon rooms sit at a fixed Z (generator center + 300 cm lift) over sloped Hollow Approach
  terrain — room edges may clip the ground at extreme slopes; flattening the approach is a
  future polish item.
- Zone events publish but no quest consumes them yet (`ReachLocation` covers the dungeon portal);
  a future `VisitZone` objective type is a natural extension.
- Weather remains global — per-zone weather states (e.g. permanent Fog in the marsh) are future work.
