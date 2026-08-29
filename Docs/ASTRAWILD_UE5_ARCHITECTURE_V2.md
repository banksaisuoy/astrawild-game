# ASTRAWILD — UE5 Architecture V2

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Scope:** `Source/AstrawildCore/` — 40 headers + 38 cpp files + `AstrawildCore.Build.cs` (≈11,363 LOC), branch `main`

This document supersedes the module-split sketch in `ASTRAWILD_PROJECT_MASTER_PLAN_v1.md` §8. It describes the architecture
that is actually implemented in the repository, as planned by `ASTRAWILD_UE5_ARCHITECTURE_AUDIT.md` §F.

---

## 1. Architectural Principles

| # | Principle | Implementation |
|---|---|---|
| 1 | **Single runtime module** | One module `AstrawildCore` (UE 5.8). Module split is deliberately deferred until the first successful compile on the target Windows machine — fewer cross-module UBT failure surfaces in a sandbox that cannot compile (audit §F decision). Internal organization is folder-based (see §3). |
| 2 | **Server-authoritative** | Every gameplay mutation (damage, inventory, capture, crafting, building, quests, research, needs decay, weather, time) runs behind `ROLE_Authority` / `HasAuthority()` guards. Clients send intent through `Server_` RPCs; state flows back via replication. See `ASTRAWILD_MULTIPLAYER.md`. |
| 3 | **Event-driven** | `UAstrawildEventBusSubsystem` publishes `Event.*` gameplay tags (`FAstrawildGameplayEvent`: tag + instigator + target id + amount + location). Quests, journal milestones, ecosystem bookkeeping and future audio/UI subscribe without knowing publishers. |
| 4 | **Data-driven registry** | All content (items, recipes, Echo species, buildings, technologies, quests) resolves through `UAstrawildItemRegistrySubsystem` by stable `FName` id. Gameplay code never hard-codes object references. |
| 5 | **Code-defined content library** | `UAstrawildContentLibrary::BuildDefaults()` registers all default definitions in memory at world start (10 items, 5 recipes, 5 Echo species, 9 buildings, 4 technologies, 5 quests). Every entry is tagged `CODE_DEFAULT` and replaceable by `.uasset` data assets through the same `Register*` API (see `ASTRAWILD_ASSET_PIPELINE.md`). |
| 6 | **Zero-asset playability** | `AAstrawildWorldBootstrapper` builds the Dawn Fields arena — lighting rig, ground, resource nodes, wild Echoes, hostiles, rest point, crafting stations, work sites — entirely from engine basic shapes at BeginPlay. The project is PIE-playable immediately after compile with an empty `Content/` folder. |
| 7 | **Simulation LOD** | Echoes register with `UAstrawildEcosystemSubsystem`; tier assignment by distance to nearest player (Tier 0–3) throttles needs decay and AI think rates (see `ASTRAWILD_PERFORMANCE.md`). |
| 8 | **Save schema v2 + migration + checksum** | `UAstrawildSaveSubsystem` writes schema v2 with FNV-1a header checksum, migrates v1 files, orchestrates full world state (see `ASTRAWILD_SAVE_SYSTEM.md`). |

---

## 2. Module & Dependency Diagram

