# ASTRAWILD — IMPLEMENTATION GAP REPORT

**Date:** 2026-08-30
**Source of truth:** Production Master Plan V2 + Production Checklist V2 + Ultimate Roadmap V3
**Evidence base:** `ASTRAWILD_UE5_PRODUCTION_AUDIT.md` (same date, line-cited)
**Rule:** No system may be deleted to make a build pass. No mock implementations to flip checklist marks. No feature-complete claims from documentation alone.

---

## CRITICAL — blocks compilation or the vertical-slice loop entirely

| ID | Gap | Evidence | Impact | Fix direction |
|---|---|---|---|---|
| **C-1** | Crafting station interact path does not compile: `GetAllRecipes()` returns `TArray<UAstrawildRecipeDefinition*>` but is iterated as `TPair<FName, TObjectPtr<...>>` | `AstrawildCraftingStationActor.cpp:52` vs `AstrawildItemRegistrySubsystem.h:73` | **Build fails.** Nothing can compile until fixed. | Change loop to `for (UAstrawildRecipeDefinition* Recipe : Registry->GetAllRecipes())` |
| **C-2** | No legitimate path unlocks any technology: `TryUnlockTech` callers are CheatManager + quest `RewardTechId` only; no quest sets `RewardTechId`; no tech UI; `Tech_BasicCrafting` never auto-granted | `ResearchSubsystem.cpp:70`; `CheatManager.cpp:141`; `QuestComponent.cpp:285`; zero `RewardTechId` in `ContentLibrary.cpp` | **Tech tree is cosmetic.** Quests 4–6 halt; power buildings (Tech_Electrical) and husbandry (Tech_Husbandry) unreachable; advanced tech layer (P9) has no entry point. | Wire a real unlock path: auto-grant root tech on session start + research interaction at Research Desk building (or quest rewards). |
| **C-3** | No navigation mesh in the zero-asset world; bootstrapper spawns no `ANavMeshBoundsVolume` and no runtime navmesh generation exists | Zero `NavMesh` references in `Source/` (grep-verified) | **All creature locomotion fails at runtime** — `MoveToLocation/MoveToActor` requests fail; Explore/Flee/Combat/Follow cannot move. The game's creatures would stand still. | Spawn a navmesh bounds volume + `FNavigationSystem` build in `WorldBootstrapper` (runtime-generated Recast navmesh), or add `ACharacter` movement fallback. |
| **C-4** | Dungeon can never complete: entry room has no encounters; empty-encounter rooms early-out of the clear-check tick → `RoomsCleared` maxes at N−1 → `OnDungeonCompleted` unreachable | `DungeonGeneratorActor.cpp:38, 127-130`; `DungeonRoomActor.cpp:118`; `Generator.cpp:142-146` | Dungeon progression stall; completion event has no consumers anyway. | Auto-mark empty rooms cleared at generation; subscribe quest/loot consumers to `OnDungeonCompleted`. |
| **C-5** | Phased boss `AAstrawildEchoBossCharacter` is never spawned anywhere; dungeon boss room spawns a plain Gloomfang | Zero `SpawnActor<AAstrawildEchoBossCharacter>` in Source (grep); `DungeonRoomActor.cpp:91-92` | The 3-phase enrage/summon boss fight — a P11 checklist item — is dead code. | Spawn boss character from boss room template; route damage through player mitigation. |
| **C-6** | Build mode locked to one arbitrary building: `CycleBuildingDefinition()` has no caller; TMap iteration order decides the only placeable piece | `BuildingComponent.cpp:99-106`; no references elsewhere | Players cannot choose buildings → base building (P7) functionally capped at 1 piece type. | Bind cycling to an input (N already rotates; add e.g. mouse-wheel or repeated B) + HUD piece indicator. |
| **C-7** | Work-site automation is dead: `AssignWorker`/`CollectOutput` have zero callers; site is not interactable | `WorkSiteActor.cpp:125-147`; no callers (grep) | Captured Echoes cannot perform work roles → First Complete Echo "utility role" fails; automation loop (P8) unreachable; fiber/berry site economy dead. | Implement `IAstrawildInteractable` on work site: E = collect output; assign nearest idle captured Echo. |
| **C-8** | Respawn input death: pawn `BeginPlay` early-returns before IMC setup when controller is null during `RestartPlayer` spawn ordering | `PlayerCharacter.cpp:122-133`; `GameMode.cpp:82-105` | After dying once, the player likely loses all input → death = game over. | Move IMC add to `OnActorRestarted`/possess path or re-add in `HandleRespawn` via controller. |

## HIGH — breaks a promised system or creates serious defects

