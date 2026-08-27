# ASTRAWILD Milestones 6–8 — Crafting, Ranged Combat, and Tower Dungeons

## Scope and truth boundary

This package adds source contracts for **64 craft recipes**, technology-gated crafting, eight ranged weapon definitions, a server-authoritative hitscan path, and five tower dungeon rows. It does not contain weapon meshes, projectile VFX, audio, authored tower maps, boss skeletal assets, binary DataTables, or a packaged executable.

## Data import order

| Order | CSV | Row struct | Rows |
|---:|---|---|---:|
| 1 | `DT_TechnologyNodes.csv` | `FAstrawildTechnologyNodeRow` | 20 |
| 2 | `DT_Recipes.csv` | `FAstrawildCraftingRecipeRow` | 64 |
| 3 | `DT_RangedWeapons.csv` | `FAstrawildRangedWeaponRow` | 8 |
| 4 | `DT_Dungeons.csv` | `FAstrawildDungeonRow` | 5 |

Import derived binary DataTables under `Content/Astrawild/Data/Imported/` and assign the recipe table to the player’s native `Crafting` component, the weapon table to `RangedCombat`, and the dungeon table to `UAstrawildDungeonSubsystem` through the authored GameInstance/World setup.

## Crafting gates

`UAstrawildCraftingComponent::CanCraft` checks station, required technology, and all ingredient quantities. `CraftRecipe` removes ingredients only after the same checks and refunds removed ingredients if a race or full inventory prevents output insertion. The DataTable importer uses parallel ingredient-tag/quantity arrays and skips malformed rows rather than creating partial recipes. The source validator requires exactly 64 unique recipe tags and validates array alignment; the Windows import step must still confirm Unreal DataTable parsing and GameplayTag registration.

## Ranged combat gates

`UAstrawildRangedCombatComponent` equips only configured weapons whose technology requirement is unlocked. Fire is cooldown- and magazine-gated, decrements ammo once, performs a view-direction trace, and sends a hit through the existing `UAstrawildCombatComponent::ApplyDamageToTarget` path so mitigation, elemental multiplier, crit, and combat delegates remain centralized. The current source implementation is authoritative-only; multiplayer client RPC/input prediction remains a Phase 10 hardening task. `bUseHitscan` is data-ready for future projectile actors but does not falsely claim that projectile simulation exists.

## Tower dungeon assembly

Create one authored gate/arena per row: Solar, Torrent, Geo, Volt, and Abyssal. Each gate should provide the native dungeon subsystem with the matching `DungeonId` and DataTable. The subsystem validates the required key, consumes it when configured, tracks participants, enforces the time limit, and completes only when the server registers the expected boss encounter as defeated. Blueprint owns boss spawning, room geometry, telegraphs, VFX/audio, reward inventory grants, and quest/lore hookups. Do not grant rewards from client-only callbacks.

## Windows acceptance matrix

| Check | Expected evidence |
|---|---|
| Recipe import | 64 rows, no import warnings, ingredient arrays visible in DataTable |
| Tech gate | Locked recipe fails before inventory mutation; unlocked recipe crafts and refunds on full output inventory |
| Ranged fire | Equip → ammo decrement → cooldown → damage event; no duplicate damage on one fire |
| Elemental matrix | Solar/Torrent/Geo/Volt/Glacial/Abyssal/Astra rows preserve legacy Aether behavior |
| Dungeon key | Missing key blocks entry; configured key is consumed once on server |
| Coop dungeon | Second player joins active supported dungeon; client cannot complete it alone |
| Timeout | Completion after time limit fails and clears active state |
| Package | Development package launches with the authored map/DataTables and no missing hard references |

Static checks in the repository do not replace UE 5.8 C++ compilation, DataTable import, PIE, network PIE, or packaging. Update `Docs/BUILD_STATUS.md` with those results and attach screenshots/logs from the Windows machine.
