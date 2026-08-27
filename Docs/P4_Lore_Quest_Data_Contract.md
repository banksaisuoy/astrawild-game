# P4 — Lore and Quest DataTable Contract

## C++ row types

`FAstrawildLoreRow` and `FAstrawildQuestRow` are `FTableRowBase` schemas in `AstrawildTypes.h`. `FAstrawildQuestObjectiveRow` is a separate row type so objectives can be authored in a second DataTable without relying on fragile CSV array syntax.

## Source files prepared

| Source | Intended imported asset | Row struct |
|---|---|---|
| `Content/Astrawild/Data/Source/DT_Lore.csv` | `Content/Astrawild/Data/DT_Lore` | `FAstrawildLoreRow` |
| `Content/Astrawild/Data/Source/DT_Quests.csv` | `Content/Astrawild/Data/DT_Quests` | `FAstrawildQuestRow` |
| `Content/Astrawild/Data/Source/DT_QuestObjectives.csv` | `Content/Astrawild/Data/DT_QuestObjectives` | `FAstrawildQuestObjectiveRow` |

## Import procedure

Import each CSV from the Unreal Content Browser as a Data Table and select the exact row struct shown above. Keep the imported asset paths stable. After import, verify that Gameplay Tags exist in the project tag list; a missing tag should be treated as a content error rather than silently accepted.

## Content rules

Quest IDs and Lore IDs are stable identifiers and must not be renamed after a save file references them. `Quest.DangerPit` requires `Quest.FirstResonator`, and its `DefeatAlpha` objective targets `Echo.SolarixAlpha`. The Alpha Echo defeat event must advance progression on the authoritative server only. Use `RegionTag` for map filtering and codex grouping.

## Localization and writing

Keep `Title`, `Description`, `Body`, and objective text in `FText`. Do not store player-facing text as `FString`. Use concise objective descriptions and avoid hard-coded UI text in Widget Blueprints. When the project adds localization, gather these fields from the DataTables through the normal localization pipeline.

## QA gate

Create the three `.uasset` DataTables, open each row in the DataTable editor, verify the values and Gameplay Tags, and complete one PIE flow from `Quest.Awakening` through `Quest.FirstResonator` to `Quest.DangerPit`. Record row counts and the exact asset paths in `Docs/BUILD_STATUS.md`.
