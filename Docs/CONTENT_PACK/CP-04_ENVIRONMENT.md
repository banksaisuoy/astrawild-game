# CP-04 — ENVIRONMENT: 12 Biome Asset Lists, Foliage, Rocks, Crystals, Ruins & POIs

**Goal:** each of the 12 zones reads as a distinct place within one 3.2 × 2.4 km world.
The deterministic scatter (Batch 2 dressing actor) already places PMC placeholders by
archetype with per-zone tints; this pack replaces the placeholder meshes per zone list
(soft refs already exist: `UAstrawildBiomeDefinition::TreeMeshes/RockMeshes/GrassMeshes/
LandscapeMaterial/AmbientAudio` — the dressing actor auto-upgrades to ISM real meshes
when they are bound, mixed binding allowed).

**World map (C++ zone ids):** Dawn Fields · Dusk Marsh · Ember Ridge · Frostveil Expanse ·
Glimmerwood · Hollow Approach · Azure Shallows · Tidebreaker Isles · Sunscar Desert ·
Stormcrest Highlands · Verdant Reach · Pearlsea Reef.

---

## Foliage archetypes (shared meshes, per-zone tint instances)

| Archetype (C++ scatter key) | Mesh | Tris | Used by |
|---|---|---|---|
| Broadleaf | `SM_AW_Foliage_Broadleaf` (3 variants) | 2.5k | Dawn Fields, Verdant Reach, Dusk Marsh |
| Conifer+snow | `SM_AW_Foliage_Conifer` (3 variants) | 3k | Frostveil, Stormcrest |
| Palm | `SM_AW_Foliage_Palm` (2 variants) | 1.8k | Tidebreaker, Pearlsea, Azure |
| Dead spire | `SM_AW_Foliage_DeadSpire` (2) | 1.6k | Hollow Approach, Sunscar, Dusk Marsh |
| Cactus | `SM_AW_Foliage_Cactus` (2) | 1.2k | Sunscar |
| Spire canopy | `SM_AW_Foliage_Spire` (2) | 2.2k | Glimmerwood, Ember Ridge |
| Grass tufts | `SM_AW_Foliage_Grass` (4 variants) | 120 | all land zones |
| Reeds | `SM_AW_Foliage_Reed` (2) | 150 | Dusk Marsh, Pearlsea, Azure |
| Kelp/coral fans | `SM_AW_Foliage_CoralFan` (3) | 900 | Pearlsea Reef (underwater) |

Materials: 2 master foliage materials — `M_AW_Foliage_Leaf` (subsurface wind, 2-sided,
tint param) and `M_AW_Foliage_Branch` (bark, no wind). Wind speed driven by weather
(CP-04 §6); snow zones get `bSnowDusted` scalar.

## Rocks & crystals

| Asset | Tris | Notes |
|---|---|---|
| `SM_AW_Rock_Field` ×3 | 800 | rounded pasture stone |
| `SM_AW_Rock_Cliff` ×3 | 3k | triplanar dress on slopes |
| `SM_AW_Rock_Volcanic` ×2 | 1.5k | obsidian shard look, Ember Ridge |
| `SM_AW_Rock_Sandstone` ×2 | 1.2k | Sunscar mesa chunks |
| `SM_AW_Rock_Ice` ×2 | 1.4k | translucent Frostveil boulders (`M_AW_Crystal_Water` family, CP-07) |
| `SM_AW_Crystal_Astraite` | 700 | teal emissive crystal clusters — dawn crystals |
| `SM_AW_Crystal_Pyronite` | 700 | ember emissive, Ember Ridge veins |
| `SM_AW_Crystal_Voltine` | 700 | pulse arcs, Glimmerwood |

Crystals are the 10 resource-node shapes (C++ rarity shape kits already tint them);
meshes replace the PMC kits 1:1. Hidden veins (Oracle Scanner) get 50% opacity + slight
subsurface shimmer until scanned.

## Ruins & POI dressing

