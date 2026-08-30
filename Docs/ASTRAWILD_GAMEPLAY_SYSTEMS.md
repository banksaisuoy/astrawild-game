# ASTRAWILD — Gameplay Systems Overview

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-30** (wave 4 sync — hostile respawn subsystem, building dismantle, power-state persistence)

This is the system map of the V2 foundation round: every gameplay system, where it lives, and how it feeds
the core loop. Deep dives live in the per-system documents referenced in each row.

---

## 1. The Core Loop

```
        ┌──────────────────────────────────────────────────────────────────┐
        │                                                                  │
        ▼                                                                  │
   PREPARE ────► EXPLORE ───► DISCOVER ───► INTERACT ───► FIGHT / CAPTURE / OBSERVE
   (craft gear,    (Dawn Fields  (Echoes, nodes,   (harvest,        (combat, capture
    feed Echoes,    arena, day/   POIs, weather      feed, talk,      pipeline, journal
    check vitals)   night cycle)  changes)           place buildings) observation)
        ▲                                                                  │
        │                                                                  ▼
   UNLOCK ◄──── UPGRADE ◄──── CRAFT / RESEARCH / BUILD ◄──── COLLECT ◄─────┘
   (tech gates    (Echo growth,   (stations, tech tree,     (inventory,
    new recipes/  base power,     placement, power grid)     weight limits,
    buildings,    better gear)                              loot)
    new biomes)
        │
        └──────────────► EXPLORE DEEPER (future biomes, dungeons — PLANNED)
```

### How each stage works today (vertical slice)

| Stage | Player action | Systems involved | Loop payoff |
|---|---|---|---|
| **Prepare** | Craft a Resonator/bandage at the campfire/workbench, eat/drink, feed party Echoes (R) | Crafting, Survival, Echo needs | Consumables + capture devices in inventory |
| **Explore** | WASD + sprint (stamina), day/night lighting, weather states | Time, Weather, Survival, HUD | Time pressure (nocturnal Gloomfangs at night), weather capture bonuses |
| **Discover** | Approach creatures / resource nodes; journal fills by looking at Echoes | Journal (observation cone), Ecosystem (population), HUD prompt | Knowledge milestones → research points |
| **Interact** | E on nodes/stations/rest points/NPCs; B/N/LMB building placement | Interaction interface, ResourceNode, CraftingStation, RestPoint, Building | Materials, crafting, rest, base growth |
| **Fight** | LMB light / F heavy / Q dodge / RMB block (weapon adds flat ATK, shield replaces unarmed block) | Combat, Survival (stamina), Echo AI (flee/aggro), Equipment (wave 3) | Weakened Echo = higher capture chance; loot on defeat |
| **Capture** | E on a wild Echo with a Resonator; feed first for trust | Capture pipeline, Journal bonus, Echo trust/bond, Roster | New companion joins party (max 3) |
| **Observe** | Keep an Echo in view | Journal subsystem | +2 RP per milestone (4 per species), +15 % capture bonus at 100 % |
| **Collect** | Harvest nodes, defeat loot, work-site output, dungeon clear rewards | Inventory (weight 120 kg gate), EventBus `Event.ItemCollected`, Loot tables (wave 3) | Quest objectives tick; craft ingredients; boss-room loot |
| **Return** | Walk back to camp; rest point heals | RestPoint `FullRestore`, autosave every 300 s | Safe prep point |
| **Craft / Research / Build** | Station interact (E); tech unlock via points; B placement | Crafting, Research, Building, Power grid | Better gear (armory equipment — wave 3), electrical buildings, powered work sites |
| **Upgrade** | Echo XP/levels (+10 % HP, +8 % ATK per level), bond growth; X to equip best gear | Echo growth, Work sites, Equipment (wave 3) | Stronger party, faster production, sharper combat numbers |
| **Unlock** | Spend research points | Research (6 techs), ContentLibrary gates | New recipes (Cooking, Armory equipment) and buildings (Generator/Battery/Lamp/FeedTrough) |
| **Explore deeper** | Enter Hollow Underlight east of the arena (procedural dungeon + 3-phase boss) | Dungeon generator + rooms + Echo boss | Boss-room loot table (Ancient Core, shards, ash — wave 3); new biomes still PLANNED |