```
                        ┌──────────────────────────────────────────────┐
                        │  Unreal Engine 5.8                           │
                        │  Core · CoreUObject · Engine · InputCore      │
                        │  EnhancedInput · GameplayTags · GameplayTasks │
                        │  GameplayAbilities · AIModule · NavSystem     │
                        │  UMG (+ Slate/SlateCore private)              │
                        └───────────────────────┬──────────────────────┘
                                                │
┌───────────────────────────────────────────────▼────────────────────────────────────────────────┐
│  AstrawildCore  (single runtime module — Source/AstrawildCore)                                  │
│                                                                                                  │
│  FOUNDATION                             WORLD (subsystems + authority)                          │
│  ├─ AstrawildCore (module, LogAstrawild)├─ AstrawildGameState      (replicated world state)     │
│  ├─ AstrawildLog      (7 log categories)├─ AstrawildTimeSubsystem (day/night, 1s=1world min)   │
│  ├─ AstrawildGameplayTags (77 native)   ├─ AstrawildWeatherSubsystem (8 states, weighted roll) │
│  ├─ AstrawildTypes    (enums/structs v2)├─ AstrawildEcosystemSubsystem (LOD tiers + population)│
│  └─ AstrawildDataAssets (8 def. types)  ├─ AstrawildEventBusSubsystem (Event.* pub/sub)         │
│                                          └─ AstrawildWorldBootstrapper (procedural Dawn Fields) │
│                                                                                                  │
│  PLAYER                                  ECHO                                                    │
│  ├─ AstrawildPlayerCharacter (EI runtime)├─ AstrawildEchoCharacter   (def-driven creature)     │
│  ├─ AstrawildSurvivalComponent           ├─ AstrawildEchoAIController (perception + 16 states)│
│  ├─ AstrawildCombatComponent             ├─ AstrawildEchoRosterSubsystem (captured roster)    │
│  ├─ AstrawildQuestComponent (on PC)      └─ AstrawildWorkSiteActor   (Echo base jobs)          │
│  ├─ AstrawildBuildingComponent                                                                    │
│  ├─ AstrawildHudWidget (pure C++)        ITEMS / ECONOMY                                         │
│  └─ AstrawildPlayerController            ├─ AstrawildInventoryComponent (weight + equip)        │
│                                          ├─ AstrawildItemRegistrySubsystem (id→definition)     │
│  CAPTURE                                 ├─ AstrawildContentLibrary   (CODE_DEFAULT content)   │
│  ├─ AstrawildCaptureComponent            ├─ AstrawildCraftingComponent (timed, gated)          │
│  └─ AstrawildJournalSubsystem            └─ AstrawildCraftingStationActor (interactable)       │
│                                                                                                  │
│  BASE BUILDING                           META / PRODUCTION                                       │
│  ├─ AstrawildBuildingActor               ├─ AstrawildResearchSubsystem (tech tree, GI-scoped)  │
│  └─ AstrawildPowerSubsystem              ├─ AstrawildSaveSubsystem v2 (+ UAstrawildSaveGame)    │
│                                          ├─ AstrawildCheatManager   (12 exec functions)         │
│  WORLD OBJECTS / LEGACY v1 KEEPERS       ├─ AstrawildGameMode       (bootstrapper+respawn+auto) │
│  ├─ AstrawildResourceNode                ├─ AstrawildNPCCharacter   (architecture-ready)       │
│  ├─ AstrawildRestPoint                   └─ AstrawildAutomationTests (8 tests)                  │
│  ├─ AstrawildDamageTarget                                                                       │
│  └─ AstrawildInteractable (UInterface)                                                           │
└──────────────────────────────────────────────────────────────────────────────────────────────────┘
         │                                    │                              │
   PlayerCharacter ◄── (possess)      EchoAIController ──► EchoCharacter   GameMode spawns
   PlayerController creates HUD;      (sight perception)  (auto-possessed) WorldBootstrapper
   QuestComponent rides on PC
```

Dependency direction rules (enforced by include discipline, not by module boundaries):

- **Foundation** is included by everything; includes nobody except engine.
- **World subsystems** depend on Foundation + GameState; never on Player/Echo actors' headers at the *subsystem* level (the ecosystem and journal do iterate actors, but actors call into subsystems, not the reverse at construction time).
- **Components/Actors** may call subsystems (registry, event bus, research, power, ecosystem) and each other's public API.
- **Meta systems** (save, cheats, game mode) orchestrate everything but nothing depends on them except entry points.

---

## 3. Folder / Subsystem Architecture