| ID | Gap | Evidence | Impact |
|---|---|---|---|
| **H-1** | Survival vitals saved but never restored — load calls `FullRestore()` | `SaveSubsystem.cpp:74` vs `213-216` | Hunger/thirst/health reset each session; contradicts persistence claims. |
| **H-2** | Captured party Echoes are not respawned on load; roster imported twice; v1 path drops XP | `SaveSubsystem.cpp:201-204, 231-245`; `EchoCharacter.cpp:538` | "Continue" doesn't actually restore your party into the world. |
| **H-3** | Autosave writes to `ASTRAWILD_Auto` but nothing ever loads it; no load-on-boot | `GameMode.cpp:107-119`; loaders use `ASTRAWILD_Main` only | Crash recovery / continue-game loop broken. |
| **H-4** | `WeatherSubsystem::GetProfile` returns `const&` bound to `FindRef` by-value temporary — dangling reference UB | `WeatherSubsystem.cpp:87` | Works "by luck" per-compiler; latent crash/corruption. |
| **H-5** | Building damage resets on load: `FromSaveData` sets health then `InitializeFromDefinition` overwrites to max | `BuildingActor.cpp:178-187` | Damaged bases always heal on reload (dupe-ish exploit: save before raid). |
| **H-6** | Building materials consumed client-side pre-RPC; server never deducts (only refunds on fail) | `BuildingComponent.cpp:237, 258-266` | MP item-duplication vector; single-player harmless. |
| **H-7** | Captured "Attack" command can target the owner (no owner exclusion in `ExecuteCombat` fallback) | `EchoAIController.cpp:357-363` | Player's own Echo hunts them down. |
| **H-8** | `OnAIStateChanged` never broadcasts (controller writes enum directly, bypassing `SetAIState`) | `EchoAIController.cpp:256-262` | Blueprint/UI consumers get nothing; documented delegate is dead. |
| **H-9** | Ecosystem population counters broken for runtime-spawned Echoes (register-before-init) and never decrement on unregister | `EcosystemSubsystem.cpp:180-199, 328-343` | Population model inert; no respawn; species depletion untracked. |
| **H-10** | Quest chain halts at quest 4 (`Quest_Spark` requires the blocked tech unlock) — also `ReachLocation`/`SurviveTime` objective types unimplemented | `ContentLibrary.cpp:460-465`; `QuestComponent.cpp:185-186` | Main quest slice incomplete even after C-2 fix (needs unlock path to exist first). |
| **H-11** | Crafting output validation missing: `AddItem` result ignored → items silently destroyed at weight cap (same for cancel refunds) | `CraftingComponent.cpp:286-289, 164-195` | Player loss at 120kg cap with zero feedback. |
| **H-12** | Client-side gaps for co-op: eat/feed/equip/capture/command are not RPCs; Echo health/trust/bond/level not replicated; client registries empty; EventBus never reaches clients | `PlayerCharacter.cpp:513, 552, 610, 653`; `EchoCharacter.cpp:45-54`; `ItemRegistrySubsystem.cpp:19-22` | Remote clients silently no-op most gameplay. Acceptable "single-player first" but must be tracked. |
| **H-13** | No gamepad bindings in runtime IMC (keyboard/mouse only) — checklist P2 "Controller support" | `PlayerCharacter.cpp:202-250` | Controller players cannot play at all. |
| **H-14** | Research Desk building exists and draws power but has no interact function; no research UI of any kind | `ContentLibrary.cpp:346-348`; no research interact | Research points accrue but the player can never spend them in normal play (ties to C-2). |

## MEDIUM — quality/robustness defects in working systems

