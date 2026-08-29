# ASTRAWILD — ULTIMATE PRODUCTION ROADMAP V3

## Purpose
This is the long-horizon design and production roadmap for turning ASTRAWILD into a complete UE5 open-world survival creature-tech RPG rather than a feature collection.

The roadmap is intentionally broader than the first vertical slice. Agents MUST NOT implement everything at once. Use dependency order, vertical slices, evidence-based acceptance, and milestone gates.

## 0. Product North Star

ASTRAWILD is an original third-person open-world survival adventure RPG combining:
- creature companionship and collection
- systemic ecosystem simulation
- survival and exploration
- modular base building and automation
- research and technology progression
- advanced armor, energy weapons, drones and robots
- dungeons, bosses and world events
- story, factions and consequences
- 1–4 player cooperative play

Reference inspiration may include the systemic breadth of games such as Palworld and ARK, but all characters, creatures, world, fiction, mechanics presentation, UI, art, audio and assets must remain original.

Palworld demonstrates the commercial appeal of combining creatures, combat, farming, building, automation and co-op; ARK demonstrates the value of taming/breeding, survival, exploration, building and large creature ecosystems. These are design references, not assets or content to copy.

## 1. Core Experience Pillars

1. Explore a world that reacts to the player.
2. Build relationships with original Echo creatures rather than collecting disposable units.
3. Progress from survival technology to advanced energy technology.
4. Make the base a functional home, factory, laboratory and defensive system.
5. Make technology unlock new ways to traverse and understand the world.
6. Let player choices affect creatures, factions, settlements and world states.
7. Make co-op amplify the systems rather than simply adding more players.

## 2. Core Gameplay Loop

Prepare → Explore → Observe → Decide → Fight/Avoid/Capture/Assist → Gather → Return → Heal/Feed/Assign → Craft/Research/Build → Upgrade → Unlock capability → Explore farther.

Every major system should strengthen at least one link in this loop.

## 3. Player Systems

### 3.1 Movement
- walk/run/sprint
- jump
- crouch
- dodge
- slide where appropriate
- climb
- ledge interaction
- swim
- underwater traversal
- grapple/traversal technology later
- vehicle traversal later

### 3.2 Character Progression
- level/experience only if it supports meaningful choices
- attributes
- equipment power
- technology unlocks
- research knowledge
- creature relationship progression
- traversal progression
- faction reputation

Avoid pure vertical stat inflation. Prefer new capabilities and build choices.

### 3.3 Survival
- health
- stamina
- hunger
- thirst
- temperature
- environmental hazards
- status effects
- injuries where useful
- fatigue
- food quality
- medicine

Survival should create decisions, not chores.

## 4. Equipment and Armor — Advanced Technology Layer

Build a modular equipment architecture from the beginning.

### Armor Slots
- helmet
- chest
- arms
- legs
- boots
- back/core
- utility module slots

### Armor Families
- survival gear
- explorer gear
- combat armor
- environmental armor
- energy armor
- exosuit
- advanced relic armor

### Armor Stats
- physical protection
- elemental protection
- thermal protection
- environmental resistance
- stamina efficiency
- mobility
- energy capacity
- shield capacity
- scanner range
- stealth/noise profile

### Armor Modules
- thermal regulator
- cold protection
- heat protection
- radiation/energy anomaly protection
- oxygen/water module
- scanner
- creature analysis module
- shield generator
- energy capacitor
- mobility booster
- jump assist
- grapple module
- stealth module
- emergency medical module
- drone control module

Modules must have trade-offs. Avoid one best-in-slot build.

## 5. Weapon System

Use a unified weapon/ability architecture. UE5 Gameplay Ability System is a suitable foundation for data-driven abilities, attributes, effects and cooldown/resource mechanics.

### Weapon Classes
- primitive melee
- advanced melee
- bows/crossbows if they fit the fiction
- firearms if they fit the setting
- energy rifle
- laser weapon
- plasma/particle weapon
- rail/kinetic advanced weapon
- launcher
- guided missile weapon
- area denial device
- non-lethal capture/utility weapon

### Energy Model
Advanced weapons may consume:
- battery energy
- weapon capacitor
- heat budget
- ammunition
- rare cores

Do not make all advanced weapons simply use infinite mana.

### Combat Features
- hit reactions
- stagger
- weak points
- armor penetration
- elemental interactions
- status effects
- environmental interactions
- projectile physics where valuable
- charge attacks
- weapon overheating
- reload/charge timing
- alternate fire
- weapon attachments/mods