```
Source/AstrawildCore/
├── AstrawildCore.Build.cs          (+ AIModule, NavigationSystem, UMG, GameplayTags, EnhancedInput)
├── Public/                         (40 headers)
│   # Foundation:  AstrawildCore.h, AstrawildLog.h, AstrawildGameplayTags.h,
│   #              AstrawildTypes.h, AstrawildDataAssets.h
│   # World:       AstrawildGameState.h, AstrawildTimeSubsystem.h, AstrawildWeatherSubsystem.h,
│   #              AstrawildEcosystemSubsystem.h, AstrawildEventBusSubsystem.h, AstrawildWorldBootstrapper.h
│   # Player:      AstrawildPlayerCharacter.h, AstrawildSurvivalComponent.h, AstrawildCombatComponent.h,
│   #              AstrawildQuestComponent.h, AstrawildHudWidget.h, AstrawildPlayerController.h
│   # Echo:        AstrawildEchoCharacter.h, AstrawildEchoAIController.h, AstrawildEchoRosterSubsystem.h,
│   #              AstrawildWorkSiteActor.h
│   # Items:       AstrawildInventoryComponent.h, AstrawildItemRegistrySubsystem.h,
│   #              AstrawildContentLibrary.h, AstrawildCraftingComponent.h, AstrawildCraftingStationActor.h
│   # Capture:     AstrawildCaptureComponent.h, AstrawildJournalSubsystem.h
│   # Base:        AstrawildBuildingActor.h, AstrawildBuildingComponent.h, AstrawildPowerSubsystem.h
│   # Meta:        AstrawildResearchSubsystem.h, AstrawildSaveSubsystem.h, AstrawildCheatManager.h,
│   #              AstrawildGameMode.h, AstrawildNPCCharacter.h
│   # Objects:     AstrawildResourceNode.h, AstrawildRestPoint.h, AstrawildDamageTarget.h,
│   #              AstrawildInteractable.h
└── Private/                        (38 cpp; mirrors Public + AstrawildAutomationTests.cpp)
```

Note: the audit (§F) sketched a separate `AstrawildEchoWorkComponent`. During implementation the Echo work
behavior was **consolidated** into `AAstrawildEchoCharacter` (needs/personality/`AssignedWorkSite`) plus
`AAstrawildWorkSiteActor` (production math). No separate work component file exists — by design, not omission.

---

## 4. Class Inventory

### 4.1 Foundation

| Class / Asset | File | Responsibility |
|---|---|---|
| `FAstrawildModule` | `AstrawildCore.h/.cpp` | Module implementation; declares the general `LogAstrawild` category |
| Log categories | `AstrawildLog.h/.cpp` | 7 subsystem categories: `LogAstrawildAI`, `LogAstrawildCombat`, `LogAstrawildSave`, `LogAstrawildNetwork`, `LogAstrawildBuilding`, `LogAstrawildWorld`, `LogAstrawildEconomy` |
| Native gameplay tags | `AstrawildGameplayTags.h/.cpp` | 77 tags across 12 groups (`State.Creature/Player`, `Status`, `Element`, `Damage`, `Item`, `Interaction`, `Biome`, `Weather`, `Event`, `Faction`, `Gameplay`) — see `ASTRAWILD_GAMEPLAY_TAGS.md` |
| Types & enums | `AstrawildTypes.h` | `EAstrawildElementType` (5), `EAstrawildEchoRole` (4), `EAstrawildPersonality` (10), `EAstrawildActivityPattern` (3), `EAstrawildWeatherState` (8), `EAstrawildEchoCommand` (7), `EAstrawildEchoAIState` (16), `EAstrawildWorkType` (11), `EAstrawildQuestObjectiveType` (9), `EAstrawildTechEra` (6), `EAstrawildBuildingCategory` (13), `EAstrawildPowerRole` (3); structs: `FAstrawildStableId`, `FAstrawildItemStack`, `FAstrawildEchoStats`, `FAstrawildEchoNeeds`, `FAstrawildWorkAffinity`, `FAstrawildSurvivalStats`, `FAstrawildStatusEffect`, `FAstrawildEchoInstanceV2`, `FAstrawildQuestObjective`, `FAstrawildBuildingSaveData`, `FAstrawildWorldSaveData`, `FAstrawildResearchSaveData`, `FAstrawildQuestSaveData`, `FAstrawildJournalEntry`, v1 save structs |
| `UAstrawildItemDefinition` | `AstrawildDataAssets.h` | Item template: category, weight, stack size, food/water/heal values, attack power, block mitigation, Echo feed value |
| `UAstrawildRecipeDefinition` | `AstrawildDataAssets.h` | Recipe: ingredients, outputs, craft duration, required tech + station |
| `UAstrawildEchoDefinition` | `AstrawildDataAssets.h` | Species template: stats, element, role, personality, activity pattern, food, habitat, weather preference, capture difficulty, weakness, work affinities, needs rates, growth, loot, hostility, sight radii |
| `UAstrawildBuildingDefinition` | `AstrawildDataAssets.h` | Building piece: category, required item/tech, grid cell, health, power role/gen/draw/battery, enabled work type |
| `UAstrawildTechnologyDefinition` | `AstrawildDataAssets.h` | Tech node: era, research cost, prerequisites, unlocked recipes/buildings |
| `UAstrawildQuestDefinition` | `AstrawildDataAssets.h` | Quest: objectives, item/point/tech rewards, chain link |
| `UAstrawildLootTableDefinition` | `AstrawildDataAssets.h` | Weighted loot (data contract ready; not yet consumed by runtime content) |
| `UAstrawildNPCDefinition` | `AstrawildDataAssets.h` | NPC: display name, offered quest, shop table id |

