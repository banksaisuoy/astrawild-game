# ASTRAWILD — GLM 5.3 FINAL BUILD PROMPT V4

You are the **Lead Unreal Engine 5 Gameplay Engineer + Technical Director + Release Engineer** for ASTRAWILD.

Your goal is NOT to keep expanding the roadmap.

Your goal now is:

> **Finish a genuinely playable ASTRAWILD UE5 vertical slice so another agent only needs to build/package/deploy it.**

## SOURCE OF TRUTH

Read first:
1. `Docs/ASTRAWILD_PROJECT_MASTER_PLAN_v1.md`
2. `Docs/ASTRAWILD_PRODUCTION_MASTER_PLAN_V2.md`
3. `Docs/ASTRAWILD_PRODUCTION_CHECKLIST_V2.md`
4. `Docs/ASTRAWILD_ULTIMATE_PRODUCTION_ROADMAP_V3.md`
5. `Docs/ASTRAWILD_PLAYABLE_BUILD_MASTER_PLAN_V4.md`
6. `Docs/ASTRAWILD_ULTIMATE_GAP_ANALYSIS.md`
7. `Docs/ASTRAWILD_IMPLEMENTATION_GAP_REPORT.md`
8. `Docs/ASTRAWILD_UE5_PRODUCTION_AUDIT.md`

V4 is the immediate priority. V3 is long-term scope.

## CURRENT STATUS YOU MUST ACCEPT

The repository contains substantial gameplay/source implementation, but the latest audit explicitly says the project has never been engine-compiled or runtime-verified in the available environment. The checklist therefore must not be treated as proof of a working build.

Your first job is to turn source completeness into an engine-buildable, playable project.

## ABSOLUTE RULES

- UE5 only.
- No web version.
- No HTML game.
- No React game.
- No replacement engine.
- No fake screenshots.
- No mock gameplay classes used to claim completion.
- No deleting systems merely to make compilation pass.
- No commenting out broken gameplay as a workaround.
- No hardcoded one-off systems when a reusable architecture is required.
- No giant content expansion before the core loop works.
- Do not copy Palworld or ARK assets, characters, creatures, UI, names, audio, animations or proprietary presentation.

## NEW HANDOFF MODEL

GLM 5.3 = IMPLEMENTATION / FIX / INTEGRATION.

Antigravity = BUILD / PACKAGE / DEPLOY / RUNTIME VERIFICATION ONLY.

Do NOT rely on Antigravity to finish gameplay architecture.

## STEP 1 — AUDIT CURRENT REPOSITORY

Before editing:
- inspect Git state
- inspect `.uproject`
- inspect Source
- inspect Config
- inspect Plugins
- inspect Content structure
- inspect current docs
- inspect latest commits
- inspect known gaps

Then produce/update:
`Docs/ASTRAWILD_FINAL_HANDOFF_GAP_REPORT.md`

For every P0/P1/P2 and V4 playable item state:
- implementation evidence
- dependency
- remaining work
- acceptance test
- priority

## STEP 2 — BUILD BLOCKERS FIRST

Fix all known compile blockers and latent compile issues.

Priority from current gap analysis includes:
- actual UE5 compilation
- navigation/runtime AI
- respawn input
- reachable research progression
- save/load round-trip
- building UX
- Echo work automation
- dungeon/boss runtime integration
- grid-level battery persistence
- Echo population unregister/decrement
- quest objective integrity
- crafting output validation
- gamepad support

Do not mark these complete without evidence.

## STEP 3 — BUILD THE PLAYABLE LOOP

The game must support this exact no-cheat loop:

START
→ explore Dawn Fields
→ scan Echo
→ fight/avoid
→ capture Echo
→ inventory/equipment
→ gather resources
→ construct small base
→ generate/store power
→ power workstation
→ assign Echo to work
→ collect produced resource
→ research technology
→ craft advanced equipment
→ enter dungeon
→ defeat boss
→ receive advanced technology reward
→ return to base
→ save
→ quit
→ continue
→ verify persistent state.

If any step cannot be performed naturally, fix it before adding large new content.

## STEP 4 — PLAYER CERTIFICATION

Verify:
- third-person movement
- camera
- sprint
- stamina drain
- jump
- dodge
- interaction
- melee
- ranged/energy weapon
- death
- respawn
- input after respawn
- inventory
- equipment
- controller support

## STEP 5 — FIRST COMPLETE ECHO

Pick one existing Echo as the reference implementation.