## 6. Drone and Robot Systems

Robotics is a major ASTRAWILD identity layer.

### Utility Drone
First complete drone should support:
- scouting
- scanning
- resource marking
- light inventory transfer
- camera/remote view if useful
- emergency support

### Worker Robot
Can support:
- hauling
- crafting station assistance
- farming
- power maintenance
- repair

### Defense Robot
Can support:
- perimeter patrol
- target identification
- base defense
- alarm
- turret coordination

### Robot Architecture
Robots should share a data-driven AI/command framework with Echo where possible but must not be forced into identical behavior.

Robots need:
- battery/energy
- maintenance
- upgrades
- command modes
- ownership
- save/load
- replication
- failure states

## 7. Creature / Echo System — Ultimate Scope

### Identity
Every Echo species has:
- silhouette
- habitat
- diet
- temperament
- personality archetypes
- social behavior
- time behavior
- weather behavior
- combat role
- utility role
- traversal role
- ecological role
- rarity
- growth/progression

### Individual State
- stable Instance ID
- age
- health
- stamina
- needs
- traits
- personality
- trust
- bond
- memories
- learned abilities
- injuries
- equipment if supported
- work skill
- experience
- genetics/lineage if breeding is approved

### Relationship
Player actions affect:
- trust
- loyalty
- stress
- fear
- confidence
- willingness to follow commands
- work performance
- combat performance

### Commands
- follow
- stay
- defend
- attack
- focus target
- retreat
- protect player
- harvest
- gather
- work
- return home
- use ability

## 8. Creature Breeding / Genetics

Consider as a post-vertical-slice system.

If implemented, it should provide:
- lineage
- inherited traits
- rare variants
- compatibility rules
- breeding facilities
- incubation/hatching where appropriate
- genetic trade-offs
- welfare/needs

Do not turn this into a spreadsheet simulator.

## 9. Ecosystem Simulation

The world should contain:
- producers
- herbivores
- omnivores
- predators
- apex predators
- scavengers
- special anomalous species

Behaviors:
- feeding
- hunting
- fleeing
- resting
- socializing
- territory
- migration
- reproduction where appropriate
- population pressure
- weather response

Use simulation tiers:
Tier 0 near player = detailed
Tier 1 nearby = reduced
Tier 2 far = abstract
Tier 3 very far = world-level population model

Do not run expensive AI for every entity in a large world.

## 10. World Simulation

### Time
- day/night
- dawn/dusk
- season-ready architecture

### Weather
- clear
- cloudy
- rain
- storm
- fog
- heat wave
- cold wave
- rare anomaly weather

Weather changes:
- creature behavior
- resource availability
- visibility
- traversal
- combat
- hazards
- world events

### World Events
- creature migration
- storms
- resource blooms
- ancient machine activation
- faction conflict
- emergency distress event
- rare creature appearance
- world boss
- anomaly

## 11. Biome Architecture

Launch foundation should support 5 major regions:
1. Dawn Fields
2. Luminous Rainforest
3. Salt Plains
4. Azure Snowline
5. Veldara Megacity Ruins

Each biome requires:
- visual identity
- climate
- ecosystem
- resource table
- creature table
- traversal challenge
- unique technology requirement
- landmarks
- dungeon
- story content
- world event
- boss/elite encounter

## 12. World Building Technology

Use UE5 systems appropriate to scale:
- World Partition
- One File Per Actor
- Data Layers
- Level Instances
- HLOD
- PCG for controlled procedural content
- Nanite where appropriate
- Lumen where appropriate
- Large World Coordinates where relevant

PCG should assist with biome/vegetation/resource variation, but important authored locations, quests, dungeons and story spaces should remain designer-controlled.

## 13. Exploration Systems

Add:
- map
- compass
- landmarks
- field journal
- scanner
- creature tracking
- footprints/traces
- environmental clues
- hidden passages
- secret rooms
- ancient terminals
- traversal gates
- fast travel later

### Knowledge Progression
Scanning/observing should unlock knowledge, not merely XP:
- species info
- weaknesses
- diet
- habitat
- behavior
- technology fragments
- world lore

## 14. Base Building — Full System

### Structures
- foundation
- floor
- wall
- roof
- stairs
- ramps
- doors
- windows
- defensive walls
- gates
- bridges

### Functional Rooms
- storage
- workshop
- forge
- laboratory
- kitchen
- medical bay
- creature housing
- breeding facility if enabled
- power room
- command room
- drone bay
- robot bay
- armory