### 4.2 World

| Class | File | Responsibility |
|---|---|---|
| `AAstrawildGameState` | `AstrawildGameState.h/.cpp` | Replicated world state: `TimeOfDayMinutes` (default 480 = 08:00), `DayNumber`, `WeatherState`, `WorldSeed` (default 1337); server-only setters; sun-cycle helpers |
| `UAstrawildTimeSubsystem` | `AstrawildTimeSubsystem.h/.cpp` | Server day/night clock: `MinutesPerRealSecond = 1.0` → 24-minute full day; hour/day broadcasts; debug `SetTimeOfDay`/`AdvanceDays` |
| `UAstrawildWeatherSubsystem` | `AstrawildWeatherSubsystem.h/.cpp` | 8 weather states; weighted transition every **90 in-world minutes**; per-state temperature offset / visibility multiplier profiles |
| `UAstrawildEcosystemSubsystem` | `AstrawildEcosystemSubsystem.h/.cpp` | Echo registry + simulation LOD tiers (Tier0 ≤ 3000 cm, Tier1 ≤ 8000, Tier2 ≤ 20000, Tier3 beyond; sweep every 1.0 s); species population bookkeeping |
| `UAstrawildEventBusSubsystem` | `AstrawildEventBusSubsystem.h/.cpp` | Decoupled event bus: `Publish(FAstrawildGameplayEvent)` → `OnGameplayEvent` broadcast |
| `AAstrawildWorldBootstrapper` | `AstrawildWorldBootstrapper.h/.cpp` | Deterministic (seeded) procedural Dawn Fields: lighting rig, ground plane, 26 resource nodes, 9 wild Echoes, 2 Gloomfangs, camp (rest point, workbench, campfire, 2 work sites); sun tracking at 0.25 s tick |

### 4.3 Player

| Class | File | Responsibility |
|---|---|---|
| `AAstrawildPlayerCharacter` | `AstrawildPlayerCharacter.h/.cpp` | Third-person character; owns Inventory/Crafting/Capture/Survival/Combat/Building components; builds a complete runtime Enhanced Input mapping when no editor IMC asset is assigned; party command cycling; quick save/load |
| `UAstrawildSurvivalComponent` | `AstrawildSurvivalComponent.h/.cpp` | Server vitals: hunger/thirst decay, stamina regen, starvation damage, temperature, status effects, death (see Survival doc) |
| `UAstrawildCombatComponent` | `AstrawildCombatComponent.h/.cpp` | Light/heavy attacks, dodge i-frames, block mitigation, elemental pipeline, sweep hit resolution (see Combat doc) |
| `UAstrawildQuestComponent` | `AstrawildQuestComponent.h/.cpp` | Event-driven quest progression on the PlayerController; chain following; rewards; save export/import |
| `UAstrawildHudWidget` | `AstrawildHudWidget.h/.cpp` | Pure-C++ HUD: widget tree built in `NativeConstruct`, 0.15 s refresh |
| `AAstrawildPlayerController` | `AstrawildPlayerController.h/.cpp` | Creates the HUD for local players; owns the QuestComponent |

