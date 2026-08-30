# ASTRAWILD — PLAYABLE BUILD MASTER PLAN V4

## Mission

Convert the current code-complete-but-unverified UE5 repository into a **playable, self-contained production vertical slice** that Antigravity can clone, open/build/package and launch without implementing gameplay systems itself.

This is the immediate execution plan. V3 remains the long-term roadmap; V4 is the release-to-play target.

## Hard Rule

**GLM 5.3 builds and fixes the game. Antigravity only verifies, packages, deploys/builds and reports.**

Antigravity must NOT invent gameplay, rewrite architecture, add placeholder systems, or silently fix source code. If something fails, it reports the exact failure back to GLM.

## Current Reality — 2026-08-30

Repository audit reports that the architecture and many gameplay paths exist, but the project has **never been compiled or runtime-tested in the available environment**. The checklist therefore correctly treats build/runtime claims as unverified. Critical source-level issues found during audit were patched, but engine verification is still required. fileciteturn8file0

The current gap analysis identifies the first hard blockers as:
1. real UE5 compilation/build
2. navigation/runtime AI
3. death/respawn input
4. reachable research progression
5. save/load round-trip completeness
6. building UX
7. Echo work/automation path
8. dungeon/boss runtime validation
9. remaining power persistence and population edge cases
10. crafting validation
11. controller support
12. multiplayer authority later, not before the single-player slice is stable

The first playable release must NOT attempt the full V3 scope. It must finish a coherent vertical slice.

# PLAYABLE TARGET — ASTRAWILD: DAWN PROTOCOL

The player must be able to start a new game and complete this loop without cheats:

Start → explore → scan Echo → fight/avoid → capture one Echo → manage inventory → gather resources → build a small base → power a workstation → assign Echo → produce resource → research → craft equipment → enter dungeon → fight boss → receive advanced-tech reward → return to base → save → quit → reload → continue.

If this loop works reliably, the build is considered a playable foundation.

# PHASE 0 — BUILD FIRST

## 0.1 Engine
- Confirm exact Unreal Engine version used by `.uproject`.
- Do NOT silently migrate engine versions.
- Generate project files.
- Compile Development Editor.
- Compile Development Game.
- Package a Development/Shipping test build as appropriate.

## 0.2 Fix all compile blockers
- Fix root causes.
- Never delete systems to make compilation pass.
- Never comment out gameplay as a workaround.
- Never hide warnings/errors.

## 0.3 Establish build evidence
Create/update:
`Docs/BUILD_STATUS.md`

Record:
- engine version
- compiler/toolchain
- commit
- command
- result
- errors
- warnings
- package result
- executable/package path

# PHASE 1 — FOUNDATION LOCK

Before adding content, verify:
- GameMode
- GameState
- PlayerController
- PlayerCharacter
- Enhanced Input
- Save subsystem
- Inventory subsystem
- Research subsystem
- Power subsystem
- Quest subsystem
- Echo roster/subsystem
- Building subsystem
- World bootstrapper
- logging
- stable IDs

Add/verify automated smoke tests for pure logic where practical.

# PHASE 2 — CORE PLAYER

Must work in runtime:
- movement
- camera
- sprint
- jump
- dodge
- interact
- attack
- death
- respawn
- input after respawn
- inventory
- equipment

Add gamepad support before final playable build.

# PHASE 3 — FIRST COMPLETE ECHO

Use one flagship Echo as the reference implementation.

Required:
- spawn
- navigation
- perception
- idle
- wander
- hostile behavior
- combat
- damage
- death
- capture
- ownership
- follow
- stay
- defend
- attack
- basic utility/work role
- trust/bond
- stable instance ID
- save/load

Do not add dozens of creatures until this pipeline is reliable.

# PHASE 4 — SURVIVAL + INVENTORY

Verify:
- health
- stamina
- hunger
- thirst
- temperature
- status effect framework
- food
- medicine
- inventory stack/weight
- pickup/drop
- equipment
- persistence

Fix sprint stamina drain and movement-state refresh.

# PHASE 5 — COMBAT FEEL

Minimum playable combat:
- melee
- ranged or first energy weapon
- hit reaction
- stagger
- damage feedback
- death
- elemental/status hook
- creature-assisted combat
- capture feedback

Do not overbuild weapon variety yet.

# PHASE 6 — BASE + AUTOMATION

Minimum base:
- foundation
- floor
- wall
- roof/door as appropriate
- storage
- workstation
- research desk
- power generator
- battery
- powered consumer
- placement preview
- snap
- collision validation
- piece selection
- delete/dismantle
- partial refund
- repair
- ownership foundation
- save/load

Automation loop:
Echo → Work Site → Production → Inventory.

The loop must be visible and useful to the player.

# PHASE 7 — RESEARCH + TECHNOLOGY

Make technology progression playable, not just data.

Required first tech branch:
Survival → Electrical → Energy → Ancient Technology.

