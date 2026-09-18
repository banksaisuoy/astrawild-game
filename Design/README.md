# Design/ — Traced Design Data (Master Directive v1, P0-T0.1 + L2)

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

## Final honest counts (schema `astrawild-design-data/2`, L2-complete)

The extraction now covers **every content domain** and reconciles 15/15
against the repo's authoritative census (`Scripts/validate_final_run.py`
`EXPECTED_CENSUS` — see `Docs/ASTRAWILD_COVERAGE_REPORT.md` for the L1
truth audit that forced this completeness):

| Domain | Count | Source file(s) |
|---|---|---|
| tunables (UPROPERTY defaults) | 574 | 99 Public headers |
| bestiary species | 204 | AstrawildBestiaryData.cpp |
| authored species | 25 | ContentLibrary.cpp (10) + ProductionContent.cpp (9 MakeProductionEcho + 6 evolution targets) |
| **species total** | **229** | = 204 + 25 |
| items | 78 | ContentLibrary.cpp (49) + ProductionContent.cpp (29) |
| recipes | 58 | ContentLibrary.cpp (32) + ProductionContent.cpp (26) |
| buildings | 26 | ContentLibrary.cpp |
| weapons | 8 | ProductionContent.cpp |
| technologies | 17 | ContentLibrary.cpp (10) + ProductionContent.cpp (7) |
| quests | 22 | ContentLibrary.cpp (10) + ProductionContent.cpp (12) |
| NPCs | 13 | ContentLibrary.cpp |
| dialogue trees | 13 | ProductionContent.cpp |
| loot tables | 11 | ContentLibrary.cpp (5) + ProductionContent.cpp (6) |
| world events | 16 | ProductionContent.cpp |
| POIs | 17 | ProductionContent.cpp |
| resource nodes | 10 | ProductionContent.cpp |
| work sites | 8 | ProductionContent.cpp |
| robots | 3 | ProductionContent.cpp |
| zones | 12 | AstrawildZoneSubsystem.cpp |
| weather profiles | 8 | AstrawildWeatherSubsystem.cpp |
| hunt contracts | 8 | AstrawildHuntSubsystem.cpp |
| abilities | 53 | AstrawildAbilityLibrary.cpp |
| mutation specs | 204 | AstrawildEchoMutationData.cpp |
| dungeons | 3 | AstrawildWorldBootstrapper.cpp |
| bosses | 4 | AstrawildWorldBootstrapper.cpp (3 dungeon + 1 world) |
| save schema | 29 fields + 22 record structs | AstrawildSaveSubsystem.h + AstrawildTypes.h |
| automation tests | 134 | AstrawildAutomationTests.cpp |

Every entry — including each item/species/quest attribute setter line —
carries `{file, line}`. Evolution-target stats are runtime-derived by
formulas (traced to the loop, never invented): their spec rows carry the
literal level/bond gates only.

## Schema `astrawild-design-data/2`

```
{
  schema, generated, generator, repo_head, rule,
  domains: {
    tunables, bestiary, species, items, recipes, buildings, weapons,
    technologies, quests, npcs, loot_tables, world_events, pois,
    resource_nodes, work_sites, robots, dialogue_trees, zones, weather,
    hunt_contracts, abilities, mutations, dungeons, bosses, save_schema,
    counts: {
      automation_tests, source_files, source_loc,
      census_vs_validate_final_run: {<metric>: {expected, extracted, match}},
      census_all_match, non_census_domains
    }
  }
}
```

## Regenerate + validate (sandbox-safe, no Unreal required)

```
python Scripts/extract_design_data.py
python Scripts/validate_design_data.py
```

The validator (78 checks) re-opens every traced file at its traced line and
re-reads the value from source (round-trip proof), resolves every
inter-domain reference (recipe→item, loot→item, weapon→ammo, poi→loot,
event→loot, hunt→species, quest→* , tech→tech/recipe, npc→quest/dialogue,
dungeon/boss→species, zone bijection), and independently re-counts the
headline numbers with fixed-string greps. `ALL CHECKS PASSED` == the JSON
never drifted from the code that actually compiles.

## Machine-run relationship

Nothing here needs the engine: extraction and validation are pure text
round-trips over Source/. They stay green on any machine (Windows or
otherwise) and are safe to run inside CI or the Unreal Editor Python
environment alike.