### 4.4 Echo

| Class | File | Responsibility |
|---|---|---|
| `AAstrawildEchoCharacter` | `AstrawildEchoCharacter.h/.cpp` | Creature instance: definition-driven stats, personality roll, needs decay, trust/bond/growth, commands, capture, elemental damage, save v2 |
| `AAstrawildEchoAIController` | `AstrawildEchoAIController.h/.cpp` | Sight perception + 16-state C++ state machine (no BT asset required); personality-modulated decisions; blackboard key contract for future BT |
| `UAstrawildEchoRosterSubsystem` | `AstrawildEchoRosterSubsystem.h/.cpp` | GameInstance-scoped captured-Echo roster (party cap 3); save export/import |
| `AAstrawildWorkSiteActor` | `AstrawildWorkSiteActor.h/.cpp` | Base work site: Echoes produce items over time scaled by affinity/personality/mood/energy/power |

### 4.5 Items / Economy

| Class | File | Responsibility |
|---|---|---|
| `UAstrawildInventoryComponent` | `AstrawildInventoryComponent.h/.cpp` | TMap stacks, weight gate (`MaxWeight = 120`), one equipment slot, event publishing on collection |
| `UAstrawildItemRegistrySubsystem` | `AstrawildItemRegistrySubsystem.h/.cpp` | Central id→definition registry for all six content kinds; loads code defaults at world begin |
| `UAstrawildContentLibrary` | `AstrawildContentLibrary.h/.cpp` | Static builder of all CODE_DEFAULT content (10 items, 5 recipes, 5 Echoes, 9 buildings, 4 techs, 5 quests) |
| `UAstrawildCraftingComponent` | `AstrawildCraftingComponent.h/.cpp` | Timed one-at-a-time craft queue; tech + station + ingredient gating |
| `AAstrawildCraftingStationActor` | `AstrawildCraftingStationActor.h/.cpp` | Interactable station (`UseRadius = 500`); interact crafts the first craftable station recipe |

### 4.6 Capture / Journal

| Class | File | Responsibility |
|---|---|---|
| `UAstrawildCaptureComponent` | `AstrawildCaptureComponent.h/.cpp` | Capture pipeline: Resonator consumption, situation chance (weaken/trust/observe/track/weather/time), roll, cooldown 1.0 s |
| `UAstrawildJournalSubsystem` | `AstrawildJournalSubsystem.h/.cpp` | Automatic field-journal observation (1400 cm, ~41° cone, 5%/s); knowledge milestones at 25/50/75/100 % each award +2 research |

### 4.7 Base Building

| Class | File | Responsibility |
|---|---|---|
| `AAstrawildBuildingActor` | `AstrawildBuildingActor.h/.cpp` | Placed piece: health, power registration, save data, category-scaled placeholder silhouette |
| `UAstrawildBuildingComponent` | `AstrawildBuildingComponent.h/.cpp` | Client placement UX: B toggle, N rotate, preview ghost, grid snap, validation; server RPC placement with re-validation + refund |
| `UAstrawildPowerSubsystem` | `AstrawildPowerSubsystem.h/.cpp` | Proximity power grid (1200 cm connectivity); re-solves every 2.0 s; brownout sheds lowest-priority consumers first |

### 4.8 Meta

| Class | File | Responsibility |
|---|---|---|
| `UAstrawildResearchSubsystem` | `AstrawildResearchSubsystem.h/.cpp` | GameInstance-scoped tech tree; shared co-op research pool (documented decision); prereq + cost validation |
| `UAstrawildSaveGame` / `UAstrawildSaveSubsystem` | `AstrawildSaveSubsystem.h/.cpp` | Schema v2 payload; FNV-1a header checksum; v1→v2 migration; `SaveWorld`/`LoadWorld` orchestration; legacy snapshot API |
| `UAstrawildCheatManager` | `AstrawildCheatManager.h/.cpp` | 12 console cheats (`AW.*`) — see `ASTRAWILD_INPUT_REFERENCE.md` |
| `AAstrawildGameMode` | `AstrawildGameMode.h/.cpp` | Session owner: default classes, bootstrapper spawn, respawn (5 s), autosave (300 s) |
| `AAstrawildNPCCharacter` | `AstrawildNPCCharacter.h/.cpp` | NPC base: interact offers quest from definition; schedule/dialogue PLANNED |
| Automation tests | `AstrawildAutomationTests.cpp` | 8 logic tests — see `ASTRAWILD_TEST_PLAN.md` |