Minimum advanced technology slice:
- modular armor framework
- chest/core equipment
- scanner module
- thermal/environment module
- energy capacitor
- shield module
- one laser weapon
- one advanced energy weapon OR guided projectile weapon
- one utility drone
- one utility robot

These must use reusable data-driven systems, not one-off hardcoded classes.

# PHASE 8 — EXPLORATION

Dawn Fields must contain:
- starting camp area
- resource locations
- Echo habitats
- environmental hazard
- landmark
- hidden area
- technology-gated area
- dungeon entrance
- return path

Scanner should reveal meaningful information:
- Echo species
- weakness
- habitat
- resource
- technology clue

# PHASE 9 — DUNGEON + BOSS

Dungeon must support:
- entrance
- progression gates
- encounters
- rewards
- boss arena
- boss phases
- weak point
- telegraph
- environmental hazard where feasible
- completion event
- unique technology reward
- save/load state policy

Boss difficulty must not rely on HP inflation alone.

# PHASE 10 — QUEST / STORY SLICE

Finish one coherent short story arc:
1. First Light
2. Discover the Echo
3. Establish shelter
4. Research the anomaly
5. Enter the ancient facility
6. Defeat the guardian
7. Recover the technology
8. Return and unlock the next technology tier

All objectives must be achievable without cheats.

Retire or implement unused objective types rather than leaving dead APIs.

# PHASE 11 — SAVE / LOAD CERTIFICATION

Mandatory test:

Create state
→ damage player
→ capture Echo
→ assign Echo
→ build structures
→ damage building
→ power devices
→ research technology
→ acquire advanced weapon
→ enter dungeon
→ save
→ quit
→ reload
→ verify every state.

Also test:
- repeated save/load
- autosave
- missing/invalid data
- duplicate item attempts
- duplicate Echo attempts
- building persistence
- battery/grid stored energy
- dungeon state policy

No release candidate until round-trip passes.

# PHASE 12 — PERFORMANCE PASS

Before adding large content:
- profile CPU
- profile GPU
- profile memory/VRAM
- profile AI
- audit Tick
- audit spawning
- audit navigation
- audit save/load
- audit asset loading

Implement simulation tiers for distant creatures if needed.

Do not optimize blindly; attach measurements to every major optimization.

# PHASE 13 — UX / POLISH

Minimum:
- main menu
- new game
- continue
- pause
- settings
- readable HUD
- inventory
- equipment
- creature roster
- crafting
- research
- quest tracking
- map/compass
- feedback for capture/crafting/research
- controller support
- subtitles
- UI scaling

No placeholder debug controls in the final playable build.

# PHASE 14 — CONTENT QUALITY

Use a small number of high-quality assets.

Required for playable build:
- 3–5 Echo species
- 1 flagship companion fully functional
- several hostile encounters
- 1 dungeon
- 1 boss
- 1 base location
- 1 biome slice
- 1 technology progression
- 1 story arc

Content must be original. Do not copy Palworld/ARK characters, creatures, UI, names, assets, animations, audio, or proprietary presentation.

# PHASE 15 — RELEASE CANDIDATE GATE

All must be TRUE:

BUILD
- [ ] UE5 compile passes
- [ ] package passes
- [ ] no unresolved compile errors

STARTUP
- [ ] game launches
- [ ] main menu works
- [ ] new game works
- [ ] continue works

CORE LOOP
- [ ] explore
- [ ] scan
- [ ] combat
- [ ] capture
- [ ] inventory
- [ ] crafting
- [ ] base
- [ ] power
- [ ] Echo work
- [ ] research
- [ ] advanced tech
- [ ] dungeon
- [ ] boss
- [ ] reward

PERSISTENCE
- [ ] save
- [ ] quit
- [ ] reload
- [ ] state matches

QUALITY
- [ ] no game-breaking crash
- [ ] no item duplication
- [ ] no Echo duplication/loss
- [ ] no progression dead-end
- [ ] no respawn input failure
- [ ] controller works
- [ ] acceptable performance on defined target PC

# DEFINITION OF READY FOR ANTIGRAVITY

GLM must hand over only when:
1. source is committed
2. repository is internally consistent
3. checklist is updated with evidence
4. build instructions are documented
5. known issues are documented
6. all playable-slice acceptance criteria are met in code/tests where engine execution is unavailable
7. no known Critical blocker remains

Antigravity's job after handoff:

CLONE/PULL → INSTALL/CONFIGURE ENGINE → BUILD → COOK/PACKAGE → LAUNCH → SMOKE TEST → REPORT.

If Antigravity finds a failure, it does not fix gameplay. It creates a precise failure report for GLM.

# POST-SLICE ONLY

Do NOT block the playable build on:
- all 5 biomes
- breeding/genetics
- vehicles
- full factions
- full multiplayer
- 50+ Echo
- endgame
- procedural megaworld
- every advanced weapon

Those belong to V5+ after the core vertical slice is proven.
