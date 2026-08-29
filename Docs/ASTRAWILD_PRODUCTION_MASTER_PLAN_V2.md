# ASTRAWILD — PRODUCTION MASTER PLAN V2

## Status
Production planning directive — UE5 only

## Vision
ASTRAWILD is a third-person open-world cooperative survival adventure RPG built entirely in Unreal Engine 5. Palworld and ARK are reference points for creature companionship, survival, crafting, base building and world simulation only. ASTRAWILD must remain an original IP with original creatures, technology, world, story, art and gameplay identity.

## Core fantasy
Explore a recovering world, build relationships with Echo creatures, combine creature abilities with advanced technology, establish an evolving base, and uncover the cause of the First Dawn.

## Technology identity
ASTRAWILD is not primitive survival only. Progression deliberately moves from natural survival to advanced technology:

Primitive → Mechanical → Electrical → Energy → Ancient Technology → Echo Technology

Advanced systems planned for production:
- Modular powered exosuits / armor
- Energy shields
- Energy weapons
- Laser weapons
- Plasma weapons
- Rail/coil technology where appropriate to the fiction
- Missiles and guided projectiles
- Drones
- Autonomous utility robots
- Combat robots / sentry platforms
- Power generators and batteries
- Research laboratories
- Scanners and field technology
- Advanced traversal equipment
- Technology-assisted creature interaction

Technology must interact with survival, exploration, creatures and base systems rather than being a separate weapon menu.

---

# 1. PRODUCTION RULES

1. Unreal Engine 5 is the only game runtime/platform. No web game replacement.
2. C++ owns core gameplay architecture; Blueprint owns content assembly, tuning and presentation where appropriate.
3. Data-driven definitions are mandatory for content-heavy systems.
4. Multiplayer authority is considered from the beginning even when implementing single-player first.
5. Save/load is part of feature completion, not a final polish task.
6. Every milestone must compile, run, test and document its result.
7. Never declare a feature complete from documentation alone.
8. Never use placeholder assets as release assets. Track all temporary assets.
9. Do not copy copyrighted designs, code, characters, creatures, maps, UI or assets from reference games.
10. Prefer scalable systems over one-off demonstrations.

---

# 2. TARGET GAME SYSTEMS

## Player
- Third-person movement
- Sprint
- Jump
- Dodge
- Climb/traversal
- Swim
- Glide/advanced traversal later
- Interaction
- Equipment
- Armor/exosuit
- Weapons
- Abilities
- Survival
- Inventory
- Crafting
- Technology
- Field Journal

## Survival
- Health
- Stamina
- Hunger
- Thirst
- Temperature
- Poison/status effects
- Environmental hazards
- Food and medicine
- Rest/safe areas

## Combat
- Melee
- Ranged
- Dodge/parry where appropriate
- Targeting
- Hit reactions
- Stagger
- Armor/damage mitigation
- Element/status effects
- Energy weapons
- Laser weapons
- Plasma weapons
- Explosive/missile weapons
- Creature combat
- Robot combat
- Boss encounters

## Creatures / Echo
- Species definitions
- Individual instances
- Stats
- Traits
- Personality
- Needs
- Abilities
- Combat roles
- Utility roles
- Work roles
- Relationships
- Growth/progression
- Habitat
- Diet
- Time/weather behavior
- Capture
- Storage
- Breeding/genetics only if it supports the design and production scope

## Base
- Modular building
- Storage
- Crafting stations
- Farming
- Creature housing
- Power generation
- Batteries
- Power grid
- Automation
- Research
- Defense
- Robots
- Decorations
- Permissions/ownership

## World
- World Partition
- Biomes
- Day/night
- Weather
- Ecosystem
- Dynamic events
- Resource spawning
- Creature migration
- Dungeons
- Ruins
- World bosses
- Fast travel
- Traversal gates

## Technology
- Research tree
- Equipment tiers
- Armor tiers
- Weapon tiers
- Power technology
- Automation
- Robotics
- Scanning
- Ancient technology
- Echo technology

## NPC / Story
- NPC schedules
- Dialogue
- Factions
- Reputation foundation
- Main quests
- Side quests
- World events
- Story state
- Discoverable lore

## Multiplayer
- 1–4 player co-op target
- Server-authoritative gameplay
- Replication
- Ownership
- Shared world state
- Shared/individual progression rules defined before implementation
- Host/listen-server first; dedicated server later if justified

---

# 3. ADVANCED TECHNOLOGY SYSTEM

Technology must have gameplay consequences.

## Armor / Exosuit
Create an extensible equipment framework rather than a single armor class.

Armor slots may include:
- Helmet
- Chest
- Arms
- Legs
- Boots
- Backpack/core module
- Optional exosuit frame

Armor properties:
- Defense
- Resistances
- Temperature protection
- Environmental protection
- Energy capacity
- Mobility modifiers
- Stamina modifiers
- Module slots
- Durability where appropriate
- Set bonuses only when they create meaningful build choices

Exosuit modules:
- Shield generator
- Jet/boost module
- Thermal regulator
- Scanner enhancement
- Energy capacitor
- Climbing assist
- Carry capacity module
- Emergency recovery

Do not make the player permanently overpowered. Advanced gear should open new choices and areas.

## Weapons
Create a common weapon/ability framework supporting:
- Melee
- Bow/energy bow
- Pistol
- Rifle
- Shotgun equivalent
- Laser
- Plasma
- Energy cannon
- Missile launcher
- Guided projectile system
- Utility weapon/tool

Each weapon should support appropriate:
- Damage type
- Energy/ammo model
- Heat/overheat if applicable
- Reload/cooldown
- Range
- Accuracy
- Mods
- Durability/maintenance if used
- Animation
- VFX
- Audio
- Multiplayer authority