### 4.9 World Objects (v1 keepers)

| Class | File | Responsibility |
|---|---|---|
| `AAstrawildResourceNode` | `AstrawildResourceNode.h/.cpp` | Harvestable node: quantity per harvest 2, remaining 3, respawn timer, hide/show |
| `AAstrawildRestPoint` | `AstrawildRestPoint.h/.cpp` | Interact → full vitals restore; activation event; save data |
| `AAstrawildDamageTarget` | `AstrawildDamageTarget.h/.cpp` | Simple HP test dummy for combat verification |
| `UAstrawildInteractable` (interface) | `AstrawildInteractable.h` | `Interact` + `GetInteractionPrompt` Blueprint interface used by all interaction targets |

---

## 5. Server-Authoritative Model (summary)

**The server decides; clients request.**

| Domain | Client sends | Server validates & mutates | State reaches clients via |
|---|---|---|---|
| Combat | `ServerLightAttack` / `ServerHeavyAttack` / `ServerDodge` / `ServerSetBlocking` | cooldown, stamina, death, sweep trace, mitigation | `Survival::Stats` (ReplicatedUsing), `bIsBlocking`, `bReplicatedDodgeTimer` |
| Inventory | — (server-side actions only today) | weight gate, add/remove | `Items`, `EquippedItemId` (Replicated) |
| Capture | — (interact runs on server path in SP/listen) | Resonator cost, chance, roll, roster add | Echo replicated state (`Personality`, `OwnerPlayerId`, …), event bus |
| Building | `ServerPlaceBuilding(DefId, Location, Yaw)` | definition exists, overlap re-validation, spawn | `AAstrawildBuildingActor` replication (`bIsSwitchedOn`, `CurrentHealth`, `StoredCharge`) |
| Quests | — | event bus consumption, rewards, chaining | Not yet replicated (single-player-first; see Multiplayer doc) |
| World time/weather | — | subsystem ticks on server only | `AAstrawildGameState` replicated properties |

Full inventory: `ASTRAWILD_MULTIPLAYER.md`.

---

## 6. Event Bus Pattern

```
Publisher (server)                                Subscriber
────────────────────                               ──────────────────────────────
InventoryComponent.AddItem ──┐
EchoCharacter.Capture ───────┤
EchoCharacter.ApplyElementalDamage (defeat) ──┤──► EventBus.PublishEvent(Event.*, …) ──► QuestComponent (objective progress)
CraftingComponent (craft) ───┤                                                ──► (future) audio / UI / ecosystem
BuildingComponent.ServerPlaceBuilding ─┤
ResearchSubsystem.TryUnlockTech ─┘
```

Event payload (`FAstrawildGameplayEvent`): `EventTag` (Event.* native tag), `Instigator` (weak actor),
`TargetId` (FName domain id), `Amount` (int32), `Location` (FVector). Publishing is a broadcast on the
`OnGameplayEvent` dynamic multicast delegate — subscribers filter by tag.

---

## 7. Data-Driven Content + CODE_DEFAULT Library

All six definition kinds register into `UAstrawildItemRegistrySubsystem` maps keyed by stable `FName`:

```
UAstrawildContentLibrary::BuildDefaults(registry)   [server world begin]
    ├── BuildItems        → 10 items
    ├── BuildRecipes      → 5 recipes
    ├── BuildEchoes       → 5 Echo species
    ├── BuildBuildings    → 9 buildings
    ├── BuildTechnologies → 4 technologies
    └── BuildQuests       → 5 quests (First Dawn chain)
```