| Asset | Notes |
|---|---|
| `SM_AW_Ruin_WallKit` (3 pieces) | ancient-alloy walls, CP-07 `M_AW_Metal_Ancient` |
| `SM_AW_Ruin_Arch` | landmark gates |
| `SM_AW_Ruin_Monolith` ×2 | Glimmerwood/First Light signal stones (POI ids exist) |
| `SM_AW_Ruin_TowerShell` | watchtowers (POI type Watchtower) |
| `SM_AW_POI_Beacon` | the 12 POI markers: colored beacon cores, type-tinted (C++ tints already) |
| `SM_AW_Dungeon_Gate` | Sunken Vault door — heavy, ancient alloy, 60% open animation |

## Per-zone asset lists (bind arrays on the 12 biome definitions)

| Zone (BiomeId) | Trees | Rocks | Grass/ground | Crystals | Ruins/POIs | Key tint |
|---|---|---|---|---|---|---|
| DawnFields | Broadleaf ×3 | Field ×3 | Grass ×4 | Astraite | First Light Ruin kit, Watchtower | canopy `#6A9E3F` grass `#8CCC57` |
| DuskMarsh | DeadSpire ×2, Broadleaf (dark MI) | Field (dark MI) | Reeds ×2 | Astraite (dim) | Sunkencollar Cave mouth | `#4D6B52` reeds |
| EmberRidge | Spire ×2, DeadSpire (charred) | Volcanic ×2 | sparse dry grass | Pyronite | Ember Foundry shell | `#D9532E` canopy |
| FrostveilExpanse | Conifer+snow ×3 | Ice ×2 | snow tuft | Astraite (frost MI) | frozen monolith | `#EAF7FD` |
| Glimmerwood | Spire ×2 (glow MI) | Field (moss MI) | Grass ×2 glow | Voltine | Glimmerwood Monolith, ancient tech | `#7FD9A8` |
| HollowApproach | DeadSpire ×2 | Cliff ×2 | sparse | Ash crystal (new, dim) | dungeon approach ruins | `#5A5148` |
| AzureShallows | Palm ×2 | Field (wet MI) | Reeds (short) | — | shipwreck rib set (NEW small) | `#9FD8D2` |
| TidebreakerIsles | Palm ×2, DeadSpire ×1 | Field ×2 | Grass ×2 | Astraite (shore) | Driftwood Landing kit, Vault Gate | `#8CC8B8` |
| SunscarDesert | Cactus ×2, DeadSpire | Sandstone ×2 | dry shrub ×2 | — | half-buried arch | `#E0C48A` |
| StormcrestHighlands | Conifer ×3 (stunted) | Cliff ×3 | alpine tuft | Voltine (storm MI) | weather shrine (NEW small) | `#8FA8B8` |
| VerdantReach | Broadleaf ×3 (large MI) | Field ×2 moss | Grass ×4 lush | Astraite | research outpost shell | `#4E9E3E` |
| PearlseaReef | Palm ×1 (shore only) | Coral fan ×3, Field | Reeds | Sea pearls node | coral arch ruins | `#B8647A` coral |

*(Tints above are authored conversions of the live per-zone `TreeCanopyTint/RockTint/
GrassTuftTint` linear values — keep the C++ values as the source of truth; MI overrides
match them.)*

## Landscape material (per CP-07 §1)

`M_AW_Landscape_SciFiFrontier` — 4 layers (grass top / cliff triplanar / loam / sand-reef),
distance-based macro blend; per-zone layer weights painted from the C++ heightmap blend
(export scripts already emit .r16 per zone).

## Water

3 water planes already exist (walkable stylized). Upgrade: `M_AW_Water_Ocean` (depth fade,
refraction, shoreline foam, CP-07 §5) + reef shallow variant tint `#9FD8D2`.

## Weather coupling (C++ live)

Fog/sun/sky respond to weather via `EvalAtmosphereRamp`; assets expose: foliage wind
(0–1), rock wetness (0–1), water wave amplitude ×storm. Storm Surge = wind 1.0, wet 0.8.

## Acceptance criteria

- [ ] All 12 zones identifiable from a single screenshot (no HUD).
- [ ] ISM counts within scatter budgets (Dawn Fields ≤ 2,200 instances land zone).
- [ ] Foliage wind + wetness respond to Storm Surge in < 1 s.
- [ ] Total environment texture budget ≤ 220 MB; draw calls ≤ 700 static geometry.
- [ ] Zero gameplay regressions: exclusions (camp/village/dungeon/POI) still honored —
      the scatter gates are C++ and unchanged.