---

## 2. System Inventory

| # | System | Core class(es) | Doc | One-line responsibility |
|---|---|---|---|---|
| 1 | Player controller & camera | `AAstrawildPlayerCharacter`, `AAstrawildPlayerController` | UI/Input docs | Third-person movement, sprint/jump, interaction trace, component host |
| 2 | Survival vitals | `UAstrawildSurvivalComponent` | `ASTRAWILD_SURVIVAL_SYSTEM.md` | HP/stamina/hunger/thirst/temperature + status effects + death |
| 3 | Combat | `UAstrawildCombatComponent` | `ASTRAWILD_COMBAT_SYSTEM.md` | Light/heavy/dodge/block + elemental damage pipeline (weapon ATK + shield mitigation feed in) |
| 4 | Echo creatures | `AAstrawildEchoCharacter` | `ASTRAWILD_CREATURE_SYSTEM.md` | Definition-driven instances: personality, needs, trust, bond, growth, commands |
| 5 | Echo AI | `AAstrawildEchoAIController` | `ASTRAWILD_AI_ARCHITECTURE.md` | Sight perception + 16-state machine, LOD think rates |
| 6 | Ecosystem & population | `UAstrawildEcosystemSubsystem` | World doc | LOD tiers, wild/captured/defeated counts per species |
| 7 | Capture pipeline | `UAstrawildCaptureComponent` | Creature doc §5 | Resonator + weaken/trust/observe/track/weather → chance roll |
| 8 | Field journal | `UAstrawildJournalSubsystem` | Creature doc §6 | Observation progress, knowledge milestones, research points |
| 9 | Echo roster & party | `UAstrawildEchoRosterSubsystem` | Creature doc §7 | Captured roster (party cap 3), save round-trip |
| 10 | Inventory | `UAstrawildInventoryComponent` | — | Stacks + weight gate (120 kg) + equipment slots (weapon + shield, wave 3) + **`AddItemSilent` for dismantle refunds (wave 4)** |
| 11 | Item registry & content library | `UAstrawildItemRegistrySubsystem`, `UAstrawildContentLibrary` | `ASTRAWILD_ASSET_PIPELINE.md` | Id→definition resolution; CODE_DEFAULT content (items/recipes/species/buildings/techs/quests/loot tables/NPCs) |
| 12 | Crafting | `UAstrawildCraftingComponent`, `AAstrawildCraftingStationActor` | `ASTRAWILD_CRAFTING_SYSTEM.md` | Timed queue, station/tech/ingredient gates |
| 13 | Building placement | `UAstrawildBuildingComponent`, `AAstrawildBuildingActor` | `ASTRAWILD_BUILDING_SYSTEM.md` | Preview/snap/rotate/validate + server-authoritative placement + **dismantle/refund via `AddItemSilent` (wave 4 — Z)** |
| 14 | Power grid | `UAstrawildPowerSubsystem` | Building doc §7 | Generation/draw/battery, brownout shedding, 2 s re-solve, **per-actor `bIsPowered` replication + `ResolveGridNow()` called from `LoadWorld` (wave 4)** |
| 15 | Echo work | `AAstrawildWorkSiteActor` | Creature doc §8 | Affinity/personality/mood/energy-scaled production |
| 16 | Research / tech tree | `UAstrawildResearchSubsystem` | `ASTRAWILD_RESEARCH_SYSTEM.md` | Points, prerequisites, unlock gates (6 techs incl. Armory) |
| 17 | Quests | `UAstrawildQuestComponent` | `ASTRAWILD_QUEST_SYSTEM.md` | Event-driven objectives, 6-quest First Dawn chain (Quest 5 chain-completes via hostile respawn — wave 4) |
| 18 | World state (time/weather) | `AAstrawildGameState`, `UAstrawildTimeSubsystem`, `UAstrawildWeatherSubsystem` | `ASTRAWILD_WORLD_SYSTEM.md` | 24-min days, 8 weather states, replicated |
| 19 | Event bus | `UAstrawildEventBusSubsystem` | Architecture V2 §6 | Decoupled gameplay event pub/sub |
| 20 | Procedural world | `AAstrawildWorldBootstrapper` | World doc §5 | Zero-asset Dawn Fields arena + camp NPCs |
| 21 | Save/load | `UAstrawildSaveSubsystem` | `ASTRAWILD_SAVE_SYSTEM.md` | Schema v2, checksum, migration, autosave, equipment persistence, **per-building power-state persistence (wave 4)** |
| 22 | HUD | `UAstrawildHudWidget` | `ASTRAWILD_UI_ARCHITECTURE.md` | Pure-C++ HUD (12 widgets incl. equipment readout) |
| 23 | Input | runtime Enhanced Input (in PlayerCharacter) | `ASTRAWILD_INPUT_REFERENCE.md` | **19 runtime actions** (wave 4: +Z = Delete Building), full default keymap |
| 24 | Gameplay tags | `AstrawildGameplayTags.h/.cpp` | `ASTRAWILD_GAMEPLAY_TAGS.md` | 77 native tags |
| 25 | Debug/cheats | `UAstrawildCheatManager` | Input Reference §3 | 13 console commands |
| 26 | NPCs | `AAstrawildNPCCharacter` | Quest doc §6 | Camp NPCs (Warden Maren, Trader Tam — wave 3); schedule/dialogue screens PLANNED |
| 27 | Game mode / session | `AAstrawildGameMode` | — | Bootstrapper spawn, respawn (5 s), autosave (300 s) |
| 28 | Tests | `AstrawildAutomationTests.cpp` | `ASTRAWILD_TEST_PLAN.md` | 9 automation tests |
| 29 | Equipment progression | `UAstrawildInventoryComponent` (slots) + `UAstrawildCombatComponent` (math) | Combat doc §2.3/§4 | Weapon ATK + shield mitigation routing, equip-best (X), save persistence |
| 30 | Loot tables | `UAstrawildLootTableDefinition` + registry | Asset Manifest §7 | Guaranteed drops + bonus roll; dungeon clear rewards, vendor stock |
| 31 | Hostile respawn (wave 4) | `UAstrawildHostileSpawnerSubsystem` | World doc / Building doc / Quest doc §5 | `UTickableWorldSubsystem` (server-only, 25 s sweep) that refills `Echo_Gloomfang` (target 4) + `Echo_Emberfang` (target 2) around the player pawn — closes the Quest 5 chain |