Replacement strategy: authoring a `.uasset` data asset with the **same id** and registering it (same `Register*`
API) overrides the code default — see `ASTRAWILD_ASSET_PIPELINE.md` and `ASTRAWILD_ASSET_MANIFEST.md`.

---

## 8. Zero-Asset Playability Strategy

Three pillars make the project playable straight from compile with an empty `Content/` folder:

1. **`AAstrawildWorldBootstrapper`** (spawned by the GameMode): builds the 160 m × 160 m Dawn Fields arena —
   directional sun + sky light + sky atmosphere + height fog, scaled engine plane ground, seeded scatter of
   26 resource nodes, 9 wild Echoes + 2 hostile Gloomfangs, and a starter camp (rest point, workbench,
   campfire, gathering + farm work sites) at a 900 cm radius around origin.
2. **Runtime Enhanced Input**: `AAstrawildPlayerCharacter::BuildRuntimeInputDefaults()` creates 15 input
   actions + a mapping context in code when no editor IMC is assigned (WASD/mouse/LMB/RMB/Q/F/E/C/R/B/N/Space/Shift/F5/F9).
3. **Pure-C++ HUD**: `UAstrawildHudWidget` builds its whole widget tree in `NativeConstruct` — bars, texts,
   prompts — with zero UMG assets.

---

## 9. Simulation LOD Tiers

| Tier | Distance to nearest player | What simulates | Recommended update interval |
|---|---|---|---|
| Tier 0 — Full | ≤ 3000 cm | Full AI, movement, needs, combat | 0.0 s (per-frame budget owned by AI/character) |
| Tier 1 — Reduced | ≤ 8000 cm | Reduced-rate needs decay, AI | 0.25 s (4 Hz) |
| Tier 2 — Statistical | ≤ 20000 cm | Needs decay + population bookkeeping, no movement | 1.0 s |
| Tier 3 — World | > 20000 cm / despawned | World-level bookkeeping only | 5.0 s |

Tier sweep runs every 1.0 s on the server (`TierUpdateIntervalSeconds`). Echo needs decay consumes the
recommended interval through an accumulator. AI think-rate LOD is computed but currently reschedules per
frame — see `ASTRAWILD_PERFORMANCE.md` §Known Issues.

---

## 10. Save Schema v2 (summary)

`UAstrawildSaveGame` (schema version constant `2`) carries: world state (time/day/weather/seed), player
transform + survival stats + inventory, Echo roster v2 (personality/needs/bond), buildings, research, quests,
journal — plus the v1 payload kept alive for migration. FNV-1a checksum over `"<schema>|<timestamp>"`.
Slots: `ASTRAWILD_Main` (F5 manual) and `ASTRAWILD_Auto` (GameMode autosave every 300 s). Details:
`ASTRAWILD_SAVE_SYSTEM.md`.

---

## 11. What Is NOT In This Architecture (honest gaps)

| Item | Status |
|---|---|
| Behavior Tree / StateTree assets | NOT IMPLEMENTED — C++ state machine instead; blackboard key contract documented for future assets |
| PlayerState / per-client quest replication | NOT IMPLEMENTED — quests live on PlayerController, single-player-first |
| World Partition / real landscape | NOT IMPLEMENTED — bootstrapper arena instead |
| GAS (Gameplay Ability System) abilities | NOT IMPLEMENTED — module dependency exists, plain components used |
| UMG crafting/inventory screens | NOT IMPLEMENTED — station interact stopgap + C++ HUD |
| Loot table runtime consumption | DATA CONTRACT ONLY — `UAstrawildLootTableDefinition` exists; species loot uses `DefeatLoot` directly |
| Dungeon / boss | PLANNED (roadmap M8+) |

---

## 12. Related Documents

`ASTRAWILD_GAMEPLAY_SYSTEMS.md` (system map) · `ASTRAWILD_MULTIPLAYER.md` (replication inventory) ·
`ASTRAWILD_PERFORMANCE.md` (tick budgets) · `ASTRAWILD_ASSET_PIPELINE.md` (uasset replacement) ·
`ASTRAWILD_UE5_ARCHITECTURE_AUDIT.md` (the audit that drove this round)
