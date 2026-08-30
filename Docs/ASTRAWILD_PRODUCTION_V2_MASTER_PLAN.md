# ASTRAWILD — Production V2 Master Plan

**Status:** ACTIVE — Production Expansion
**Target:** Unreal Engine 5.8.2 / Windows x64
**Current baseline:** `eceabd3`
**Primary objective:** Turn the verified UE5 technical prototype into a visually convincing, content-rich, production-quality sci-fi survival game while preserving the working engineering foundation.

> ASTRAWILD takes inspiration from the survival, creature-companion, crafting, base-building and exploration genres represented by games such as ARK and Palworld, but must develop its own setting, creatures, art direction, mechanics, names, assets and identity. Do not copy protected characters, creatures, maps, UI, audio, branding or proprietary assets.

---

## 0. NON-NEGOTIABLE RULES

- Preserve working systems unless a change is required by a verified defect or production requirement.
- Never claim PASS without actually testing the relevant UE5 build/runtime behavior.
- Do not hide failures by disabling systems, removing features, bypassing checks, or replacing real gameplay with fake success states.
- GLM works primarily on GitHub/source/data/specification; it must not assume it can run UE5 in its sandbox.
- Antigravity works primarily on the local Windows UE5 environment and is responsible for real editor/runtime verification.
- Every meaningful change must be documented and committed.
- Avoid uncontrolled scope expansion. Implement the highest-value production slice first, then expand.
- Prefer data-driven systems and reusable assets over one-off hardcoded content.
- Do not introduce a web version. ASTRAWILD is a native Unreal Engine 5 game.

## 1. CURRENT BASELINE

The previous engineering run established:

- UE 5.8.2 local toolchain validated.
- C++ compilation/linking passed.
- Standalone game build passed.
- 495 packages cooked.
- Windows packaged build produced.
- 25/25 automation tests passed.
- Save/Load V3 round-trip tested across multiple cycles.
- Core systems include survival, inventory, equipment, combat, capture, Echo assignment, building, power, research, quests, dungeon/boss framework, drone and robot framework.

### Known baseline limitation

The runtime screenshot revealed that the current world presentation is still a functional graybox/prototype: primitive geometry, sparse environment, limited visual identity, and insufficient production art/content. This is now the primary development bottleneck.

### Known minor technical item

Resource nodes can emit warnings when `ResourceItemId` is not populated during early bootstrap. The final production pipeline should make resource identity deterministic rather than relying on fallback loot.

---

# 2. PRODUCTION DEFINITION OF DONE

ASTRAWILD V2 is considered complete only when all of the following are true:

### Engineering
- [ ] Clean UE5.8.2 Development Editor build.
- [ ] Clean Development Game build.
- [ ] Cook succeeds.
- [ ] Packaged Windows build launches.
- [ ] No Critical or High runtime blockers.
- [ ] Automation tests pass.
- [ ] Save/Load remains deterministic.

### Visual production
- [ ] No dependence on a blank white/primitive-only world for the main gameplay experience.
- [ ] At least one polished biome is fully playable.
- [ ] Production-quality lighting and atmosphere.
- [ ] Foliage, rocks, water, terrain dressing and points of interest create a believable environment.
- [ ] Player, Echoes and important machines have identifiable visual silhouettes.
- [ ] Core weapons and technology have visible VFX.
- [ ] UI uses a coherent ASTRAWILD visual language.

### Gameplay
- [ ] A complete survival → exploration → capture → base → technology → combat → dungeon/boss → progression loop exists.
- [ ] Multiple meaningful Echo roles.
- [ ] Multiple weapons and equipment tiers.
- [ ] Building and power have meaningful tradeoffs.
- [ ] Research unlocks visibly useful technology.
- [ ] Quests guide the player without replacing exploration.
- [ ] World events create reasons to leave the base.

### Player experience
- [ ] A new player can understand movement, survival, inventory, interaction, capture and crafting without developer knowledge.
- [ ] Inputs are discoverable.
- [ ] Combat gives readable feedback.
- [ ] Damage, hit, capture, crafting, research and objective completion have feedback.
- [ ] Save/load does not cause unexplained state loss.

---

# 3. PRODUCTION PRIORITY ORDER

Work in this order. Do not jump to low-value polish while the higher priority layer is broken.

