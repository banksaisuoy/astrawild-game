# ASTRAWILD — GLM 5.3 UE5 IMPLEMENTATION TASKLIST V5

## Purpose
GLM 5.3 is the CODE/REPOSITORY agent. It does not need to run Unreal Engine locally. Its job is to implement, repair, document, and prepare the UE5 project so that Antigravity can later pull it onto the user's Windows PC, build/cook/package it with the locally installed Unreal Engine 5, launch it, and report real runtime failures.

## Golden Rule
Do not claim that a feature is runtime-verified unless Antigravity has actually built and run the UE5 project on the target PC.

Allowed GLM statuses:
- SOURCE_IMPLEMENTED — code/assets/config are prepared; engine runtime not verified.
- SOURCE_VERIFIED — static/code-level checks passed.
- ENGINE_VERIFIED — only after evidence from local UE5 build/runtime.
- BLOCKED — requires local UE5/tool/asset/runtime access.

## Roles
### GLM 5.3
- Read repository.
- Design/implement C++ and project configuration.
- Create/update data structures, assets that can be generated, tests, docs and checklists.
- Review code for compile/runtime risks.
- Keep architecture production-grade.
- Commit changes to GitHub.
- Never pretend to have run UE5 if it cannot access UE5.

### Antigravity
- Pull/clone repository to Windows PC.
- Detect installed UE5 version.
- Generate project files.
- Build Editor/Game.
- Open/run UE5.
- Cook/package.
- Launch packaged build.
- Perform smoke/regression tests.
- Capture logs/errors/screenshots where useful.
- Report failures back into GitHub.
- Do not redesign gameplay unless explicitly instructed.

## WORKFLOW
GLM implementation → GitHub → Antigravity local UE5 verification → GitHub QA report → GLM fixes → repeat.

Never skip the local-engine verification loop.

---

# PHASE 0 — REPOSITORY AUDIT

- [ ] Read all Docs/ASTRAWILD*.md
- [ ] Inspect Source tree
- [ ] Inspect Config
- [ ] Inspect Content structure
- [ ] Identify .uproject and UE version assumptions
- [ ] Identify plugins
- [ ] Identify missing generated/config files
- [ ] Identify C++ compile risks
- [ ] Identify placeholder/mock systems
- [ ] Identify hardcoded gameplay
- [ ] Produce/update `Docs/ASTRAWILD_GL53_SOURCE_AUDIT.md`
- [ ] Produce/update `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md`

Do not rewrite working systems merely for style.

# PHASE 1 — UE5 FOUNDATION

- [ ] Confirm target UE5 version in project documentation
- [ ] Validate module names and Build.cs
- [ ] Validate Target.cs files
- [ ] Validate plugin dependencies
- [ ] Validate includes/API usage
- [ ] Validate reflection macros
- [ ] Validate UObject lifetime/ownership
- [ ] Validate subsystem architecture
- [ ] Validate Gameplay Tags
- [ ] Validate Data Assets/Data Tables
- [ ] Validate stable IDs
- [ ] Validate logging
- [ ] Validate automation-test structure
- [ ] Remove accidental web-game remnants from gameplay architecture
- [ ] Ensure UE5 is the only gameplay runtime target

# PHASE 2 — PLAYER CORE

Implement/repair:
- [ ] Character
- [ ] Camera
- [ ] Enhanced Input
- [ ] Movement
- [ ] Sprint
- [ ] Jump
- [ ] Dodge
- [ ] Interaction
- [ ] Combat input
- [ ] Death
- [ ] Respawn
- [ ] Input restoration after respawn
- [ ] Controller/gamepad mapping
- [ ] Input remapping architecture

Acceptance: code paths are coherent and testable. Runtime verification remains Antigravity's job.

# PHASE 3 — SURVIVAL

- [ ] Health
- [ ] Stamina
- [ ] Hunger
- [ ] Thirst
- [ ] Temperature
- [ ] Status effects
- [ ] Food
- [ ] Medicine
- [ ] Damage/death rules
- [ ] Respawn state reset
- [ ] Save/load state

# PHASE 4 — INVENTORY/EQUIPMENT

- [ ] Item Definition
- [ ] Item Instance
- [ ] Stack handling
- [ ] Weight
- [ ] Equipment slots
- [ ] Pickup/drop
- [ ] Transfer
- [ ] Consume/use
- [ ] Duplication protection
- [ ] Stable persistence IDs
- [ ] Save/load

