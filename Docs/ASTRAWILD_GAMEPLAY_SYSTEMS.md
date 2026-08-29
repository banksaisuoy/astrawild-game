# ASTRAWILD — Gameplay Systems Overview

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**

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
| **Fight** | LMB light / F heavy / Q dodge / RMB block | Combat, Survival (stamina), Echo AI (flee/aggro) | Weakened Echo = higher capture chance; loot on defeat |
| **Capture** | E on a wild Echo with a Resonator; feed first for trust | Capture pipeline, Journal bonus, Echo trust/bond, Roster | New companion joins party (max 3) |
| **Observe** | Keep an Echo in view | Journal subsystem | +2 RP per milestone (4 per species), +15 % capture bonus at 100 % |
| **Collect** | Harvest nodes, defeat loot, work-site output | Inventory (weight 120 kg gate), EventBus `Event.ItemCollected` | Quest objectives tick; craft ingredients |
| **Return** | Walk back to camp; rest point heals | RestPoint `FullRestore`, autosave every 300 s | Safe prep point |
| **Craft / Research / Build** | Station interact (E); tech unlock via points; B placement | Crafting, Research, Building, Power grid | Better gear, electrical buildings, powered work sites |
| **Upgrade** | Echo XP/levels (+10 % HP, +8 % ATK per level), bond growth | Echo growth, Work sites | Stronger party, faster production |
| **Unlock** | Spend research points | Research (4 techs), ContentLibrary gates | New recipes (Cooking) and buildings (Generator/Battery/Lamp) |
| **Explore deeper** | — | PLANNED (dungeons, bosses, new biomes — see Roadmap) | Long-term motivation |

---

## 2. System Inventory

