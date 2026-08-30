# ASTRAWILD — IMPLEMENTATION GAP REPORT

**Date:** 2026-08-30
**Source of truth:** Production Master Plan V2 + Production Checklist V2 + Ultimate Roadmap V3
**Evidence base:** `ASTRAWILD_UE5_PRODUCTION_AUDIT.md` (same date, line-cited)
**Rule:** No system may be deleted to make a build pass. No mock implementations to flip checklist marks. No feature-complete claims from documentation alone.

---

## Current tally (post-`021f93a` — Wave 5 Batch 3)

| Priority | Total | Closed Batch 1 | Closed Batch 2 | Closed Batch 3 | Outstanding |
|---|---|---|---|---|---|
| **CRITICAL** | 9 (C-1..C-8 + C-1b discovered during impl) | 9 | 0 | 0 | **0** |
| **HIGH** | 14 | 8 (H-1, H-2, H-3, H-4, H-5, H-7, H-8, H-14) | 3 (B2-A hostile respawn / B2-B building dismantle / B2-C power persistence) | 0 | **3 fully + 3 partial** (H-6 MP dupe vector, H-11 crafting output validation, H-12 MP RPC layer, H-13 gamepad, plus partial: H-9 UnregisterEcho decrement, H-10 ReachLocation/SurviveTime objective types, Ultimate Gap "Power persistence" grid-level StoredEnergy) |
| **MEDIUM** | 14 | 2 (M-4 HUD temp, M-12 missing tags) | 0 | 1 (M-1 status system dormant — now live via the element-driven factory + combat hooks) | 11 |
| **LOW** | 9 | 0 | 0 | 0 (1 partial: L-1 — 2 real tests added of the tautological set) | 9 |

**Honest compile status:** `NOT RUN (sandbox has no UE engine — must be verified on target machine)`. All "closed" marks above mean "closed at source level — implementation exists with real logic and correct integration, verifiable by inspection". They do **not** mean compiled-and-run.

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
| M-1 | ~~Status-effect system fully implemented but dormant — zero callers; `SpeedMultiplier` unconsumed~~ **CLOSED in Batch 3 (`021f93a`)** — `MakeElementalStatusEffect` factory + player-weapon and creature-attack application hooks + `GetStatusSpeedMultiplier` consumed by `RefreshMovementSpeed` (see THIRD IMPLEMENTATION BATCH below) | `SurvivalComponent.cpp:189-213` |
| M-2 | Sprint drains no stamina (gate-only); block movement penalty dead (never refreshed on toggle) | `PlayerCharacter.cpp:395, 402-405` vs `495-509` |
| M-3 | Dead code: player `AttackDamage/AttackDistance/AttackCooldownSeconds/LastAttackTimeSeconds`; `bReplicatedDodgeTimer`; `MaxStackSize` unenforced; `PendingOutputTarget`; `GetResearch()`; `IsLocationPowered` dead loop; `StoredCharge` written-never-read; `SetSwitchedOn` unreachable; `AbilityIds` unused; ~~`AttackElement` equipment override unimplemented~~ (**that piece closed in Batch 3** — `GetResolvedAttackElement()` + weapon `Element` field, `021f93a`) | various |
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
| L-1 | 7 of 9 automation tests are tautological (assert local arithmetic / literal `true`); no save round-trip test; no ContentLibrary sanity test — **PARTIAL as of Batch 3 (`021f93a`)**: `FAstrawildArmorMathTest` + `FAstrawildStatusEffectFactoryTest` call the production statics (11 tests total, 4 real); the remaining 7 tautological tests + save round-trip + content sanity still pending |
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

---

## SECOND IMPLEMENTATION BATCH — Wave 4 Batch 2 ("Integrity")

The three Batch-2 work items below were scoped during RESEARCH-2 (read-only research pass) and
code-reviewed during REVIEW-2 (read-only compile-risk review) before landing in commit `d5d23c2`.
REVIEW-2 found one MEDIUM-risk runtime bug in Item A (population clamp silently broken — see
`ASTRAWILD_UE5_PRODUCTION_AUDIT.md` §22 for the one-line fix that landed inline). All three items
are now **closed at source level** — compile/runtime verification still required on the target
machine (Windows + UE 5.8 + Antigravity).

