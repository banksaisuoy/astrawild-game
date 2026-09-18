# Design/ — Traced Design Data (Master Directive v1, P0-T0.1)

## What lives here

| File | Purpose |
|---|---|
| `design_data.json` | Every authored design value extracted from C++ source by `Scripts/extract_design_data.py`. **Generated — do not hand-edit.** Re-run the extractor after any Source/ change. |

## The one rule (Master Directive v1 — R1)

**No number in the design layer is hand-typed.** Every value in
`design_data.json` carries a full source trace:

```json
{
  "file": "Source/AstrawildCore/Public/AstrawildSurvivalComponent.h",
  "line": 49,
  "class": "UAstrawildSurvivalComponent",
  "name": "HungerDecayPerSecond",
  "value": 0.083,
  "category": "ASTRAWILD|Survival|Rates",
  "clamp": { "ClampMin": "0.0" }
}
```

If a design conversation needs a number, it must quote the trace. If the
number you want is not in this file, it does not exist yet — change the C++
source and re-run the extractor (never edit the JSON).

## Schema `astrawild-design-data/1`

```
{
  schema, generated, generator, repo_head, rule,
  domains: {
    tunables:  all UPROPERTY(...) default-valued members in Public/*.h
               (574 at generation time) — per-entry: class, name, type,
               value, category, clamp, comment, file, line.
    bestiary:  the 204-row FBestiaryRow species table in
               AstrawildBestiaryData.cpp — identity, family/body/size,
               element/weakness, home zone, personality/activity, stats
               (HP/ATK/DEF/Speed/CaptureDifficulty), tint colors, food,
               loot, work types, sight radius + aggregates.
    items:     all 49 RegisterItem(...) definitions in
               AstrawildContentLibrary.cpp — id, name, category, weight,
               max stack, plus every authored attribute setter line
               (FoodValue, AttackPower, PerishableSeconds, ...).
    recipes:   all 32 RegisterRecipe(...) definitions — inputs, outputs,
               duration, tech gate, station.
    counts:    headline repo facts (134 automation tests, source file and
               LOC totals) with their traces.
  }
}
```

## Regenerate + validate (sandbox-safe, no Unreal required)

```
python Scripts/extract_design_data.py
python Scripts/validate_design_data.py
```

The validator re-opens every traced file at its traced line and re-reads
the value from source — a round-trip proof that the JSON never drifted
from the code that actually compiles.

## Machine-run relationship

Nothing here needs the engine: extraction and validation are pure text
round-trips over Source/. They stay green on any machine (Windows or
otherwise) and are safe to run inside CI or the Unreal Editor Python
environment alike.