---

## 3. System Interaction Map

```
                        ┌───────────── WORLD ─────────────┐
                        │ GameState (time/day/weather/seed)│
                        │ TimeSub      WeatherSub          │
                        └───┬───────────▲──────────┬──────┘
                            │           │          │ (temp offset,
              activity gate │           │          │ visibility)
                            ▼           │          ▼
   ┌──────── ECHO ────────┐          ┌─ SURVIVAL ─┐
   │ EchoCharacter ◄──────┼──────────┤ SurvivalComp│
   │  needs/personality/  │ damage   │ (hunger/    │
   │  trust/bond/growth   │ └────────┤  thirst/temp)│
   │ EchoAIController ────┤          └────────────┬─┘
   │  (16 states, sight)  │                       │ stamina
   └──┬─────────┬─────────┘                       ▼
      │ capture │ work                ┌──────── COMBAT ────────┐
      ▼         ▼                     │ CombatComponent        │
 ┌─ CAPTURE ─┐ ┌─ WORKSITE ──┐        │  light/heavy/dodge/    │
 │CaptureComp│ │WorkSiteActor│        │  block + sweep +       │
 │ + Journal │ │ (affinity × │        │  elemental pipeline    │
 │  observe  │ │  personality)│       └───────┬────────────────┘
 └─────┬─────┘ └──────┬──────┘                │ events
       │ roster        │ items                 ▼
       ▼               ▼                ┌── EVENT BUS ──┐
 ┌─ ROSTER ──┐   ┌─ INVENTORY ─┐        │ Event.* tags  │
 │ (party 3) │◄──┤  weight 120 │◄───────┤ (quests,      │
 └───────────┘   └──────┬──────┘  add   │  journal)     │
                        │ materials     └───┬───────┬───┘
        ┌───────────────┼───────────────┐   │       │
        ▼               ▼               ▼   ▼       ▼
  ┌─ CRAFTING ─┐ ┌─ BUILDING ─┐ ┌─ QUESTS ─┐ ┌─ RESEARCH ─┐
  │ stations + │ │ placement  │ │ event-   │ │ points →   │
  │ tech gates │ │ + actors   │ │ driven   │ │ tech gates │
  └─────┬──────┘ └─────┬─────┘ └──────────┘ └─────┬──────┘
        │ items        │ power                     │ unlocks
        ▼              ▼                           ▼
   (inventory)   ┌─ POWER GRID ─┐          recipes/buildings
                  │ 2s resolve  │
                  └─────────────┘
```