### Base Systems
- power
- water
- food
- storage logistics
- production chains
- defense
- maintenance
- automation
- permissions

## 15. Factory / Automation

Create production chains such as:
Resource → Processing → Component → Advanced Component → Equipment.

Support:
- conveyor/logistics where appropriate
- storage routing
- worker assignments
- robot assistance
- power priorities
- production queues
- maintenance
- production failure

Avoid copying any specific game's UI or exact automation implementation.

## 16. Farming

- crops
- growth stages
- water
- soil quality
- fertilizer
- weather influence
- creature assistance
- harvest
- food quality
- rare plants

## 17. Research / Technology Tree

Technology eras:
1. Survival
2. Primitive
3. Mechanical
4. Electrical
5. Energy
6. Ancient
7. Echo/Organic Technology
8. Advanced Synthesis

Unlock categories:
- weapons
- armor
- traversal
- base
- power
- robotics
- creature research
- environmental protection
- scanning
- crafting

## 18. Traversal Technology

Late-game traversal should fundamentally change exploration:
- grapple
- glider
- powered jump
- climbing gear
- underwater gear
- environmental armor
- hover/vehicle prototype
- creature traversal
- advanced transport

Traversal upgrades should open old locations in new ways.

## 19. Vehicles

Post-vertical-slice.

Possible original systems:
- light rover
- utility transport
- hover bike
- watercraft
- creature-assisted transport

Vehicles need:
- fuel/energy
- durability
- storage
- upgrades
- repair
- save/load
- multiplayer authority

## 20. Combat Encounters

Combat should support:
- solo
- creature-assisted
- co-op
- stealth/avoidance
- environmental tactics
- weak points
- status combinations
- terrain
- destructible/interactive elements where performance permits

## 21. Bosses

Boss architecture:
- phases
- telegraphs
- weak points
- arena hazards
- adds
- environmental interactions
- behavior transitions
- loot/research rewards
- unique technology reward

Never solve difficulty by only increasing HP.

## 22. Dungeons

Each dungeon should have:
- theme
- traversal puzzle
- combat space
- lore
- resource risk
- elite encounter
- mini-boss
- boss or major reward
- shortcut
- secret

## 23. NPC / Factions

Add faction system:
- settlements
- researchers
- scavengers
- industrial survivors
- preservationists
- technology cult/ancient-tech faction if appropriate
- hostile factions

Systems:
- reputation
- faction relations
- vendors
- schedules
- dialogue
- quests
- consequences
- world-state changes

## 24. Quest System

Quest types:
- main story
- side story
- exploration
- creature research
- faction
- rescue
- hunting
- technology recovery
- dungeon
- world event

Support:
- branching objectives
- optional objectives
- conditions
- consequences
- rewards
- world state

## 25. Story Architecture

Avoid exposition dumps.
Use:
- environmental storytelling
- field journal
- NPC conversations
- ancient recordings
- ruins
- creature behavior
- technology discoveries

Central mystery:
Why did the First Dawn collapse, and what is the relationship between the Echoes and the ancient technology?

## 26. Player Choice

Choices may affect:
- faction reputation
- creature populations
- settlement safety
- available quests
- research paths
- certain world events

Avoid branching the entire game into impossible content. Use controlled world-state branches.

## 27. Multiplayer / Co-op

Target 1–4 players.

Server-authoritative for:
- damage
- inventory
- capture
- creature ownership
- building
- quest state
- world state
- valuable item creation

Co-op features:
- party
- ping
- shared objectives
- permissions
- base ownership
- revive
- trading where appropriate
- shared exploration
- individual creature ownership

## 28. Social / Quality of Life

- ping system
- map markers
- quick slots
- favorite creatures
- compare equipment
- sorting/filtering
- crafting queue
- storage search
- creature management
- loadout presets
- build presets later

## 29. Accessibility

- subtitles
- subtitle size
- colorblind options
- UI scaling
- remappable controls
- controller vibration toggle
- aim assistance options
- hold/toggle options
- motion reduction
- camera sensitivity
- field of view
- text readability

## 30. UI/UX

UI should be information-dense but readable.
Core screens:
- main menu
- HUD
- inventory
- equipment
- creature roster
- creature detail
- crafting
- building
- research
- map
- journal
- quests
- technology
- settings
- multiplayer

## 31. Audio

- dynamic ambience
- creature calls
- biome-specific soundscape
- weather audio
- combat audio
- UI feedback
- technology hums
- base machinery
- dynamic music layers

## 32. VFX

- elemental effects
- laser
- plasma
- shields
- energy impacts
- weather
- creature abilities
- environmental anomalies
- base power

