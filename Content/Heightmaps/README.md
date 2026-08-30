# Shattered Vale — Landscape Heightmaps (Batch 8 "The Grand Expanse")

Optional editor-side terrain source for the **twelve** surface zones (3.2km × 2.4km
world, 4×3 grid of 800m cells). The **runtime world needs none of these** —
`AAstrawildWorldBootstrapper` builds procedural terrain from the same math at play
time. These files exist so the target machine can optionally swap in real editor
Landscapes for maximum fidelity (proper LOD, Nanite, painted layers) without
re-designing the world.

## Files (seed 1337)

| File | Zone | World rect (cm) | Notes |
|---|---|---|---|
| `Zone_Frostveil_505.r16` | Frostveil Expanse (snow ridges) | X −160000..−80000, Y 40000..120000 | mountains |
| `Zone_Glimmerwood_505.r16` | Glimmerwood (crystal forest hills) | X −80000..0, Y 40000..120000 | |
| `Zone_EmberRidge_505.r16` | Ember Ridge (volcanic ridge) | X 0..80000, Y 40000..120000 | |
| `Zone_SunscarDesert_505.r16`* | Sunscar Desert (dune sea) | X 80000..160000, Y 40000..120000 | desert |
| `Zone_DuskMarsh_505.r16` | Dusk Marsh (low muck wetland) | X −160000..−80000, Y −40000..40000 | |
| `Zone_DawnFields_505.r16` | Dawn Fields (starting meadows) | X −80000..0, Y −40000..40000 | camp + Dawnstead |
| `Zone_HollowApproach_505.r16` | Hollow Approach (ash wilds) | X 0..80000, Y −40000..40000 | dungeon 1 |
| `Zone_AzureShallows_505.r16`* | Azure Shallows (shallow sea) | X 80000..160000, Y −40000..40000 | sea floor |
| `Zone_TidebreakerIsles_505.r16`* | Tidebreaker Isles (drowned peaks) | X −160000..−80000, Y −120000..−40000 | islands + Driftwood Landing + dungeon 2 |
| `Zone_Stormcrest_505.r16`* | Stormcrest Highlands (thunder peaks) | X −80000..0, Y −120000..−40000 | mountains |
| `Zone_VerdantReach_505.r16`* | Verdant Reach (deep jungle) | X 0..80000, Y −120000..−40000 | |
| `Zone_PearlseaReef_505.r16`* | Pearlsea Reef (coral cathedrals) | X 80000..160000, Y −120000..−40000 | sea floor |

\* Batch 8 zone. The three sea zones sit below the global sea level (Z = −450 cm,
see `UAstrawildZoneSubsystem::GetSeaLevelZ`); editor Landscapes need the runtime
water planes (or engine Water plugin) to read as ocean.

All exports use **seed 1337** (the default `WorldSeed`). Regenerate for other seeds:

```
python3 Scripts/export_landscape_heightmaps.py --seed <n> --resolution 505
```

## Editor import (per zone)

1. Create a Landscape per zone, Section Size 63, Sections Per Side 8 (505 verts).
2. Select the landscape > Landscape tool > Import from file > pick `<zone>.r16`.
3. Scale Z to match: the r16 range maps ±256m; world heights beyond that clamp —
   zone amplitude stays within ±34m of each zone base, so import at world scale.
4. Position the landscape actor at the zone's world rect min corner.

Determinism: same seed → identical output to the runtime terrain
(`AAstrawildTerrainTileActor::EvalWorldHeight`) — the self-check in the exporter
pins determinism, partition-of-unity blending and seam continuity.