It must support:
- spawn
- navmesh movement
- perception
- idle/wander
- combat
- damage
- death
- capture
- ownership
- follow
- stay
- defend
- attack
- basic work role
- trust/bond
- stable ID
- save/load

Then ensure the remaining 3–5 slice Echoes can reuse the platform.

## STEP 6 — SURVIVAL / INVENTORY

Verify:
- health
- stamina
- hunger
- thirst
- temperature
- status effect API and at least one real effect
- food
- medicine
- item stacking
- weight
- equipment
- pickup/drop
- persistence

Test duplication explicitly.

## STEP 7 — BASE / POWER / AUTOMATION

Finish:
- building piece selection
- placement
- snap
- collision validation
- delete/dismantle
- partial refund
- repair
- ownership foundation
- save/load
- generator
- battery
- power consumers
- stored energy persistence
- power failure behavior

Then prove:

Captured Echo → Work Site → Powered Production → Output → Inventory.

## STEP 8 — RESEARCH / ADVANCED TECHNOLOGY

Do not build a dozen disconnected futuristic weapons.

Build reusable frameworks first.

Required playable slice:

### Modular equipment
- equipment slot framework
- chest/core module
- scanner module
- environmental/thermal module
- energy capacitor
- shield module

### Advanced weapons
- one laser weapon
- one advanced energy/plasma OR guided weapon
- unified damage/resource/heat model

### Robotics
- utility drone framework + one complete scout/scan drone
- utility robot framework + one complete worker/defense robot

Technology must be earned through research and integrated with exploration/base progression.

## STEP 9 — EXPLORATION

Dawn Fields must contain:
- starting region
- resource nodes
- Echo habitats
- environmental hazard
- landmark
- secret/hidden location
- technology-gated area
- dungeon entrance

Scanner must provide useful knowledge rather than a cosmetic overlay.

## STEP 10 — DUNGEON / BOSS

Finish one complete dungeon.

Required:
- entrance
- gates
- encounters
- rewards
- boss arena
- boss AI
- phases
- telegraphs
- weak point
- environmental mechanic if feasible
- defeat event
- unique technology reward
- save/load policy

## STEP 11 — QUEST

Finish the complete short story chain already defined in the repository.

Every quest objective must have a real producer/event and real completion condition.

No dead objective types in the active playable slice.

## STEP 12 — SAVE/LOAD CERTIFICATION

Run a comprehensive round-trip test covering:
- player health/vitals
- inventory
- equipment
- captured Echo
- Echo party state
- Echo assignment
- buildings
- building health
- power state
- battery stored energy
- research
- quests
- discovered knowledge
- dungeon state
- advanced equipment.

Test save/reload repeatedly.

## STEP 13 — QUALITY

Fix:
- progression dead ends
- duplication
- state loss
- stuck AI
- broken navigation
- impossible quest objectives
- UI feedback failures
- respawn failures
- input failures
- obvious crashes.

## STEP 14 — PERFORMANCE

Profile the playable slice.

Audit:
- Tick usage
- AI cost
- navigation
- spawning
- memory
- VRAM
- loading
- save/load
- rendering

Use evidence. Do not claim a performance target without measuring it.

## STEP 15 — RELEASE HANDOFF

Before handoff, update:
- `Docs/ASTRAWILD_PRODUCTION_CHECKLIST_V2.md`
- `Docs/ASTRAWILD_FINAL_HANDOFF_GAP_REPORT.md`
- `Docs/BUILD_STATUS.md`
- `Docs/ASTRAWILD_MILESTONE_REPORT.md`
- `Docs/ASTRAWILD_QA_REPORT.md`

Every item must be one of:
- VERIFIED
- SOURCE-COMPLETE / ENGINE-UNVERIFIED
- BLOCKED
- NOT IN PLAYABLE SCOPE

Never use `[x]` without verification evidence.

## FINAL HANDOFF CRITERIA

Do not hand off until the source is internally consistent and all known Critical gameplay blockers are fixed.

The handoff must include:
- exact UE5 version
- build steps
- package steps
- runtime launch steps
- test map
- default game mode
- known issues
- controls
- save location/policy
- debug commands, if any
- commit SHA

## IMPORTANT

Do not spend the remaining time creating:
- more biomes
- dozens of Echoes
- breeding
- vehicles
- full factions
- full multiplayer
- endgame

until the playable vertical slice is stable.

The next milestone is not "more features".

It is:

> **PLAYABLE + SAVEABLE + REPEATABLE + BUILDABLE + HANDOFF-READY.**

Start now with repository audit and gap report, then execute the highest-priority fixes in dependency order. Commit every coherent batch and keep the documentation synchronized.
