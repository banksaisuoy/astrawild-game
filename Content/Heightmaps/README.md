# Shattered Vale — Landscape Heightmaps (Batch 7)

Optional editor-side terrain source for the six surface zones. The **runtime world
needs none of these** — `AAstrawildWorldBootstrapper` builds procedural terrain from
the same math at play time. These files exist so the target machine can optionally
swap in real editor Landscapes for maximum fidelity (proper LOD, Nanite, painted
layers) without re-designing the world.

## Files

| File | Zone | World rect (cm) |
|---|---|---|
| `Zone_Frostveil_505.r16` | Frostveil Expanse (snow ridges) | X −120000..−40000, Y 0..80000 |
| `Zone_Glimmerwood_505.r16` | Glimmerwood (crystal forest hills) | X −40000..40000, Y 0..80000 |
| `Zone_EmberRidge_505.r16` | Ember Ridge (volcanic ridge) | X 40000..120000, Y 0..80000 |
| `Zone_DuskMarsh_505.r16` | Dusk Marsh (low muck wetland) | X −120000..−40000, Y −80000..0 |
| `Zone_DawnFields_505.r16` | Dawn Fields (starting meadows) | X −40000..40000, Y −80000..0 |
| `Zone_HollowApproach_505.r16` | Hollow Approach (ash wilds) | X 40000..120000, Y −80000..0 |

All exports use **seed 1337** (the default `WorldSeed`). Regenerate for other seeds:

```bash
python3 Scripts/export_landscape_heightmaps.py --seed <worldseed>
```

## Import (UE 5.8 editor, optional)

1. In the editor, create a **Landscape** per zone with **Section Size 63** and
   **Sections Per Side 8** (that yields the 505×505 vertex grid these files match).
2. Select the landscape → **Landscape Sculpt mode → Import from file** → pick the
   zone's `.r16`.
3. Scale/position the landscape actor to its world rect above (e.g. Dawn Fields
   origin at X −40000, Y −80000, size 80000×80000 cm).
4. r16 mapping is the engine standard: `0..65535 → −256m..+256m` (32768 = 0).

The exporter mirrors `AAstrawildTerrainTileActor::EvalWorldHeight` 1:1 (verified by
its self-check: determinism, partition-of-unity weights, seam continuity, zone
personality). The C++ runtime field remains the source of truth.
