# ASTRAWILD — PRODUCTION CHECKLIST V2

This file is the machine-readable human-readable acceptance checklist for AI agents. Never mark an item complete from documentation alone. Use `[x]` only when implementation exists and has been verified.

> **Evidence source of truth (2026-08-30):** every item below was audited with file:line
> evidence in `Docs/ASTRAWILD_UE5_PRODUCTION_AUDIT.md`; priorities and batch plans live in
> `Docs/ASTRAWILD_IMPLEMENTATION_GAP_REPORT.md` + `Docs/ASTRAWILD_ULTIMATE_GAP_ANALYSIS.md`.
> `[~]` marks implementation that exists in source but is NOT engine-verified (the project
> has never been compiled — sandbox has no UE). Nothing in this file is `[x]`-verified yet.

Legend:
- `[ ]` Not started
- `[~]` In progress / partial
- `[x]` Implemented and verified
- `[!]` Blocked / failed

## P0 Audit
- [x] Repository audit completed
- [!] Existing UE5 project builds — never compiled; 2 latent compile errors found by audit (C-1, C-1b) and fixed in source; engine build pending
- [x] Existing systems classified KEEP/REFACTOR/REPLACE/REMOVE/ADD
- [x] Architecture gaps documented
- [x] Technical debt documented
- [!] Current baseline build recorded — baseline = "does not compile" until proven otherwise on the target machine

## P1 Foundation
- [ ] UE5 version locked/documented
- [ ] C++ architecture established
- [ ] Gameplay subsystems established
- [ ] Logging categories established
- [ ] Gameplay Tags taxonomy established
- [ ] Data Asset conventions established
- [ ] Stable ID strategy established
- [ ] Enhanced Input architecture established
- [ ] Automated test foundation established

## P2 Player
- [ ] Third-person character
- [ ] Camera
- [ ] Movement
- [ ] Sprint
- [ ] Jump
- [ ] Dodge
- [ ] Interaction
- [ ] Controller support
- [ ] Input remapping
- [ ] Traversal foundation
- [ ] Animation architecture

## P3 Survival
- [ ] Health
- [ ] Stamina
- [ ] Hunger
- [ ] Thirst
- [ ] Temperature
- [ ] Status effects
- [ ] Food
- [ ] Medicine
- [ ] Death
- [ ] Respawn

## P3 Inventory / Equipment
- [ ] Item Definition
- [ ] Item Instance
- [ ] Stack
- [ ] Weight
- [ ] Equipment
- [ ] Inventory UI
- [ ] Drop/pickup
- [ ] Transfer
- [ ] Persistence
- [ ] Duplication tests

## P4 Combat
- [ ] Damage framework
- [ ] Melee
- [ ] Ranged
- [ ] Dodge/parry architecture
- [ ] Hit reaction
- [ ] Stagger
- [ ] Death
- [ ] Status effects
- [ ] Element system
- [ ] First hostile enemy

## P5 First Complete Echo
- [~] Species Definition
- [~] Instance state
- [~] AI Controller
- [~] Navigation — runtime navmesh via navigation invokers implemented (Batch 1 C-3); runtime-unverified
- [~] Perception
- [~] Behavior
- [~] Personality
- [~] Needs
- [~] Combat
- [~] Capture
- [~] Ownership
- [~] Follow
- [~] Commands
- [~] Relationship
- [~] Utility role — work sites now interactable (Batch 1 C-7); runtime-unverified
- [~] Save/Load — party respawns on load (Batch 1 H-2); runtime-unverified
- [ ] Edge-case tests

## P6 Echo Platform
- [ ] Data-driven species creation
- [ ] Stats
- [ ] Traits
- [ ] Abilities
- [ ] Habitat
- [ ] Diet
- [ ] Time behavior
- [ ] Weather behavior
- [ ] Work roles
- [ ] Additional Echo content pipeline

## P7 Crafting
- [~] Recipe definition
- [~] Ingredients
- [~] Stations — compile error fixed in source (Batch 1 C-1); station interact path compiles-pending
- [~] Technology requirements
- [ ] Crafting UI
- [~] Output validation — collection path weight-safe; craft output validation still ignores AddItem result (gap H-11)
- [~] Save/load compatibility

## P7 Base Building
- [ ] Foundation
- [ ] Floor
- [ ] Wall
- [ ] Roof
- [ ] Door
- [ ] Storage
- [ ] Workstation
- [ ] Placement preview
- [ ] Snap
- [ ] Collision validation
- [ ] Delete
- [ ] Repair
- [ ] Save/load
- [ ] Ownership foundation