1. **P0 — Foundation safety:** preserve current build/runtime and fix real blockers.
2. **P1 — Visual vertical slice:** transform one starting region from graybox into a convincing biome.
3. **P2 — Core gameplay feel:** movement, camera, interaction, combat, capture, crafting and building.
4. **P3 — Echo ecosystem:** creatures, behavior, roles, progression and visual differentiation.
5. **P4 — Technology:** exosuit, energy weapons, scanner, drone, robots and research.
6. **P5 — World expansion:** additional biomes, landmarks, dungeons, events and bosses.
7. **P6 — UX/audio/VFX:** presentation and feedback pass.
8. **P7 — Performance:** profiling, streaming, AI cost, memory and shader/material optimization.
9. **P8 — Release candidate:** clean package, regression test and final checklist.

---

# 4. VISUAL IDENTITY — ASTRAWILD

## Direction

**Sci-Fi Survival Frontier**: grounded survival environments combined with advanced energy technology and mysterious ancient infrastructure.

Visual ingredients:

- believable natural terrain
- industrial technology
- restrained sci-fi materials
- readable energy effects
- mysterious ancient structures
- strong day/night contrast
- atmospheric weather
- functional rather than decorative machines

Avoid:

- generic white test terrain in the final gameplay area
- random asset-store visual mixing
- excessive neon everywhere
- UI that looks like a debug tool
- direct imitation of another game's creature or interface design

## Art hierarchy

1. Player silhouette
2. Echo silhouettes
3. Major landmarks
4. Base and machines
5. Resource readability
6. Environmental detail
7. VFX
8. micro-detail

---

# 5. WORLD PRODUCTION

## Biome 01 — Verdant Frontier (P1 MUST-HAVE)

Create a complete polished starting biome:

- terrain height variation
- grass and foliage layers
- trees and shrubs
- rocks and cliffs
- streams or ponds
- resource nodes
- wildlife/Echo spawn areas
- abandoned technology
- starter ruins
- caves or entrances
- weather variation
- day/night lighting
- navigational landmarks
- safe starter area
- dangerous perimeter

The player should be able to spend the first several hours here without feeling that the map is empty.

## Biome 02 — Ember Wastes

- dry terrain
- volcanic/thermal features
- heat hazards
- rare minerals
- aggressive Echo variants
- industrial ruins

## Biome 03 — Frost Vale

- snow/ice terrain
- frozen water
- blizzards
- cold survival pressure
- cold-adapted Echoes

## Biome 04 — Toxic Basin

- contamination effects
- toxic atmosphere
- mutated/corrupted ecosystem
- high-tier resources
- environmental hazards

## Biome 05 — Ancient Ruins

- alien/ancient architecture with original ASTRAWILD design
- puzzle/exploration spaces
- lore
- advanced technology
- dungeon access

### World structure

Use reusable procedural/data-driven definitions for:

- biome descriptors
- resource distribution
- spawn zones
- points of interest
- landmarks
- weather regions
- dungeon entrances

---

# 6. ECHO CREATURE SYSTEM

Echoes are a core ASTRAWILD identity feature.

## Target content

Initial production target:

- 12+ distinct Echo concepts for the first complete content pass.
- 20+ for a broader production milestone.
- Multiple rarity tiers.
- Multiple roles.

## Roles

- combat
- gathering
- mining
- farming
- crafting
- power/energy
- transport
- exploration
- healing/support
- base defense

## Behavior

Echo AI should support appropriate combinations of:

- idle
- wander
- sleep/rest
- eat/drink where appropriate
- investigate
- flee
- hunt
- attack
- defend territory
- follow player
- return to base
- work assigned site
- react to weather/environment
- react to nearby Echoes
- react to player actions

## Progression

Echoes should have:

- level
- attributes
- role effectiveness
- personality modifiers
- capture difficulty
- elemental/technology traits where appropriate
- equipment or upgrade hooks where appropriate

Avoid making every Echo a reskinned combat unit.

---

# 7. SURVIVAL SYSTEM

Retain and polish existing:

- health
- stamina
- hunger
- thirst
- temperature/insulation
- environmental damage

Add production-quality feedback:

- status icons
- warning thresholds
- audio cues
- screen/UI feedback
- clear recovery actions

Do not make survival annoying for its own sake. It should encourage preparation and exploration.

---

# 8. COMBAT

## Player combat

Support a progression from primitive/industrial tools to advanced energy weapons.

Required weapon families:

- basic ranged weapon
- Pulse weapon
- Plasma weapon
- Laser weapon
- Arc/electric weapon
- Rail/kinetic weapon
- Missile/lock-on weapon
- experimental endgame weapon

Each weapon should have:

- distinct firing behavior
- ammo/energy cost
- damage profile
- hit feedback
- muzzle/beam/projectile VFX
- impact VFX
- sound hook
- UI/ammo feedback