| # | System | Core class(es) | Doc | One-line responsibility |
|---|---|---|---|---|
| 1 | Player controller & camera | `AAstrawildPlayerCharacter`, `AAstrawildPlayerController` | UI/Input docs | Third-person movement, sprint/jump, interaction trace, component host |
| 2 | Survival vitals | `UAstrawildSurvivalComponent` | `ASTRAWILD_SURVIVAL_SYSTEM.md` | HP/stamina/hunger/thirst/temperature + status effects + death |
| 3 | Combat | `UAstrawildCombatComponent` | `ASTRAWILD_COMBAT_SYSTEM.md` | Light/heavy/dodge/block + elemental damage pipeline |
| 4 | Echo creatures | `AAstrawildEchoCharacter` | `ASTRAWILD_CREATURE_SYSTEM.md` | Definition-driven instances: personality, needs, trust, bond, growth, commands |
| 5 | Echo AI | `AAstrawildEchoAIController` | `ASTRAWILD_AI_ARCHITECTURE.md` | Sight perception + 16-state machine, LOD think rates |
| 6 | Ecosystem & population | `UAstrawildEcosystemSubsystem` | World doc | LOD tiers, wild/captured/defeated counts per species |
| 7 | Capture pipeline | `UAstrawildCaptureComponent` | Creature doc §5 | Resonator + weaken/trust/observe/track/weather → chance roll |
| 8 | Field journal | `UAstrawildJournalSubsystem` | Creature doc §6 | Observation progress, knowledge milestones, research points |
| 9 | Echo roster & party | `UAstrawildEchoRosterSubsystem` | Creature doc §7 | Captured roster (party cap 3), save round-trip |
| 10 | Inventory | `UAstrawildInventoryComponent` | — | Stacks + weight gate (120 kg) + equipment slot |
| 11 | Item registry & content library | `UAstrawildItemRegistrySubsystem`, `UAstrawildContentLibrary` | `ASTRAWILD_ASSET_PIPELINE.md` | Id→definition resolution; CODE_DEFAULT content |
| 12 | Crafting | `UAstrawildCraftingComponent`, `AAstrawildCraftingStationActor` | `ASTRAWILD_CRAFTING_SYSTEM.md` | Timed queue, station/tech/ingredient gates |
| 13 | Building placement | `UAstrawildBuildingComponent`, `AAstrawildBuildingActor` | `ASTRAWILD_BUILDING_SYSTEM.md` | Preview/snap/rotate/validate + server-authoritative placement |
| 14 | Power grid | `UAstrawildPowerSubsystem` | Building doc §7 | Generation/draw/battery, brownout shedding, 2 s re-solve |
| 15 | Echo work | `AAstrawildWorkSiteActor` | Creature doc §8 | Affinity/personality/mood/energy-scaled production |
| 16 | Research / tech tree | `UAstrawildResearchSubsystem` | `ASTRAWILD_RESEARCH_SYSTEM.md` | Points, prerequisites, unlock gates |
| 17 | Quests | `UAstrawildQuestComponent` | `ASTRAWILD_QUEST_SYSTEM.md` | Event-driven objectives, 5-quest First Dawn chain |
| 18 | World state (time/weather) | `AAstrawildGameState`, `UAstrawildTimeSubsystem`, `UAstrawildWeatherSubsystem` | `ASTRAWILD_WORLD_SYSTEM.md` | 24-min days, 8 weather states, replicated |
| 19 | Event bus | `UAstrawildEventBusSubsystem` | Architecture V2 §6 | Decoupled gameplay event pub/sub |
| 20 | Procedural world | `AAstrawildWorldBootstrapper` | World doc §5 | Zero-asset Dawn Fields arena |
| 21 | Save/load | `UAstrawildSaveSubsystem` | `ASTRAWILD_SAVE_SYSTEM.md` | Schema v2, checksum, migration, autosave |
| 22 | HUD | `UAstrawildHudWidget` | `ASTRAWILD_UI_ARCHITECTURE.md` | Pure-C++ HUD |
| 23 | Input | runtime Enhanced Input (in PlayerCharacter) | `ASTRAWILD_INPUT_REFERENCE.md` | 15 runtime actions, full default keymap |
| 24 | Gameplay tags | `AstrawildGameplayTags.h/.cpp` | `ASTRAWILD_GAMEPLAY_TAGS.md` | 77 native tags |
| 25 | Debug/cheats | `UAstrawildCheatManager` | Input Reference §3 | 12 console commands |
| 26 | NPCs | `AAstrawildNPCCharacter` | Quest doc §6 | Quest-offering interactable; schedule PLANNED |
| 27 | Game mode / session | `AAstrawildGameMode` | — | Bootstrapper spawn, respawn (5 s), autosave (300 s) |
| 28 | Tests | `AstrawildAutomationTests.cpp` | `ASTRAWILD_TEST_PLAN.md` | 8 automation tests |

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
5. **Quest: Dawn Guard** — defeat 3 Gloomfangs (night hostiles) → Ancient Core + 20 RP. Chain ends here.

Each quest auto-activates the next on completion (`NextQuestId` chaining).

---

## 5. Feedback Surfaces

| Channel | Implementation |
|---|---|
| HUD | Bars (HP/stamina/hunger/thirst), time/weather, quest tracker, interaction prompt, live capture chance %, party command, notification line |
| Delegates | Every system broadcasts `On*` dynamic multicast delegates (`OnStatsChanged`, `OnDamaged`, `OnQuestStateChanged`, `OnTechUnlocked`, `OnRosterChanged`, …) for future UI/audio |
| Event bus | Cross-cutting gameplay events for quests/journal/analytics |
| Logging | 8 categories (`LogAstrawild` + 7 subsystem categories) |

---

## 6. Honest Status Notes

- All systems above are **implemented in C++ but not yet compiled** (sandbox has no UE5 toolchain). Compile
  validation happens on the target Windows machine — see `ASTRAWILD_TEST_PLAN.md` §4.
- Quest objective types `ReachLocation`, `ObserveEcho`, `SurviveTime` exist in the enum and quest data but
  **have no event wiring yet** (see `ASTRAWILD_QUEST_SYSTEM.md` §5).
- Work-site output accumulates on the site (`StoredOutput`) but a player "collect output" interact is
  **not yet wired** — `CollectOutput()` exists as an API.