# PHASE 5 — FIRST COMPLETE ECHO

Build one reference Echo to production quality before multiplying content:
- [ ] Definition/data asset
- [ ] Instance state
- [ ] Stats
- [ ] AI controller
- [ ] Navigation
- [ ] Perception
- [ ] StateTree/Behavior architecture as appropriate
- [ ] Idle/wander
- [ ] Follow
- [ ] Stay
- [ ] Defend
- [ ] Combat
- [ ] Hit reaction
- [ ] Death
- [ ] Capture
- [ ] Ownership
- [ ] Party membership
- [ ] Commands
- [ ] Bond/relationship
- [ ] Work assignment
- [ ] Work-site navigation
- [ ] Production
- [ ] Save/load
- [ ] Stable identity

# PHASE 6 — CAPTURE / CREATURE PLATFORM

After the reference Echo works architecturally:
- [ ] Data-driven species definitions
- [ ] Traits
- [ ] Abilities
- [ ] Habitat
- [ ] Diet
- [ ] Time behavior
- [ ] Weather response
- [ ] Work roles
- [ ] Capture item/tool framework
- [ ] Ownership framework
- [ ] Creature persistence
- [ ] Simulation tiers

Do not create dozens of shallow Echoes before this platform is stable.

# PHASE 7 — CRAFTING

- [ ] Recipe definitions
- [ ] Ingredient validation
- [ ] Crafting stations
- [ ] Technology requirements
- [ ] Crafting queue where appropriate
- [ ] Output validation
- [ ] Inventory integration
- [ ] Save/load

# PHASE 8 — BUILDING

- [ ] Foundation
- [ ] Floor
- [ ] Wall
- [ ] Roof
- [ ] Door
- [ ] Storage
- [ ] Workstation
- [ ] Placement preview
- [ ] Rotation
- [ ] Snap
- [ ] Collision validation
- [ ] Ownership
- [ ] Delete/dismantle
- [ ] Refund rules
- [ ] Repair
- [ ] Save/load
- [ ] Building stable IDs

# PHASE 9 — POWER + AUTOMATION

- [ ] Generator
- [ ] Battery
- [ ] Power producer/consumer interface
- [ ] Network/connection model
- [ ] Priority
- [ ] Power failure
- [ ] Energy storage
- [ ] Persistence
- [ ] Echo work-site integration
- [ ] Production loop
- [ ] Automation loop

Required gameplay chain:
Resource → Build workstation → Power workstation → Assign Echo → Echo performs work → Output generated → Player collects output.

# PHASE 10 — RESEARCH + TECHNOLOGY

Build extensible technology architecture.

Minimum playable advanced technology:
- [ ] Research tree
- [ ] Technology prerequisites
- [ ] Technology unlock persistence
- [ ] Modular armor
- [ ] Helmet module
- [ ] Chest/core module
- [ ] Energy capacitor
- [ ] Scanner
- [ ] Thermal/environment module
- [ ] Shield
- [ ] Exosuit framework
- [ ] Laser weapon framework + one complete weapon
- [ ] Advanced energy/plasma weapon OR guided projectile weapon
- [ ] Weapon energy/heat model where appropriate
- [ ] Weapon mod framework
- [ ] Utility drone framework + one complete drone
- [ ] Utility robot framework + one complete robot

Do not hardcode each technology item into unrelated systems.

# PHASE 11 — WORLD / THE SHATTERED VALE

> Batch 7 (`7fef4fe`) already shipped the runtime world framework — see
> `Docs/ASTRAWILD_ZONE_WORLD.md` before doing anything here. Verify/compile first;
> the remaining boxes are the editor-tier upgrades.

- [ ] World Partition compatibility (runtime world is PMC tiles — WP is the editor upgrade; optional .r16 landscape imports ready in `Content/Heightmaps/`)
- [ ] Data Layers where needed
- [ ] PCG where appropriate
- [x] Day/night (TimeSubsystem + sun tracking — verify at playtest)
- [x] Weather (WeatherSubsystem, global states replicate — verify at playtest)
- [x] Biome framework (Batch 7: `UAstrawildZoneSubsystem` — 6 zones, weight field, events, discovery; compile pending)
- [x] Resource spawning (per-zone signature tables + camp ring — compile pending)
- [x] Creature habitat/population (per-zone species placement, EcosystemSubsystem LOD sweep — compile pending)
- [x] Landmarks (per-zone silhouettes + ~20 tinted lights, 8 animated — compile pending)
- [ ] Technology-gated exploration
- [ ] Scanner discoveries
- [ ] Hidden location

