# ASTRAWILD — PRODUCTION CHECKLIST V2

This file is the machine-readable human-readable acceptance checklist for AI agents. Never mark an item complete from documentation alone. Use `[x]` only when implementation exists and has been verified.

Legend:
- `[ ]` Not started
- `[~]` In progress / partial
- `[x]` Implemented and verified
- `[!]` Blocked / failed

## P0 Audit
- [ ] Repository audit completed
- [ ] Existing UE5 project builds
- [ ] Existing systems classified KEEP/REFACTOR/REPLACE/REMOVE/ADD
- [ ] Architecture gaps documented
- [ ] Technical debt documented
- [ ] Current baseline build recorded

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
- [ ] Species Definition
- [ ] Instance state
- [ ] AI Controller
- [ ] Navigation
- [ ] Perception
- [ ] Behavior
- [ ] Personality
- [ ] Needs
- [ ] Combat
- [ ] Capture
- [ ] Ownership
- [ ] Follow
- [ ] Commands
- [ ] Relationship
- [ ] Utility role
- [ ] Save/Load
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
- [ ] Recipe definition
- [ ] Ingredients
- [ ] Stations
- [ ] Technology requirements
- [ ] Crafting UI
- [ ] Output validation
- [ ] Save/load compatibility

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
- [ ] Generator
- [ ] Battery
- [ ] Power network
- [ ] Consumers
- [ ] Power priority
- [ ] Power failure
- [ ] Energy UI
- [ ] First automation loop
- [ ] Persistence

## P9 Advanced Technology
- [ ] Technology tree
- [ ] Research system
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
- [ ] Starting area
- [ ] 3–5 Echo
- [ ] Hostile creatures
- [ ] Resources
- [ ] Base location
- [ ] Crafting
- [ ] Research
- [ ] Advanced technology unlock
- [ ] Dungeon
- [ ] Boss
- [ ] Main quest slice
- [ ] Weather
- [ ] Day/night
- [ ] Save/load
- [ ] Full gameplay loop

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
- [ ] No Critical issues
- [ ] No unresolved High issues affecting core loop
- [ ] Build passes
- [ ] Game launches
- [ ] Core loop playable
- [ ] Save/load reliable
- [ ] Documentation matches implementation
- [ ] Git working tree clean after commit
