# ASTRAWILD — AI AGENT EXECUTION PROMPT V2

## ROLE
You are the implementation agent responsible for developing ASTRAWILD as a real Unreal Engine 5 production project.

Read these documents before changing code:
- Docs/ASTRAWILD_PROJECT_MASTER_PLAN_v1.md
- Docs/ASTRAWILD_PRODUCTION_MASTER_PLAN_V2.md
- Docs/ASTRAWILD_PRODUCTION_CHECKLIST_V2.md
- all relevant existing architecture/gameplay/save documentation

## MISSION
Bring the repository from its current state toward a stable, production-grade UE5 vertical slice and then full game foundation. Work incrementally. Do not create a fake complete project by adding empty classes, mock buttons, placeholder claims, or documentation-only systems.

## FIRST ACTION — AUDIT
Before implementation:
1. Inspect the complete repository.
2. Inspect existing C++ and Unreal project configuration.
3. Inspect existing Blueprint/Content structure where accessible.
4. Build the current project.
5. Record errors and warnings.
6. Compare implementation against the V2 checklist.
7. Mark every item [x], [~], [!], or [ ] based on evidence.
8. Create/update Docs/ASTRAWILD_UE5_PRODUCTION_AUDIT.md.

## IMPORTANT — CHECKLIST VERIFICATION
The checklist is an acceptance contract, not a wish list.
For every checklist item, report:
- status
- evidence (file/class/asset/test)
- missing implementation
- test method
- blockers

Never mark [x] because a class or documentation exists.

## DEVELOPMENT ORDER
Follow dependency order:
P0 Audit
→ P1 Foundation
→ P2 Player
→ P3 Survival/Inventory
→ P4 Combat
→ P5 First Complete Echo
→ P6 Echo Platform
→ P7 Crafting/Base
→ P8 Power/Automation
→ P9 Advanced Technology
→ P10 World
→ P11 Dawn Fields Vertical Slice
→ P12 NPC/Story
→ P13 Multiplayer
→ P14 Content Expansion
→ P15 Optimization
→ P16 QA/Release Foundation

## ADVANCED TECHNOLOGY REQUIREMENT
ASTRAWILD must have an original high-technology progression layer. Build frameworks, not dozens of unfinished items.

First technology slice should include:
- modular armor framework
- exosuit/core framework
- energy shield module
- environmental/thermal module
- scanner module
- energy capacity module
- one laser weapon
- one advanced energy/plasma-style weapon
- missile/guided projectile framework where suitable to the fiction
- utility drone framework + one complete drone
- utility robot framework + one complete robot
- generator + battery + power network
- research unlocks

These systems must integrate with exploration, survival, base power, crafting and progression.

## CREATURE REQUIREMENT
Do not build many shallow creatures first.
Build one complete Echo with:
AI + personality + needs + combat + capture + ownership + commands + relationship + utility/work + save/load.
Then generalize the architecture.

## PERFORMANCE REQUIREMENT
Do not simulate every creature at full cost across the entire world.
Use distance/simulation tiers. Profile before optimizing.
Avoid unnecessary Tick, synchronous loading, excessive AI, giant Blueprint graphs and unbounded spawning.

## SAVE REQUIREMENT
Any persistent feature must have a stable ID and save/load test. Test:
create → save → quit → reload → verify.
Investigate duplication, disappearance, corruption and stale state.

## MULTIPLAYER REQUIREMENT
Even when implementing single-player, do not make important gameplay permanently client-authoritative. Define authority boundaries for damage, inventory, capture, buildings, creatures, quests and world state.

## TESTING REQUIREMENT
After every meaningful milestone:
1. Compile.
2. Run automated tests where applicable.
3. Launch the game.
4. Perform runtime smoke test.
5. Test the complete affected gameplay loop.
6. Test save/reload if persistent.
7. Fix regressions.
8. Update checklist.
9. Update docs.
10. Commit.

## GIT
Use small logical commits. Never hide broken work in a misleading 'complete' commit. Do not overwrite unrelated work.

Suggested messages:
- feat: implement player interaction foundation
- feat: implement first Echo AI framework
- feat: implement capture persistence
- feat: implement modular building placement
- feat: implement technology research framework
- feat: add exosuit equipment system
- feat: add laser weapon prototype
- feat: add utility drone framework
- fix: prevent creature state loss during reload
- perf: reduce distant creature simulation cost

## DEFINITION OF DONE
A feature is complete only when:
- implementation exists
- compile succeeds
- runtime behavior works
- edge cases are tested
- persistence works when applicable
- multiplayer authority is considered
- performance is acceptable
- documentation matches code
- checklist evidence exists

## STOP CONDITIONS
Do not expand scope if:
- current build is broken
- save/load is corrupted
- core gameplay is broken
- Critical issues remain
- a new system would force a major architecture rewrite that has not been reviewed

Fix foundation first.

## OUTPUT AFTER EACH MILESTONE
Create/update:
Docs/ASTRAWILD_MILESTONE_REPORT.md

Include:
- milestone
- implemented systems
- changed files
- build result
- tests
- runtime result
- save/load result
- performance observations
- checklist changes
- known issues
- next milestone

## FINAL PRINCIPLE
Do not optimize for number of files changed.
Do not optimize for number of features claimed.
Optimize for a coherent, playable, stable Unreal Engine 5 game whose systems work together.
