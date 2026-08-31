# CP-07 — MATERIALS: Landscape, Metal, Armor, Crystal, Water & Hologram

**Goal:** a compact, consistent master-material family — palette-parameterized
(so per-zone/per-tier variants are cheap MIs, not new shaders), stylized-realistic
between ARK's grounding and Subnautica's readability.

**Global shading rules:** ACES, exposure clamped (PPV live); no pure-black albedo
(min 0.05); emissive never pure white except Rail; ORM workflow (packed textures);
subsurface only on foliage + Echo bands. Texture budget: 2048 for hero surfaces,
1024 tiling, 512 detail macros.

---

## 1. Landscape — `M_AW_Landscape_SciFiFrontier`

| Layer | Map | Notes |
|---|---|---|
| Grass top | slope < 25° | chlorophyll family per-zone tint |
| Cliff granite | slope > 35° | **triplanar**, no UV stretch on slopes |
| Loam valleys | height band | fertile dark soil |
| Sand/reef | Z < sea level | wet darkening near waterline |

Distance-based macro texture (2048) blends tiling repetition away; runtime params:
`Wetness` (weather), `SnowAmount` (Frostveil/Stormcrest), `ZoneTint` (linear, defaults
from per-zone C++ ground tints). Heightfield: use the exported .r16 maps (12 zones,
self-checked PASS).

## 2. Metal

| Material | Use | Signature |
|---|---|---|
| `M_AW_Metal_Scrap` | Kinetic weapons, Field gear | scratched, paint-worn, amber strap cloth |
| `M_AW_Metal_Plate` | Mk II/III armor | brushed directional + edge wear in ORM |
| `M_AW_Metal_Ancient` | ruins, dungeon gates, Astralforged | dark alloy + Light `#FDF9DD` circuit veins emissive param |
| `M_AW_Metal_Coolant` | Plasma/Arc weapons | cold sheen + family-tint energy channel |

Params: `TintArmor`, `ScratchAmount`, `EmissiveColor/Intensity` (palette-fed).

## 3. Armor — CP-01 §3 lists the six armor materials

Additional shared: `M_AW_Armor_Master` (plate base: ORM + tint + emissive seams +
`Wetness`) — the five armor materials become MIs where their recipes fit.

## 4. Crystal

`M_AW_Crystal_Master`: translucent core + fresnel rim + emissive veins.
| MI | Color | Use |
|---|---|---|
| `MI_AW_Crystal_Astraite` | Pulse `#A2F6EF` | Dawn Fields, Verdant Reach |
| `MI_AW_Crystal_Pyronite` | Ember `#FFB87C` | Ember Ridge |
| `MI_AW_Crystal_Voltine` | Pulse bright | Glimmerwood arcs |
| `MI_AW_Crystal_Frost` | Frost `#C4F1FD` | Frostveil |
| `MI_AW_Crystal_Hidden` | 50% opacity + shimmer | hidden veins pre-scan |

Crystals pulse 0.6 Hz emissive ±20%; harvestable feedback: on harvest the C++ node
actor dims the mesh — keep a `Depleted` scalar param.

## 5. Water

`M_AW_Water_Master`: depth fade (scene color absorption), 2-layer normal scrolling,
refraction offset, shoreline foam mask by depth, `WaveAmplitude` ×storm, `ReefTint`
MI `#9FD8D2` for Pearlsea shallows. The 3 walkable water planes bind it directly.

## 6. Hologram

`M_AW_Hologram_Master`: fresnel additive, scanline texture, palette `EmissiveColor`,
flicker param. Users: POI beacon cones (type-tinted), scanner signature motes,
shop/dialogue screen backings (the pure-C++ UMG can adopt a UI material instance —
post-launch), research-tree hologram nodes (CP-10 §3).

## 7. Foliage & Echo (cross-links)

- `M_AW_Foliage_Leaf/Branch` — CP-04 §foliage.
- `M_AW_Echo_Skin_Master`: two-tint vertex-color blend (species `PrimaryTint`/
  `SecondaryTint` already vertex-painted by the procedural path — real meshes carry
  the same vertex colors), `EmissiveBand` scalar (element glow), 2-sided limb membranes.

## Acceptance

- [ ] Every MI derives from a master (≤ 8 masters total in this pack).
- [ ] Landscape shows all 4 layers correctly at 150 m (macro blend kills tiling).
- [ ] Crystals/ancient veins emissive-respond to palette values (change one MI →
      the whole vein family changes).
- [ ] Water foam line visible at Pearlsea shore; storm amplitude visibly raises waves.
- [ ] Compile check: shader complexity ≤ engine guidance; no translucent overdraw
      beyond crystal/water/hologram whitelist.
