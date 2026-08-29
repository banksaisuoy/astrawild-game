# ASTRAWILD — Build Status

## Status

- Overall: `PARTIAL` — full vertical-slice foundation implemented in C++ (**source-complete, never compiled**)
- Last updated: 2026-08-29
- Branch: `main` (latest code commit `7775668`; docs suite added this round, uncommitted)
- Latest change: V2 foundation round — world simulation, survival, combat, Echo v2 (needs/personality/AI), capture pipeline, journal, research, quests, crafting, building, power grid, save schema v2, zero-asset world bootstrap, C++ HUD, cheats, 8 automation tests, 23-file docs suite

## Environment

- Unreal Engine: **Not run in sandbox environments; target is 5.8** (EngineAssociation in `ASTRAWILD.uproject`)
- Compiler: Not run in sandbox environments (Linux sandbox has no MSVC/UE toolchain); target build is Windows + VS2022
- OS: Repository validation run in Ubuntu sandbox; target build is Windows
- CPU/GPU/RAM/Storage: Not measured

## Compile

- Target: `ASTRAWILDEditor Win64 Development` — pending Antigravity (user machine)
- Result: `NOT_RUN`
- Errors: Not measured; Unreal Editor unavailable in sandbox environments
- Warnings: Not measured
- Build duration: Not measured
- Validation steps for the target machine: `Docs/ASTRAWILD_TEST_PLAN.md` §4

Static repository validation passed with `Scripts/validate_repository.sh`.

## Changes in this round (2026-08-29, DOCS-1 — docs suite)

The C++ for all systems below landed in commits `3872c7e`→`7775668`; this round adds the complete
documentation suite (23 new files in `Docs/`, see "New systems documented" table) and this status refresh.
No `Source/` files were modified by DOCS-1.

### New systems (implemented in C++ this foundation round, compile pending)

| System | Key classes | Doc |
|---|---|---|
| Logging (8 categories) | `AstrawildLog` | — |
| Native gameplay tags (77) | `AstrawildGameplayTags` | `ASTRAWILD_GAMEPLAY_TAGS.md` |
| Types v2 + 8 data-asset definition classes | `AstrawildTypes`, `AstrawildDataAssets` | `ASTRAWILD_UE5_ARCHITECTURE_V2.md` |
| Replicated world state | `AstrawildGameState` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Day/night (24-min day) | `AstrawildTimeSubsystem` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Weather (8 states, weighted) | `AstrawildWeatherSubsystem` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Event bus | `AstrawildEventBusSubsystem` | `ASTRAWILD_UE5_ARCHITECTURE_V2.md` |
| Ecosystem LOD + population | `AstrawildEcosystemSubsystem` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Procedural Dawn Fields (zero-asset arena) | `AstrawildWorldBootstrapper` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Survival vitals + status effects + respawn | `AstrawildSurvivalComponent` | `ASTRAWILD_SURVIVAL_SYSTEM.md` |
| Action combat (light/heavy/dodge/block/elemental) | `AstrawildCombatComponent` | `ASTRAWILD_COMBAT_SYSTEM.md` |
| Echo v2 (needs/personality/growth/commands) | `AstrawildEchoCharacter` | `ASTRAWILD_CREATURE_SYSTEM.md` |
| Echo AI (perception + 16-state machine) | `AstrawildEchoAIController` | `ASTRAWILD_AI_ARCHITECTURE.md` |
| Capture pipeline + field journal | `AstrawildCaptureComponent`, `AstrawildJournalSubsystem` | `ASTRAWILD_CREATURE_SYSTEM.md` |
| Echo roster/party (max 3) | `AstrawildEchoRosterSubsystem` | `ASTRAWILD_CREATURE_SYSTEM.md` |
| Echo work sites | `AstrawildWorkSiteActor` | `ASTRAWILD_CREATURE_SYSTEM.md` |
| Inventory v2 (weight 120 kg, equipment slot) | `AstrawildInventoryComponent` | `ASTRAWILD_GAMEPLAY_SYSTEMS.md` |
| Item registry + CODE_DEFAULT content library | `AstrawildItemRegistrySubsystem`, `AstrawildContentLibrary` | `ASTRAWILD_ASSET_PIPELINE.md` |
| Timed crafting + stations | `AstrawildCraftingComponent`, `AstrawildCraftingStationActor` | `ASTRAWILD_CRAFTING_SYSTEM.md` |
| Building placement + actors + power grid | `AstrawildBuildingComponent`, `AstrawildBuildingActor`, `AstrawildPowerSubsystem` | `ASTRAWILD_BUILDING_SYSTEM.md` |
| Research / tech tree | `AstrawildResearchSubsystem` | `ASTRAWILD_RESEARCH_SYSTEM.md` |
| Quests (event-driven, 5-quest chain) | `AstrawildQuestComponent` | `ASTRAWILD_QUEST_SYSTEM.md` |
| Save schema v2 (checksum, migration, autosave) | `AstrawildSaveSubsystem` | `ASTRAWILD_SAVE_SYSTEM.md` |
| Pure-C++ HUD + runtime Enhanced Input | `AstrawildHudWidget`, `AstrawildPlayerCharacter` | `ASTRAWILD_UI_ARCHITECTURE.md` |
| Cheat manager (12 commands) | `AstrawildCheatManager` | `ASTRAWILD_INPUT_REFERENCE.md` |
| NPC base (architecture-ready) | `AstrawildNPCCharacter` | `ASTRAWILD_QUEST_SYSTEM.md` |
| Automation tests (8) | `AstrawildAutomationTests.cpp` | `ASTRAWILD_TEST_PLAN.md` |
| Multiplayer authority/replication (20 props, 5 RPCs) | across classes | `ASTRAWILD_MULTIPLAYER.md` |