## P8 Power / Automation
- [~] Generator
- [~] Battery
- [~] Power network
- [~] Consumers
- [~] Power priority
- [~] Power failure
- [ ] Energy UI
- [~] First automation loop — work-site interact implemented (Batch 1 C-7); runtime-unverified
- [ ] Persistence

## P9 Advanced Technology
- [~] Technology tree — unlock path implemented (Batch 1 C-2: root auto-grant + Research Desk interact); runtime-unverified
- [~] Research system — points/prereqs/unlock logic real; UI-lite (HUD readout) only
- [ ] Armor framework
- [ ] Helmet slot
- [ ] Chest slot
- [ ] Arms slot
- [ ] Legs slot
- [ ] Boots slot
- [ ] Core/backpack module
- [ ] Exosuit framework
- [ ] Shield module
- [ ] Thermal module
- [ ] Scanner module
- [ ] Energy capacitor
- [ ] Traversal module
- [ ] Laser weapon
- [ ] Plasma/advanced energy weapon
- [ ] Missile/guided projectile framework
- [ ] Weapon heat/energy model where appropriate
- [ ] Weapon mod framework
- [ ] Utility drone framework
- [ ] One complete drone
- [ ] Utility robot framework
- [ ] One complete robot
- [ ] Technology integrates with exploration

## P10 World
- [ ] World Partition
- [ ] Day/night
- [ ] Weather
- [ ] Biome definition
- [ ] Resource spawning
- [ ] Creature population
- [ ] Ecosystem tiers
- [ ] Migration foundation
- [ ] Dynamic events
- [ ] Streaming/HLOD baseline

## P11 Dawn Fields Vertical Slice
- [~] Starting area
- [~] 3–5 Echo
- [~] Hostile creatures
- [~] Resources
- [~] Base location
- [~] Crafting
- [~] Research
- [~] Advanced technology unlock
- [~] Dungeon — completion logic fixed (Batch 1 C-4); runtime-unverified
- [~] Boss — phased boss now spawns in the boss room (Batch 1 C-5); runtime-unverified
- [~] Main quest slice — chain code-reachable end-to-end after C-2; hostile respawn still missing (gap H-5 in gap report)
- [~] Weather
- [~] Day/night
- [~] Save/load — vitals/party/building-health restore + LoadLatest (Batch 1 H-1/H-2/H-3/H-5); runtime-unverified
- [~] Full gameplay loop — code-reachable end-to-end without cheats after Batch 1; compile + runtime test pending

## P12 NPC / Story
- [ ] NPC framework
- [ ] Dialogue
- [ ] Schedule
- [ ] Faction foundation
- [ ] Main quest chain
- [ ] Side quest framework
- [ ] World state
- [ ] Lore discovery

## P13 Multiplayer
- [ ] Server authority rules
- [ ] Player replication
- [ ] Creature replication
- [ ] Inventory authority
- [ ] Capture authority
- [ ] Building authority
- [ ] Quest authority
- [ ] Shared world state
- [ ] Save authority
- [ ] Join/leave handling
- [ ] 1–4 player co-op test

## P14 Content Expansion
- [ ] Luminous Rainforest
- [ ] Salt Plains
- [ ] Azure Snowline
- [ ] Veldara Megacity Ruins
- [ ] Additional Echo
- [ ] Additional dungeons
- [ ] Additional bosses

## P15 Optimization
- [ ] CPU profiling
- [ ] GPU profiling
- [ ] RAM profiling
- [ ] VRAM profiling
- [ ] AI profiling
- [ ] Network profiling
- [ ] Streaming profiling
- [ ] Save/load profiling
- [ ] Tick audit
- [ ] AI simulation tier validation
- [ ] 60 FPS target tested on defined baseline hardware

## P16 QA / Release Foundation
- [ ] Automated tests pass
- [ ] Runtime smoke test passes
- [ ] Save/reload test passes
- [ ] Regression test passes
- [ ] Duplication tests pass
- [ ] Crash tests pass
- [ ] Multiplayer regression passes
- [ ] Controller test passes
- [ ] Accessibility settings tested
- [ ] Packaging test passes
- [ ] Placeholder asset audit complete
- [ ] License/asset manifest complete
- [ ] Definition of Done satisfied

## GLOBAL QUALITY GATE
- [!] No Critical issues — all 8 Critical gaps fixed in source (Batch 1); engine verification pending
- [!] No unresolved High issues affecting core loop — 7 of 14 High fixed; see gap report
- [!] Build passes — never compiled
- [ ] Game launches — unverified
- [~] Core loop playable — code-reachable end-to-end; runtime-unverified
- [~] Save/load reliable — 6 defects fixed; runtime-unverified
- [~] Documentation matches implementation — audit docs added; per-system docs one wave stale in places
- [ ] Git working tree clean after commit