VFX must have performance tiers.

## 33. Animation

Architecture should support:
- locomotion
- combat
- interaction
- creature movement
- creature combat
- work animations
- traversal
- armor equipment
- weapon handling

Use animation reuse carefully but preserve creature identity.

## 34. Technical Architecture

Use appropriate UE5 systems rather than custom reinvention:
- Gameplay Ability System for abilities/effects/attributes where justified
- Gameplay Tags
- Enhanced Input
- StateTree and/or Behavior Trees for AI depending on behavior complexity
- Navigation/EQS/AI Perception
- Mass Entity only for suitable large-scale crowds/simulation, not every gameplay Actor
- World Partition
- PCG
- CommonUI/UMG as appropriate
- Subsystems
- Actor Components
- Data Assets
- Replication framework

Do not adopt a framework simply because it exists. Measure complexity and payoff.

## 35. Save Architecture

Persist:
- player
- inventory
- equipment
- creatures
- bases
- buildings
- technology
- research
- quests
- factions
- world events
- discovered locations
- world state

Requirements:
- stable IDs
- versioning
- migration
- corruption detection strategy
- atomic/robust save strategy
- test coverage

## 36. Security / Anti-Cheat Foundation

For co-op:
- server authority
- validate inventory operations
- validate damage
- validate item creation
- validate capture
- validate building ownership
- reject impossible client state

Do not rely on client claims for valuable state.

## 37. Performance Architecture

Targets must be measured on a defined PC baseline.

Audit:
- CPU
- GPU
- memory
- VRAM
- streaming
- shader compilation
- AI
- animation
- physics
- network
- save/load

Use profiling evidence.

## 38. Build / CI / QA

Automate where possible:
- compile
- unit tests
- automation tests
- cooking
- packaging
- smoke tests

Maintain:
- build report
- crash report
- QA report
- milestone report
- known issues

## 39. Content Production Strategy

Do not build 100 shallow systems.
Build vertical slices.

### Vertical Slice A
- Dawn Fields
- player
- 3–5 Echo
- survival
- inventory
- combat
- capture
- crafting
- small base
- power
- research
- one advanced armor module
- one laser/energy weapon
- one drone or robot
- one dungeon
- one boss
- save/load

### Vertical Slice B
Expand Dawn Fields and add:
- more Echo
- factions
- NPCs
- quest chain
- advanced base
- automation
- co-op test

### Vertical Slice C
Add second biome and traversal gate.

## 40. Endgame Architecture

Endgame should not simply be "bigger numbers".
Potential endgame loops:
- rare Echo research
- world anomalies
- high-tier dungeons
- advanced technology hunts
- faction outcomes
- base automation optimization
- boss rematches with modifiers
- exploration completion
- hidden technology
- co-op challenges

## 41. Post-Launch Ready Architecture

Only after the core game is stable:
- additional regions
- additional Echo
- additional quests
- challenge modes
- cosmetic customization
- seasonal events if appropriate

Avoid designing the base game around monetization.

## 42. Production Gates

GATE A — Buildable Foundation
GATE B — Playable Character
GATE C — First Echo
GATE D — Core Loop
GATE E — Vertical Slice
GATE F — Co-op Slice
GATE G — Content Alpha
GATE H — Feature Complete
GATE I — Optimization Beta
GATE J — Release Candidate

No gate passes based on documentation alone.

## 43. Agent Rules

Every milestone must:
1. Read current repository state.
2. Read relevant roadmap/checklist.
3. Inspect dependencies.
4. Implement smallest coherent increment.
5. Compile.
6. Run tests.
7. Launch/test gameplay when applicable.
8. Test save/load when applicable.
9. Profile if performance-sensitive.
10. Update checklist and milestone report.
11. Commit logically.

If an existing architecture prevents the roadmap, stop and document the architectural decision before performing a destructive rewrite.

## 44. Ultimate Definition of Done

ASTRAWILD is not "complete" until the shipped scope is:
- playable from new game to ending/endgame
- stable through save/reload
- performant on the defined baseline PC
- controller and accessibility ready
- content complete for the chosen launch scope
- multiplayer stable if co-op is included in launch scope
- no unresolved Critical defects
- no core-loop High defects
- all release assets have known provenance/licensing
- documentation matches implementation
- package build succeeds
- recovery from common failures is tested

The roadmap is deliberately ambitious. Production priority is always:
STABILITY → CORE FUN → SYSTEM INTEGRATION → CONTENT → POLISH → SCALE.