## Combat readability

Player must be able to understand:

- when a shot fired
- whether it hit
- damage result
- enemy threat
- weak point state
- status effect
- weapon cooldown/ammo state

---

# 9. ARMOR / EXOSUIT

Expand current equipment foundation into a visible technology progression.

Slots should support a coherent set such as:

- helmet
- torso/exosuit
- legs
- boots
- backpack/module
- scanner/core

Tiers:

- Mk I
- Mk II
- Mk III
- advanced/experimental

Potential bonuses:

- defense
- carry capacity
- movement
- heat resistance
- cold resistance
- energy efficiency
- scanner range
- stamina efficiency
- hazard resistance

Every major tier should have a visual difference.

---

# 10. SCANNER

Scanner is a signature exploration tool.

Required features:

- hold-to-scan
- resource identification
- Echo identification
- weak-point/analysis hooks
- unknown signal detection
- POI discovery
- research/lore observations
- readable scan UI

Advanced scanner progression may add:

- longer range
- faster scan
- threat classification
- hidden resource detection
- ancient signal tracking

---

# 11. DRONE SYSTEM

Current drone framework should evolve into a real support system.

Drone functions:

- follow player
- scan nearby entities
- identify resources
- collect lightweight resources where appropriate
- provide navigation/telemetry
- assist with base monitoring

Future upgrades:

- larger battery
- longer range
- better scanner
- remote camera
- emergency support
- advanced harvesting

Drone must have visible model, animation/hover motion and VFX.

---

# 12. ROBOT / AUTOMATION

Expand robot framework into meaningful base automation.

Robot types:

- mining
- farming
- construction
- logistics
- repair
- defense
- combat support

Base automation loop:

`Build Robot → Power Robot → Assign Job → Robot Performs Job → Consume Energy → Produce Result`

Robots must have:

- visible state
- work animation
- battery/energy state
- job assignment UI
- failure/idle state

---

# 13. BASE BUILDING

Expand building into a coherent modular system.

Core pieces:

- foundation
- floor
- wall
- window
- door
- roof
- stairs
- storage
- workbench
- generator
- battery
- research station
- robot bay
- drone station
- defensive turret

Production requirements:

- placement preview
- valid/invalid placement feedback
- snapping
- collision
- repair/damage
- ownership/state persistence
- readable power connections

---

# 14. POWER / ENERGY

Energy should connect technology together.

Loop:

`Generator → Grid → Battery → Consumers`

Consumers:

- lights
- crafting machines
- research
- robots
- drone station
- defenses
- advanced weapons where appropriate

Provide readable UI for:

- generation
- consumption
- stored energy
- brownout
- disconnected devices

Make power a strategic choice, not just a number.

---

# 15. CRAFTING / ECONOMY

Expand content around existing inventory/economy foundations.

Production tiers:

- survival
- industrial
- advanced
- energy
- experimental

Targets:

- 50+ meaningful items over the broader production milestone.
- 40+ recipes.
- No filler items solely to inflate counts.

Every major resource should have a reason to exist.

---

# 16. RESEARCH / TECHNOLOGY TREE

Research should create a clear progression:

`Explore → Discover → Research → Unlock → Craft → Use → Reach new area`

Branches:

- survival
- tools
- weapons
- armor
- energy
- automation
- scanner
- Echo technology
- exploration

Research UI must clearly show:

- requirement
- cost
- unlocked feature
- next tier

---

# 17. QUEST / STORY

Quests should support exploration rather than turn the game into a checklist simulator.

Target broader production content:

- onboarding
- first survival
- first Echo
- first base
- first power system
- first research
- advanced technology
- ancient signal
- first dungeon
- boss progression
- endgame mystery

Quest objectives should use existing objective framework wherever possible.

---

# 18. DUNGEONS / BOSSES

Dungeon design requirements:

- entrance landmark
- combat spaces
- exploration space
- resource/reward logic
- environmental storytelling
- checkpoint or sensible recovery rules
- final encounter

Boss requirements:

- multiple phases
- telegraphs
- weak points
- hazards
- readable attacks
- meaningful rewards
- visual/audio escalation

Bosses must not be bullet sponges only.

---

# 19. WORLD EVENTS / WEATHER

Add reasons to explore and react.

Events:

- storms
- creature migration
- resource surge
- supply drop
- ancient signal
- night raid
- meteor/impact event
- rare Echo appearance
- boss event

Events should be data-driven and save-safe.

---

# 20. UI / UX

Replace debug-like presentation with a coherent ASTRAWILD interface.