---

## 4. Player-Facing Progression Chain (First Dawn)

The current content deliberately walks a new player through one complete loop:

1. **Quest: First Light** — collect 10 Dawnwood + 5 Fieldstone → +2 Resonators, +5 RP.
2. **Quest: A Friend in the Fields** — observe a Lumewisp (journal fills automatically) then capture one
   (weaken it or feed Glimmer Berries first) → +10 Berries, +10 RP.
3. **Quest: Homeground** — place a Foundation and a Workbench → +10 RP.
4. **Quest: The Spark** — unlock Electrical Foundations (15 RP) and build an Echo Dynamo → +15 RP.
5. **Quest: Dawn Guard** — defeat 3 Gloomfangs (night hostiles) → Ancient Core + 20 RP. **Wave 4: hostile respawn (`UAstrawildHostileSpawnerSubsystem`) keeps `Echo_Gloomfang` population at 4 around the player every 25 s — Quest 5 chain-completes organically via the existing death pipeline (`EchoCharacter::OnDefeated → EventBus TAG_Astrawild_Event_HostileDefeated → QuestComponent::ApplyEventToQuest`). No new quest wiring required.**
6. **Quest: Shepherd's Dawn** (wave 2) — unlock Echo Husbandry, capture a Sprigling, place a Feed Trough,
   craft 3 Feed Mix → +5 Feed Mix, +2 Salves, +20 RP. Chain ends here.

Each quest auto-activates the next on completion (`NextQuestId` chaining).

**Equipment progression (wave 3)** runs alongside the chain: craft the Dawnwood Club (3 Wood + 1 Sunfiber,
no tech) early, then unlock the Armory (8 RP) for the Stonehide Shield and Dawn Crystal Blade; the boss
room of Hollow Underlight drops the blade ingredients (Dawn Crystal Shards, Ember Ash) — press **X** to
auto-equip the best owned gear at any time.

---

## 5. Feedback Surfaces

| Channel | Implementation |
|---|---|
| HUD | Bars (HP/stamina/hunger/thirst), time/weather, quest tracker, interaction prompt, live capture chance %, party command, equipment readout (wave 3), notification line |
| Delegates | Every system broadcasts `On*` dynamic multicast delegates (`OnStatsChanged`, `OnDamaged`, `OnQuestStateChanged`, `OnTechUnlocked`, `OnRosterChanged`, …) for future UI/audio |
| Event bus | Cross-cutting gameplay events for quests/journal/analytics |
| Logging | 8 categories (`LogAstrawild` + 7 subsystem categories) |

---

## 6. Honest Status Notes

- All systems above are **implemented in C++ but not yet compiled** (sandbox has no UE5 toolchain). Compile
  validation happens on the target Windows machine — see `ASTRAWILD_TEST_PLAN.md` §4.
- Quest objective types `ReachLocation`, `SurviveTime` exist in the enum and quest data but **have no event
  wiring yet** (see `ASTRAWILD_QUEST_SYSTEM.md` §5). `ObserveEcho` is wired via journal milestones.
- Work-site output accumulates on the site (`StoredOutput`) — `CollectOutput()` is wired to the **E** interact
  (Batch 1 C-7). Hostile respawn (`UAstrawildHostileSpawnerSubsystem`) refills the wild population around
  the player every 25 s (wave 4).