## Robots
Robots are a distinct technology layer.

Utility robots:
- Carrying
- Farming assistance
- Mining assistance
- Repair
- Logistics
- Scouting

Defense robots:
- Sentry
- Drone
- Mobile defender

Robots must use the same scalable AI/data architecture principles as Echo, while remaining mechanically distinct.

Robots should consume resources/power where appropriate and must integrate with the base power system.

## Drones
Drone framework should support:
- Scout drone
- Camera/scanner drone
- Resource survey
- Combat support only at later progression
- Remote interaction

Do not implement all drone types in the first slice. Build the framework, then one complete drone.

---

# 4. SYSTEM INTERACTION REQUIREMENT

Advanced technology must connect to the rest of the game.

Example:

Discover rare Echo
→ scan behavior
→ recover ancient component
→ research component
→ unlock energy technology
→ build generator
→ generate power
→ craft exosuit module
→ reach previously inaccessible cold/high/toxic area
→ discover new Echo/resource

The player should feel that systems create new possibilities rather than isolated feature checkboxes.

---

# 5. PRODUCTION PHASES

## P0 — Audit
- Inspect repository and all existing docs/code.
- Build current UE5 project.
- Record current state.
- Classify KEEP / REFACTOR / REPLACE / REMOVE / ADD.
- Produce architecture gap report.

## P1 — Foundation
- UE5 project/build configuration
- C++ module structure
- gameplay subsystems
- logging
- gameplay tags
- data assets
- interfaces
- save ID strategy
- input architecture
- test framework

## P2 — Player
- movement
- camera
- interaction
- input
- traversal foundation
- animation architecture

## P3 — Survival + Inventory
- survival
- status
- inventory
- item definitions
- equipment
- food
- medicine
- save/load

## P4 — Combat
- damage
- weapons
- abilities
- hit reactions
- death
- status effects
- first enemy

## P5 — First Complete Echo
Build one production-quality Echo before scaling content.
It must have:
- AI
- personality
- needs
- navigation
- combat
- capture
- ownership
- commands
- relationship
- save/load
- one utility role

## P6 — Echo Platform
Generalize the framework so additional Echo can be created mostly through data/content rather than rewriting core systems.

## P7 — Crafting + Base
- crafting
- stations
- modular building
- storage
- farming foundation
- base save/load

## P8 — Power + Automation
- generator
- battery
- power network
- consumers
- priority
- failure states
- first automation loop

## P9 — Technology + Advanced Gear
First complete technology vertical slice:
- research
- armor/exosuit
- one laser weapon
- one advanced ranged weapon
- one utility drone or robot
- energy system
- one traversal upgrade

## P10 — World Simulation
- day/night
- weather
- creature population
- ecosystem tiers
- resource simulation
- dynamic events

## P11 — Dawn Fields Vertical Slice
One polished playable region containing:
- starting area
- 3–5 Echo
- hostile creatures
- resources
- base location
- crafting
- research
- advanced technology unlock
- dungeon
- boss
- quests
- save/load
- weather
- day/night

## P12 — Story/NPC
- NPC
- dialogue
- main quest chain
- side quests
- world state
- lore discovery

## P13 — Multiplayer
- 1–4 players
- authority
- replication
- shared world
- creature ownership
- inventory
- building
- quests
- save/load

## P14 — Expansion
Add additional biomes and content only after the vertical slice is stable.

## P15 — Optimization
Profile CPU/GPU/RAM/VRAM/AI/network/streaming/save/load.

## P16 — QA / Release Foundation
- regression
- automated tests
- runtime tests
- save corruption tests
- multiplayer tests
- performance gates
- packaging
- crash handling

---

# 6. VERTICAL SLICE DEFINITION OF DONE

The first real slice must support:

Start → explore → discover → fight/avoid → capture Echo → gather → craft → build base → assign Echo → research → unlock advanced technology → equip armor/weapon → enter dungeon → fight boss → return → save → quit → reload → continue.

If this complete loop does not work, do not expand to a huge world.

---

# 7. RELEASE-QUALITY GATES

A feature is COMPLETE only if:

- implemented
- compiled
- runtime-tested
- edge-case tested
- save/load tested if persistent
- multiplayer-aware
- profiled when performance-sensitive
- documented
- no known Critical/High blocker

A feature is NOT complete because:
- class exists
- Blueprint exists
- UI button exists
- mock data works
- documentation says complete
- editor preview looks correct

---

# 8. AGENT EXECUTION LOOP

For every milestone:

1. Read current repository.
2. Read relevant production documents.
3. Inspect existing implementation.
4. Make a plan.
5. Implement the smallest complete vertical increment.
6. Compile.
7. Run tests.
8. Run the game.
9. Test the full affected gameplay loop.
10. Fix regressions.
11. Update documentation.
12. Update checklist.
13. Commit with a meaningful message.
14. Push branch/PR.

Never silently skip failed tests.

---

# 9. PRIORITY

When resources are limited, prioritize:

1. Core gameplay stability
2. First complete Echo
3. Combat
4. Capture
5. Inventory/crafting
6. Base building
7. Save/load
8. Power/technology
9. World simulation
10. Advanced armor/weapons/robots
11. Story/content
12. Visual polish

Do not sacrifice core stability to add more features.

---

# 10. PRODUCT IDENTITY CHECK

Every major new feature must answer:

- Does it improve exploration?
- Does it interact with Echo?
- Does it interact with technology?
- Does it create meaningful player choice?
- Does it fit the world fiction?
- Does it scale to multiplayer?
- Does it work with save/load?
- Does it add a reason to return to the world/base?

If the answer is mostly no, challenge the feature before implementing it.