| ID | Gap | Evidence |
|---|---|---|
| M-1 | Status-effect system fully implemented but dormant — zero callers; `SpeedMultiplier` unconsumed | `SurvivalComponent.cpp:189-213` |
| M-2 | Sprint drains no stamina (gate-only); block movement penalty dead (never refreshed on toggle) | `PlayerCharacter.cpp:395, 402-405` vs `495-509` |
| M-3 | Dead code: player `AttackDamage/AttackDistance/AttackCooldownSeconds/LastAttackTimeSeconds`; `bReplicatedDodgeTimer`; `MaxStackSize` unenforced; `PendingOutputTarget`; `GetResearch()`; `IsLocationPowered` dead loop; `StoredCharge` written-never-read; `SetSwitchedOn` unreachable; `AbilityIds` unused; `AttackElement` equipment override unimplemented | various |
| M-4 | HUD hardcodes 20°C; `PushNotification` has zero callers; refresh does a camera trace every 0.15s (duplicated cost); party command not replicated (stale on clients) | `HudWidget.cpp:174-176, 135-142, 119, 182` |
| M-5 | Capture UX: cooldown starts before resonator check; `OnCaptureResult` has no UI subscriber; resonator consumed on failed rolls (design decision needed) | `CaptureComponent.cpp:78-87, 102` |
| M-6 | Power: one global grid contradicting documented 1200cm connectivity; `StoredEnergy` not persisted; lamp posts draw power with no light behavior | `PowerSubsystem.cpp:68-111` |
| M-7 | Dungeon state not saved (reload resurrects encounters); loot granted to first player only (MP-fragile); no entrance/gates | `DungeonGeneratorActor.cpp:19-27, 144-184` |
| M-8 | Quest_Spark hostile count: only 1 wild Gloomfang + dungeon pool — "defeat 3" needs respawn (ecosystem has none) | `Bootstrapper.cpp:227-243` |
| M-9 | Resource nodes not replicated; Thai/English prompt mix (ResourceNode "เก็บ", RestPoint "พักฟื้น" vs English elsewhere) | `ResourceNode.cpp:13, 79`; `RestPoint.cpp:68` |
| M-10 | `RemoveFromRoster` leaves stale `SpawnedParty` entries; roster has no per-player separation; `CaptureAll` cheat bypasses roster | `EchoRosterSubsystem.cpp:31-41`; `CheatManager.cpp:170-188` |
| M-11 | Journal sees through walls (no LOS trace); weather visibility multiplier has zero consumers (journal/AI unaffected by fog) | `JournalSubsystem.cpp:103-114` |
| M-12 | Missing tags: `Element.Ember`, `State.Creature.Injured`, `State.Creature.Dead`; `FAstrawildStableId` lacks `GetTypeHash` | `GameplayTags.h`; `Types.h:28-44` |
| M-13 | ContentLibrary log string hardcodes counts (will drift); `Tech_AdvancedEnergy` unlocks nothing (dead-end node); station ids are stringly-typed with no registry | `ContentLibrary.cpp:584, 373-374` |
| M-14 | Timer safety: respawn next-tick lambda captures raw `this`; `OnUnPossess` clears all timers bluntly; `ExecuteProtect` does `GetAllActorsOfClass` per think (O(n) at 4Hz) | `GameMode.cpp:74-79`; `EchoAIController.cpp:74, 446` |

## LOW — polish, hygiene, forward-looking

| ID | Gap |
|---|---|
| L-1 | 7 of 9 automation tests are tautological (assert local arithmetic / literal `true`); no save round-trip test; no ContentLibrary sanity test |
| L-2 | Thirst decay ≈12 min contradicts documented ~20 min (header comment) |
| L-3 | No dedicated server target file despite MP config (IpNetDriver, MaxPlayers=4) |
| L-4 | `GameDefaultMap` points at `/Engine/Maps/Entry` — no project map exists (packaging risk) |
| L-5 | `GetUnlockedBuildings(PlayerId)` dead parameter (research is global) |
| L-6 | Log category mismatches in cheats (equip failure logged under `LogAstrawildAI`) |
| L-7 | `CREATURE_SYSTEM.md` one content wave stale (5 vs 7 species; herd "PLANNED" but wired) |
| L-8 | Checksum covers header only (documented, but should be payload-covering before release) |
| L-9 | Migrated v1 saves re-migrate every load (not written back after in-memory migration) |

---

## FIRST IMPLEMENTATION BATCH — selected by dependency order

The Checklist V2 priority ladder is Foundation → Player → Survival → Inventory → Combat → First Echo → Capture → Creature Combat → Crafting → Base Building → Power → Research → … → Dungeon → Boss → Save/Load. The audit shows the slice is structurally complete but severed at specific integration points. The correct first batch is **"make the existing loop actually run end-to-end"** — no new content, no new systems:

**BATCH 1 — "Core Loop Unblocked" (dependency-ordered):**
1. **C-1** Fix crafting station compile error *(unblocks build)*
2. **C-3** Runtime navmesh in WorldBootstrapper *(unblocks all creature movement)*
3. **C-8** Respawn input fix *(unblocks death/respawn loop)*
4. **C-2** Research unlock path: auto-grant `Tech_BasicCrafting` at session start + Research Desk interact → spend points on unlocked-able techs *(unlocks tech tree, quests 4-6, power buildings)*
5. **C-6** Build piece cycling input + HUD indicator *(unblocks base building choice)*
6. **C-7** Work site interactability (assign/collect) *(unlocks Echo utility + automation)*
7. **C-4** Dungeon completion fix (empty rooms auto-clear + completion event wiring) *(unlocks dungeon loop)*
8. **C-5** Boss spawn in boss room + damage mitigation routing *(unlocks boss fight)*
9. **H-4** Weather dangling-ref fix *(UB before first runtime test)*
10. **H-1/H-2/H-3** Save/load repair pack: restore vitals, respawn party from roster, single roster import, autosave-loadable "continue" path *(unblocks the save→reload→continue leg of the loop)*

Batch 1 intentionally excludes: new content (species/quests/biomes), multiplayer RPC work (H-12), armor/weapon/drone frameworks (P9), and visual polish — those follow only after the loop runs.

**After each item: Compile (on target machine) → Runtime test → Save/load test → Update Checklist → Document → Commit.** In this sandbox (no UE engine), compile verification is impossible; commits will carry honest `compile-status: NOT RUN` notes and the batch must be engine-verified by the user before Batch 2 proceeds.