### Closed in Batch 2 — HIGH priority

| ID | Brief | Files | What's closed | What remains |
|---|---|---|---|---|
| **B2-A — Hostile respawn** | `UAstrawildHostileSpawnerSubsystem` (NEW `UTickableWorldSubsystem`, server-only Tick @ 25 s, `SpawnRadius=1800 cm` ring-biased outward 30–100 %, `FRandomStream` seeded from `WorldSeed` for deterministic SP). Tunables: `TargetGloomfangPopulation=4`, `TargetEmberfangPopulation=2`, `RespawnIntervalSeconds=25.0f`. REVIEW-2 fix: re-`RegisterEcho` after `Echo->InitializeFromDefinition(Definition)` so the species `WildCount` bumps (BeginPlay's `RegisterWithEcosystem` ran before `EchoDefinition` was set). Quest integration: Quest 5 "Defeat 3 Gloomfang" chain-completes via the existing death pipeline (`EchoCharacter::OnDefeated → EventBus TAG_Astrawild_Event_HostileDefeated → QuestComponent::ApplyEventToQuest`) — no new quest wiring required. | `Source/AstrawildCore/Public/AstrawildHostileSpawnerSubsystem.h` (NEW), `Source/AstrawildCore/Private/AstrawildHostileSpawnerSubsystem.cpp` (NEW) | The hostile-respawn piece of H-9; the Quest 5 chain-completion piece of H-10; the runtime-spawn `WildCount` race (REVIEW-2 fix) | `UnregisterEcho` still does not decrement `WildCount` (MEDIUM, pending Batch 3+); `ReachLocation` / `SurviveTime` objective types still unimplemented (H-10 remainder, pending Batch 3); `LogAstrawildBuilding` reused for spawner logging (LOW nit, non-blocking) |
| **B2-B — Building dismantle + refund** | `UAstrawildBuildingComponent::DismantleBuilding(AActor*)` — server-authoritative (`GetOwnerRole() == ROLE_Authority` gate), weight-safe (`CanAddItem` refuses if bag full — dismantle aborts, no material loss into the void), refunds via `UAstrawildInventoryComponent::AddItemSilent(FName, int32)` (structurally identical to `AddItem` minus the EventBus publish block — dismantling materials do NOT advance `CollectItem` quest objectives). `AAstrawildPlayerCharacter::DeleteBuildingAction` bound to `EKeys::Z`, 5 m crosshair trace via `FollowCamera + LineTraceSingleByChannel ECC_Visibility`. HUD toast via existing `AAstrawildPlayerController::Notify` (no new widget). 19 actions / 19 keys now (was 17 after Batch 1's X = EquipBest). | `Source/AstrawildCore/Public/AstrawildBuildingComponent.h`, `Source/AstrawildCore/Private/AstrawildBuildingComponent.cpp`, `Source/AstrawildCore/Public/AstrawildInventoryComponent.h`, `Source/AstrawildCore/Private/AstrawildInventoryComponent.cpp`, `Source/AstrawildCore/Public/AstrawildPlayerCharacter.h`, `Source/AstrawildCore/Private/AstrawildPlayerCharacter.cpp` | The Ultimate Gap's "Delete/refund" HIGH row (full close — weight-safe refund policy with no false quest advancement). The piece of H-2 (base building UX incompleteness) that called for delete-with-refund | Server-side material deduction for MP (H-6 / MP dupe vector) still pending — `DismantleBuilding` uses direct method calls gated on `GetLocalRole() == ROLE_Authority`, fine for SP but not yet MP-safe; full inventory transfer API for storage buildings still missing (Batch 3) |
| **B2-C — Power persistence** | `FAstrawildBuildingSaveData` gains `bool bIsPowered = false` (additive — old saves deserialize `false` default, re-resolve within ≤2 s — actually same-frame because `LoadWorld` calls `ResolveGridNow`). `AAstrawildBuildingActor` gains `UPROPERTY(Replicated) bool bIsPowered = false` + `DOREPLIFETIME` line. `ToSaveData()` captures `Power->IsBuildingPowered(this)` at save time. `FromSaveData()` restores the hint value (overwritten on next `ResolveGrid`). `UAstrawildPowerSubsystem::ResolveGridNow()` — public `UFUNCTION BlueprintCallable` wrapper for private `ResolveGrid()`. `ResolveGrid()` now writes back `Consumer->bIsPowered = bPowered` for replication sync (UE net driver short-circuits unchanged values — no extra bandwidth). `UAstrawildSaveSubsystem::LoadWorld` calls `Power->ResolveGridNow()` right after the building spawn loop — first frame after load is correct (no 2 s brownout flicker). | `Source/AstrawildCore/Public/AstrawildTypes.h`, `Source/AstrawildCore/Public/AstrawildBuildingActor.h`, `Source/AstrawildCore/Private/AstrawildBuildingActor.cpp`, `Source/AstrawildCore/Public/AstrawildPowerSubsystem.h`, `Source/AstrawildCore/Private/AstrawildPowerSubsystem.cpp`, `Source/AstrawildCore/Private/AstrawildSaveSubsystem.cpp` | The Ultimate Gap's "Power persistence" HIGH row **partially** — per-actor power state persists across save/load and is replicated to clients; first-frame correctness after load | Grid-level `StoredEnergy` (battery state) is NOT saved (H-6 / Ultimate Gap "Power persistence" remainder); `StoredCharge` per-building still written-never-read (dead data); power connectivity still one global grid contradicting the documented 1200 cm radius (M-6, pending Batch 3+); lamp posts draw power with no light logic (M-6, pending Batch 3) |

### Compile-risk review (REVIEW-2 — read-only, done before user runs on target machine)

- **HIGH RISK (compile blockers):** NONE. Every UCLASS/GENERATED_BODY/Tick override/DOREPLIFETIME/
  UPROPERTY/UFUNCTION pattern in the diff is well-formed; every cross-class symbol
  (`FindEcho`, `GetWildPopulation`, `IsBuildingPowered`, `PublishEvent`, `Notify`, `AddItemSilent`,
  `CanAddItem`, `DismantleBuilding`, `ResolveGridNow`, `MakeRuntimeAction`, `BindAction`, `MapKey`,
  `SpawnActor`, `InitializeFromDefinition`) has a verified-visible declaration with matching signature;
  every include chain that the new code depends on is either explicitly added
  (`AstrawildPowerSubsystem.h` in `SaveSubsystem.cpp:12`) or already present from Batch 1
  (`AstrawildBuildingActor.h` in `BuildingComponent.cpp:3`, `AstrawildDataAssets.h:5`,
  `AstrawildPlayerController.h` in `PlayerCharacter.cpp:20`). No forward-declared class used as complete.
- **MEDIUM RISK (latent runtime bug in Item A — population clamp silently broken):** caught and fixed
  inline in `d5d23c2`. See `ASTRAWILD_UE5_PRODUCTION_AUDIT.md` §22 for the full root-cause analysis
  and the one-line fix.
- **LOW RISK / nits:** see `ASTRAWILD_UE5_PRODUCTION_AUDIT.md` §22 "Residual LOW-risk items".

### Related commit (not part of Batch 2 scope but lands in the same wave)

- `6f14520` — `fix(bootstrapper): add missing Engine/StaticMeshActor.h include — compile blocker for SpawnActor<AStaticMeshActor>`. This is the only HIGH-RISK compile blocker surfaced by REVIEW-1 for Batch 1; the fix landed just before Batch 2's commit. Batch 2 code depends transitively on this fix being present.

### Next batch (Batch 3 — "Depth")

Batch 3 (Depth) — status effects + hit reactions + armor + laser weapon + gamepad + real tests + vendor UI + biome 2 prep. Scope per Ultimate Gap Analysis §3 (gameplay feel) + §5 (technology framework) + §6 (world biomes) + §11 (QA real tests). The MP batch (parallel after Batch 2): client registry, RPC layer, creature replication, building server-side deduction, node replication, join/leave.