### Docs created this round (DOCS-1)

`ASTRAWILD_UE5_ARCHITECTURE_V2` · `ASTRAWILD_GAMEPLAY_SYSTEMS` · `ASTRAWILD_CREATURE_SYSTEM` ·
`ASTRAWILD_AI_ARCHITECTURE` · `ASTRAWILD_COMBAT_SYSTEM` · `ASTRAWILD_SURVIVAL_SYSTEM` ·
`ASTRAWILD_BUILDING_SYSTEM` · `ASTRAWILD_CRAFTING_SYSTEM` · `ASTRAWILD_RESEARCH_SYSTEM` ·
`ASTRAWILD_WORLD_SYSTEM` · `ASTRAWILD_QUEST_SYSTEM` · `ASTRAWILD_SAVE_SYSTEM` · `ASTRAWILD_MULTIPLAYER` ·
`ASTRAWILD_UI_ARCHITECTURE` · `ASTRAWILD_GAMEPLAY_TAGS` · `ASTRAWILD_PERFORMANCE` ·
`ASTRAWILD_TEST_PLAN` · `ASTRAWILD_ASSET_PIPELINE` · `ASTRAWILD_PRODUCTION_ROADMAP_V2` ·
`ASTRAWILD_DEFINITION_OF_DONE` · `ASTRAWILD_ASSUMPTIONS` · `ASTRAWILD_ASSET_MANIFEST` ·
`ASTRAWILD_INPUT_REFERENCE`

## Unreal assets created by Antigravity

| Asset | Path | Status | Notes |
|---|---|---|---|
| Prototype map | — | NOT_CREATED | Not required for PIE: `AstrawildWorldBootstrapper` builds a playable zero-asset arena |
| Input assets (IMC/IA) | — | NOT_CREATED | Optional: runtime input defaults are built in code |
| Echo/Item/Building data assets | — | NOT_CREATED | CODE_DEFAULT content registered in memory; replacement plan in `ASTRAWILD_ASSET_PIPELINE.md` |
| UI | — | NOT_CREATED | Pure-C++ HUD requires none |
| Echo meshes / icons | — | NOT_CREATED | Engine basic-shape placeholders; checklist in `ASTRAWILD_ASSET_MANIFEST.md` |

## Playtest

| Test | Result | Notes |
|---|---|---|
| Open project | NOT_RUN | Awaiting target-machine compile (Test Plan §4) |
| Compile Development Editor | NOT_RUN | **Blocking step for everything below** |
| Automation suite (8 tests) | NOT_RUN | Run via Session Frontend, filter `ASTRAWILD` |
| Player movement/camera | NOT_RUN | Manual flow step 4 |
| Interaction | NOT_RUN | Step 5 |
| Harvest resource | NOT_RUN | Step 10 |
| Capture Echo | NOT_RUN | Step 8 |
| Craft recipe | NOT_RUN | Step 11 (station interact) |
| Place building | NOT_RUN | Step 12 |
| Activate rest point | NOT_RUN | Step 17 area |
| Save snapshot (F5) | NOT_RUN | Steps 14–16 |
| Load snapshot (F9) | NOT_RUN | Step 16 |
| Full first-playable flow (17 steps) | NOT_RUN | `ASTRAWILD_TEST_PLAN.md` §2 |

## Known issues

| Severity | Issue | File/asset | Reproduction | Owner/next action |
|---|---|---|---|---|
| **Blocker** | Repository has never been compiled | `Source/AstrawildCore/` | Any build attempt | Antigravity: Test Plan §4, then fix-forward |
| High | Quest "Observe a Lumewisp" objective cannot progress (no event publisher for ObserveEcho) | `AstrawildQuestComponent.cpp` | Start Quest_FirstEcho and observe a Lumewisp | Add observation event publish at journal milestones (Test Plan T-1) |
| High | AI think loop reschedules per frame; LOD interval computed but not applied | `AstrawildEchoAIController.cpp:Think` | Spawn 10+ Echoes, profile | Replace `SetTimerForNextTick` with `SetTimer(Interval)` (T-2) |
| Medium | Cold/heat damage unreachable with default weather profile (min felt temp 8 °C vs 4 °C threshold) | `AstrawildWeatherSubsystem` / Survival | `AW.SetWeather cold` and wait | Tune profile/base temp after design review (T-4) |
| Medium | Player consume food/drink action has no keybind | `AstrawildPlayerCharacter.cpp` | Have berries, press keys | Wire with inventory UI (M8+) (T-5) |
| Medium | Journal subsystem iterates all Echoes every frame per player | `AstrawildJournalSubsystem.cpp:Tick` | Insights capture | Optimize on profiling evidence (T-6) |
| Low | HUD weather label hard-codes 20 °C | `AstrawildHudWidget.cpp` | Look at HUD | Cosmetic fix (T-3) |
| Low | Log-line count drift ("16 actions" / "10 items" vs actual 15/12) | PlayerCharacter.cpp / ContentLibrary.cpp | Read logs | Cosmetic; correct the strings when touched |

## Handoff to Antigravity

The C++ core (single module `AstrawildCore`, ~11.4k LOC, 78 source files), the zero-asset playability layer
(procedural world + runtime input + C++ HUD), save schema v2, the CODE_DEFAULT content set, the
documentation suite (23 system docs), the test plan, and the asset manifest/replacement pipeline are all in
the repository. Antigravity must: **pull, generate project files, compile `ASTRAWILDEditor Win64
Development`, run the 8 automation tests, execute the 17-step first-playable checklist, and fill this report
with real results.** Do not mark `COMPLETE` until Compile, the automation suite, the core-loop Playtest,
and Save/Load have all passed (see `Docs/ASTRAWILD_DEFINITION_OF_DONE.md`).
