# Milestone 2 — Element Compatibility and Matrix Test Plan

## Compatibility rule

`EAstrawildElement` is serialized into Unreal properties and save data. Existing release data uses the following numeric values and they must not move:

| Serialized value | Existing meaning | Compatibility action |
|---:|---|---|
| 0 | Neutral | Preserve exactly |
| 1 | Solar | Preserve exactly |
| 2 | Torrent | Preserve exactly |
| 3 | Geo | Preserve exactly |
| 4 | Aether | Preserve as a legacy element; do not delete or reorder |

New production elements are appended after the legacy range. This avoids silently changing old Echoes, abilities, or save files when the enum is recompiled.

## New matrix contract

The production six-way loop is `Abyssal → Solar → Glacial → Geo → Volt → Torrent → Abyssal`, where the arrow means the left element is advantaged against the right element. The legacy release relationships remain valid as additional compatibility edges: `Solar > Geo`, `Geo > Torrent`, and `Torrent > Solar`. Same-element damage is resisted at `0.75x`; Neutral, legacy Aether, and Astra have no default advantage or disadvantage against other elements.

The scalar values are intentionally bounded and data-driven: **1.75x** for advantage, **0.50x** for disadvantage, **0.75x** for same-element resistance, and **1.00x** for neutral interaction. Multi-element defenders multiply each matchup and clamp the final multiplier to `0.25x–2.50x` so a malformed data row cannot produce unbounded damage.

## Required tests before claiming compile or PIE

| Test | Expected result |
|---|---|
| Static enum order audit | Values `Neutral=0`, `Solar=1`, `Torrent=2`, `Geo=3`, `Aether=4` remain unchanged |
| Six-way advantage audit | Every six-way edge returns `1.75x` and its reverse returns `0.50x` |
| Legacy edge audit | Existing Solar/Geo, Geo/Torrent, and Torrent/Solar interactions remain unchanged |
| Same-element audit | Every supported non-Neutral element returns `0.75x` against itself |
| Multi-element audit | Product is clamped to `0.25x–2.50x` and empty defense arrays return `1.0x` |
| DataAsset compatibility | Old single `ElementalAffinity` still populates primary element when new array is empty |
| Save compatibility | A v1 `FAstrawildCapturedEchoData.Element` value remains readable; new affinities are additive |
| Unreal automation | Windows Editor compiles the automation test target and reports all assertions passing |
| PIE damage smoke test | A melee hit uses the new matrix exactly once, with no duplicate multiplier in feedback/UI |

No source-only pass is evidence of Unreal compile, asset load, PIE behavior, multiplayer authority, or packaging success. Those gates remain Windows responsibilities.