# PHASE 12 — DUNGEON / BOSS

- [ ] Dungeon entry
- [ ] Encounter framework
- [ ] Progression gates
- [ ] Loot/reward
- [ ] Boss arena
- [ ] Boss phases
- [ ] Telegraphs
- [ ] Weak points
- [ ] Environmental mechanics
- [ ] Defeat state
- [ ] Technology reward
- [ ] Save/load state

# PHASE 13 — QUEST / STORY

- [ ] Quest definitions
- [ ] Objective/event system
- [ ] Completion conditions
- [ ] Rewards
- [ ] Main vertical-slice chain
- [ ] Side quest framework
- [ ] NPC interaction foundation
- [ ] Dialogue foundation
- [ ] World state

# PHASE 14 — SAVE/LOAD CERTIFICATION

Every persistent system must have stable IDs and explicit serialization.

Test cases to prepare for Antigravity:
- [ ] Player
- [ ] Vitals
- [ ] Inventory
- [ ] Equipment
- [ ] Echo
- [ ] Party
- [ ] Echo work assignment
- [ ] Buildings
- [ ] Building health
- [ ] Power
- [ ] Battery
- [ ] Research
- [ ] Quest
- [ ] Knowledge/scan data
- [ ] Dungeon progress
- [ ] Advanced technology

Test sequence:
CREATE → SAVE → QUIT → LOAD → VERIFY.
Repeat at least 3 cycles.

# PHASE 15 — QUALITY / PERFORMANCE

GLM source-level checks:
- [ ] Tick audit
- [ ] UObject ownership audit
- [ ] Delegate lifetime audit
- [ ] Async/thread safety audit
- [ ] Save serialization audit
- [ ] Replication/authority audit
- [ ] Null/error handling
- [ ] Logging
- [ ] Memory lifetime
- [ ] Asset reference audit
- [ ] Hardcoded path audit
- [ ] Config audit

Antigravity runtime checks:
- [ ] CPU
- [ ] GPU
- [ ] RAM
- [ ] VRAM
- [ ] AI
- [ ] Navigation
- [ ] Streaming
- [ ] Save/load
- [ ] Frame-time stability

# PHASE 16 — PLAYABLE BUILD GATE

Do not call this complete until Antigravity confirms:

- [ ] Project opens in UE5
- [ ] Editor build succeeds
- [ ] Game build succeeds
- [ ] Cook succeeds
- [ ] Package succeeds
- [ ] Packaged game launches
- [ ] New Game works
- [ ] Player works
- [ ] Echo works
- [ ] Capture works
- [ ] Inventory works
- [ ] Building works
- [ ] Power works
- [ ] Automation works
- [ ] Research works
- [ ] Advanced technology works
- [ ] Dungeon works
- [ ] Boss works
- [ ] Quest chain works
- [ ] Save works
- [ ] Quit/Continue works
- [ ] Controller works
- [ ] No blocker crash

# DEFINITION OF DONE

A feature is DONE only when:
1. source implementation exists;
2. dependencies are valid;
3. code/static checks pass;
4. documentation is updated;
5. Antigravity has verified runtime behavior when runtime verification is required;
6. regression tests pass;
7. persistent state is tested if applicable;
8. no known Critical/High blocker remains.

# GIT RULES

Commit small logical changes.
Do not bundle unrelated features.
Do not fake verification.
Do not erase the user's existing work.
Never force-push or rewrite history unless explicitly requested.

# REQUIRED REPORTS

After every implementation batch update:
- `Docs/ASTRAWILD_GL53_SOURCE_AUDIT.md`
- `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md`
- `Docs/ASTRAWILD_PRODUCTION_CHECKLIST_V2.md`
- `Docs/ASTRAWILD_MILESTONE_REPORT.md`

When Antigravity reports a runtime failure, create/update:
- `Docs/ANTIGRAVITY_RUNTIME_FAILURES.md`

GLM must then fix the reported source/config/asset problem and commit the fix.

# CURRENT COMMAND

Start with PHASE 0 audit.
Then implement only the highest-priority source-side blockers needed for the first playable vertical slice.
Do not add large amounts of new content.
Do not claim UE5 runtime success.
Prepare the repository for Antigravity to build and verify locally.