Core screens:

- HUD
- inventory
- equipment
- crafting
- research
- quest
- map
- Echo party
- base/power management
- scanner
- pause/settings

HUD should communicate:

- health
- stamina
- survival states
- quickbar
- ammo/energy
- active status
- scan state
- objective

UX requirements:

- controller support
- keyboard/mouse support
- remappable input hooks where feasible
- input hints
- clear confirmation/error states

---

# 21. VFX / NIAGARA

Priority VFX:

- laser beam
- plasma projectile
- energy impact
- missile trail
- explosion
- shield hit
- scanner pulse
- capture effect
- research/technology activation
- power grid state
- boss telegraph
- environmental weather

VFX must be readable at gameplay distance and performance-aware.

---

# 22. AUDIO

Introduce a production audio plan:

- footsteps by surface
- weapon fire
- impact
- capture
- Echo vocalization
- machine hum
- power activation
- crafting
- scanner
- UI
- weather
- ambient biome loops
- boss music/intensity hooks

Use original/licensed assets only.

---

# 23. ANIMATION

Minimum important animation coverage:

Player:

- idle
- walk
- run
- sprint
- jump/fall
- interaction
- gathering
- weapon fire/reload where applicable
- damage
- dodge

Echo:

- idle
- locomotion
- attack
- damage
- death/defeat
- work
- follow
- special ability

Robots/drone:

- idle
- movement
- work
- interaction
- damaged/disabled states where applicable

If final character assets are not yet available, use clearly temporary placeholders but keep the asset interfaces production-ready.

---

# 24. PERFORMANCE

Target a stable playable experience before adding expensive effects.

Profile:

- CPU frame time
- GPU frame time
- VRAM
- RAM
- AI tick cost
- navigation
- procedural generation
- world streaming
- draw calls
- shader complexity
- Niagara particle count

Avoid unnecessary per-frame Tick. Prefer event-driven or timed updates where practical.

---

# 25. SAVE / LOAD

Every new persistent system must be evaluated for save requirements.

At minimum verify:

- player
- inventory
- equipment
- survival state where intended
- Echo state
- Echo assignments
- robot assignments
- drone state
- buildings
- building health/state
- power
- battery
- research
- quests
- discoveries
- world event state where intended

Test:

`New Game → Progress → Save → Quit → Relaunch → Load → Compare State`

No silent state loss.

---

# 26. QA GATES

## Gate A — Engineering

- [ ] Compile
- [ ] Link
- [ ] UHT
- [ ] Automation tests

## Gate B — Runtime

- [ ] Editor startup
- [ ] Standalone startup
- [ ] Packaged startup
- [ ] No crash

## Gate C — Gameplay

- [ ] Movement
- [ ] Survival
- [ ] Gathering
- [ ] Crafting
- [ ] Capture
- [ ] Echo
- [ ] Building
- [ ] Power
- [ ] Research
- [ ] Combat
- [ ] Dungeon
- [ ] Boss

## Gate D — Persistence

- [ ] Save
- [ ] Quit
- [ ] Relaunch
- [ ] Load
- [ ] Verify

## Gate E — Presentation

- [ ] Environment
- [ ] Lighting
- [ ] Materials
- [ ] VFX
- [ ] UI
- [ ] Audio hooks
- [ ] Animation

## Gate F — Performance

- [ ] No runaway tick
- [ ] No obvious memory leak
- [ ] No uncontrolled actor spawning
- [ ] Acceptable frame time

---

# 27. GLM 5.3 RESPONSIBILITIES

GLM is the **GitHub-side architect/content/source engineer**.

GLM should:

1. Read this document before every production batch.
2. Inspect the current repository before changing anything.
3. Maintain the existing architecture.
4. Implement/refine C++ systems that are needed for production content.
5. Create/update data-driven definitions, registries, structs, enums and gameplay data.
6. Define asset contracts and Blueprint requirements for systems that Antigravity must build in UE5.
7. Expand creatures, items, weapons, armor, quests and technology using reusable data.
8. Fix source-level bugs discovered by Antigravity.
9. Never pretend that a UE5 asset/runtime change has been verified if it was not run in UE5.
10. Commit coherent batches and update the worklog.

GLM must prioritize P0/P1/P2 work before speculative systems.

---

# 28. ANTIGRAVITY RESPONSIBILITIES

Antigravity is the **local UE5 production/integration/QA engineer**.

Antigravity should:

1. Pull the latest GitHub main branch before a new batch.
2. Read this document and the GLM worklog.
3. Implement UE5-side work in the actual project.
4. Create/configure Blueprint assets where appropriate.
5. Build Landscape/World/Level content.
6. Build Materials, Niagara, lighting and environment presentation.
7. Integrate C++ systems with real UE5 assets.
8. Run actual compile/cook/package/runtime tests.
9. Perform human-like gameplay verification where possible.
10. Record failures with exact logs and reproduction steps.
11. Commit UE5-side source/config/content changes where repository-managed.
12. Never report PASS based solely on source inspection.

---

# 29. HANDOFF PROTOCOL

### GLM → Antigravity

GLM must finish with:

- changed files
- intended UE5 integration points
- new data/assets expected
- test expectations
- known limitations
- commit SHA

Then Antigravity pulls the commit.

### Antigravity → GLM

If a source-level blocker is found, report:

- exact file
- exact compiler/runtime error
- reproduction steps
- suspected cause
- whether the issue is deterministic
- minimal suggested fix if obvious
- current commit

Do not rewrite architecture merely to bypass the error.

---

# 30. AUTONOMOUS BATCH RULE

Both agents may work for long unattended periods, but each batch must have a bounded objective.

A batch ends when:

- the requested milestone is complete, OR
- a blocking issue requires the other agent, OR
- the agent reaches the defined quality gate.

At the end of every batch update:

`Docs/ASTRAWILD_WORKLOG.md`

or the repository's current worklog file if a newer canonical file exists.

Never leave the repository in a state where another agent cannot determine what changed.

---

# 31. IMMEDIATE NEXT MILESTONE — VISUAL VERTICAL SLICE

This is the next priority after the current verified build.

## Objective

Transform the starting area into a believable ASTRAWILD gameplay space.

### Required

- [ ] Production-direction landscape
- [ ] Terrain material
- [ ] Foliage
- [ ] Rocks
- [ ] Water feature
- [ ] Atmospheric sky/fog
- [ ] Day/night lighting
- [ ] Starter base location
- [ ] Resource clusters
- [ ] Echo spawn zones
- [ ] At least one landmark
- [ ] Ancient technology POI
- [ ] Basic VFX
- [ ] Coherent HUD
- [ ] Player/Echo visual placeholders at minimum

### Acceptance test

A screenshot of the starting area should no longer look like a default UE5 test map. A new player should immediately understand that they are in a sci-fi survival world.

---

# 32. CONTENT TARGETS

These are production targets, not excuses to inflate counts.

### First polished vertical slice

- 1 polished biome
- 4–6 distinct Echoes
- 15–20 items
- 10–15 recipes
- 4–6 quests
- 3+ weapons
- 2+ armor tiers
- 1 drone
- 2 robot types
- 1 dungeon
- 1 boss
- 1 complete research branch
- 1 complete base/power loop

### Broader production milestone

- 5 biomes
- 20+ distinct Echoes
- 50+ meaningful items
- 40+ recipes
- 20+ quests
- 8+ weapon families/variants
- 4+ armor tiers
- multiple drones/robots/upgrades
- multiple dungeons
- multiple bosses
- world events
- complete progression loop

---

# 33. RELEASE CANDIDATE CHECKLIST

- [ ] Git clean and reproducible.
- [ ] Documentation synchronized.
- [ ] UE project opens cleanly.
- [ ] Development Editor builds.
- [ ] Development Game builds.
- [ ] Cook succeeds.
- [ ] Package succeeds.
- [ ] Game launches.
- [ ] New Game works.
- [ ] Core gameplay works.
- [ ] Visual vertical slice looks production-directed.
- [ ] Save/load passes.
- [ ] 25+ automated tests or updated equivalent pass.
- [ ] Human playtest completed.
- [ ] Critical = 0.
- [ ] High = 0.
- [ ] Performance blockers = 0.
- [ ] No known data corruption.
- [ ] Packaged build can be launched from a clean output directory.

---

# 34. FINAL STOP CONDITION

Do not declare ASTRAWILD V2 complete until:

1. The current UE5 foundation remains stable.
2. At least one polished playable biome exists.
3. The complete core gameplay loop is playable.
4. Echoes are visibly and mechanically meaningful.
5. Advanced technology is visible and usable.
6. Base automation is functional.
7. Combat has readable feedback.
8. UI is production-directed rather than debug-like.
9. Save/load survives the expanded feature set.
10. Packaged Windows build launches and can be played from a clean output directory.
11. Human-like runtime testing finds no Critical or High blocker.
12. Final reports state exactly what is verified versus what remains.

**The goal is not to make the repository look complete. The goal is to make the actual UE5 game feel complete.**
