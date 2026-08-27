# Code-Complete Pass — Progression, Survival, Quest and Save

## Added runtime systems

`UAstrawildQuestComponent` now reads `FAstrawildQuestRow` and `FAstrawildQuestObjectiveRow` from DataTables, starts quests after prerequisite checks, tracks objective progress, broadcasts state events, and exports/imports through `FAstrawildPlayerProfile`.

`UAstrawildSurvivalComponent` now owns hunger, thirst, temperature stress, carry weight, warning thresholds, food/water restoration, and profile export/import. It is attached to the Player Character and can be driven by UMG or interactable world actors.

`UAstrawildSaveSubsystem` now captures and restores Quest and Survival state in the existing PlayerProfile flow. The schema remains version 1 and new fields have safe defaults for older saves.

## Core-loop integration

The intended code path is: harvest resources → craft food/water or a Resonator → survival values drain over time → interact with resources/Echoes/quests → capture an Echo → advance quest objectives → build a rest point → save and reload the complete state. UI and DataTable assets remain Editor-side responsibilities.

## Future extension contracts

Breeding, evolution, mounts, technology tree and multiplayer remain separate feature slices. Do not fake them by mutating stable species tags in a Widget Blueprint. Add explicit schemas, server authority and save migration before enabling them in a release build.
