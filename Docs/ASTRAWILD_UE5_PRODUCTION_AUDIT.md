# ASTRAWILD — UE5 PRODUCTION AUDIT (Evidence-Based)

**Date:** 2026-08-30
**Auditor:** Production Lead (AI agent)
**Scope:** Full repository at commit `0402472` — 85 C++ source files (~13,400 lines), single module `AstrawildCore`, UE 5.8.
**Method:** Every file in `Source/AstrawildCore/` was read and cross-referenced. Claims below cite file + line evidence. Status marks follow the Checklist V2 legend (`[x]` verified, `[~]` partial, `[!]` failed/blocked, `[ ]` not implemented).

## 0. Executive Summary

ASTRAWILD is a **source-complete, never-compiled** code-first vertical slice. The zero-asset playability strategy (procedural world bootstrapper + C++-registered content library + runtime-built Enhanced Input + pure-C++ HUD) is genuine and coherent, but the project has **never been compiled** (`Docs/BUILD_STATUS.md:5`), and this audit found:

- **1 confirmed compile error** (`AstrawildCraftingStationActor.cpp:52` — range-for type mismatch).
- **1 latent undefined-behavior bug** (`AstrawildWeatherSubsystem.cpp:87` — dangling reference return).
- **6 runtime/gameplay blockers** that prevent the vertical-slice loop from completing in normal play (no navmesh, research unlock dead-end, dungeon never completes, phased boss never spawns, build mode locked to one piece, work-site automation unreachable).
- Multiplayer is architectural scaffolding only: attack/craft/place are server-authoritative, but eat/feed/equip/capture/party-command silently no-op for remote clients.

**The full loop Start → gather → craft → build → research → power → dungeon → boss → save → reload CANNOT complete without cheat commands.** This document is the evidence record; `ASTRAWILD_IMPLEMENTATION_GAP_REPORT.md` prioritizes the fixes; `ASTRAWILD_ULTIMATE_GAP_ANALYSIS.md` maps the long-term gaps against Roadmap V3.

---

## 1. P0 — Audit

| Item | Status | Evidence |
|---|---|---|
| Repository audit completed | [x] | This document + `ASTRAWILD_UE5_ARCHITECTURE_AUDIT.md` |
| Existing UE5 project builds | [!] | **Never compiled.** Sandbox has no UE engine. 1 confirmed compile error at `AstrawildCraftingStationActor.cpp:52`: `GetAllRecipes()` returns `TArray<UAstrawildRecipeDefinition*>` (`AstrawildItemRegistrySubsystem.h:73`) but is iterated as `TPair<FName, TObjectPtr<...>>` — cannot compile. First engine build will fail here. |
| Systems classified KEEP/REFACTOR/REPLACE/REMOVE/ADD | [x] | Section 10 below |
| Architecture gaps documented | [x] | `ASTRAWILD_ULTIMATE_GAP_ANALYSIS.md` |
| Technical debt documented | [x] | Section 9 below |
| Current baseline build recorded | [!] | Cannot record without engine. Baseline = "does not compile" until proven otherwise on target machine. |

## 2. P1 — Foundation

| Item | Status | Evidence |
|---|---|---|
| UE5 version locked/documented | [x] | `ASTRAWILD.uproject:3` → `"5.8"`; targets use `Unreal5_8` include order |
| C++ architecture established | [~] | Single runtime module `AstrawildCore` (`AstrawildCore.Build.cs`) vs. 8-module plan in Master Plan v1 §8. Acceptable for vertical slice; documented deviation. |
| Gameplay subsystems established | [x] | 10 subsystems: Save/Research/Roster (GameInstance), ItemRegistry/EventBus/Time/Weather/Ecosystem/Journal/Power (World). All have real `DoesSupportWorldType` + logic. |
| Logging categories established | [x] | 7 categories, 1:1 declare/define (`AstrawildLog.h:9-15` / `.cpp:3-9`); used consistently. |
| Gameplay Tags taxonomy established | [~] | 78 native tags (`AstrawildGameplayTags.h`). **Missing:** `Element.Ember` (used by `Echo_Emberfang`, `ContentLibrary.cpp:309`), `State.Creature.Injured`, `State.Creature.Dead`. Tags lack project prefix (collision risk noted in audit). |
| Data Asset conventions established | [x] | 8 `UPrimaryDataAsset` classes with `GetPrimaryAssetId` overrides (`AstrawildDataAssets.h:23-415`). Instances are C++-built defaults in `ContentLibrary` (no .uasset yet — by design for zero-asset slice). |
| Stable ID strategy established | [~] | `FAstrawildStableId` + FGuid InstanceIds (`AstrawildTypes.h:28-44`). Gaps: no `GetTypeHash` (unusable as TMap key); `OwnerPlayerId` on buildings never populated (`Types.h:460`, no writer). |
| Enhanced Input architecture established | [x] | Runtime-built IMC with 17 actions when no assets assigned (`AstrawildPlayerCharacter.cpp:177-254`); GC-safe via `RuntimeActions` UPROPERTYs (`.h:211-216`). |
| Automated test foundation established | [!] | 9 tests exist (`AstrawildAutomationTests.cpp`) but **7 of 9 are tautological** (assert arithmetic on local constants; `Capture.DesignRuleBounds` literally asserts `true` at line 77). Only checksum determinism (l105) and quest objective progress (l125) exercise real code. No save round-trip, no world test. |

## 3. P2 — Player

Implementation: `AAstrawildPlayerCharacter` (`.h` 217 ln / `.cpp` 786 ln), `AAstrawildPlayerController`.

| Item | Status | Evidence |
|---|---|---|
| Third-person character | [x] | SpringArm 360cm + camera (`cpp:57-64`), orient-to-movement (`cpp:49-52`). Placeholder cylinder mesh (`cpp:66-76`). |
| Camera | [x] | As above; no lag/offset customization. |
| Movement | [~] | Walk 450 / Sprint 700 (`cpp:55`, `RefreshMovementSpeed` `cpp:388-408`). **No crouch, no climb, no swim handling.** |
| Sprint | [~] | Speed switch real; stamina-gated (`cpp:395`) but **sprinting drains no stamina** — gate only bites after dodge/heavy attacks. |
| Jump | [x] | JumpZ 600, AirControl 0.35 (`cpp:53-54`); input bound `cpp:284`. |
| Dodge | [x] | `Q` → `RequestDodge` (`cpp:479-493`); server RPC + i-frames handled in CombatComponent. |
| Interaction | [x] | 300cm camera trace, `IAstrawildInteractable` dispatch (`cpp:418-452`, `766-785`). |
| Controller support | [ ] | Runtime IMC maps **keyboard/mouse only** — zero gamepad bindings (`cpp:202-250`). |
| Input remapping | [ ] | No remap UI/API. |
| Traversal foundation | [ ] | None. |
| Animation architecture | [ ] | Zero animations; attacks/dodge are pure logic on static meshes. |

**Additional player bugs:** (a) respawned pawn never re-adds the IMC — `BeginPlay` early-returns before input setup when controller is null during `RestartPlayer` spawn (`cpp:122-133`) → **input death after respawn**; (b) block movement penalty is dead code — `RefreshMovementSpeed` never called on block toggle (`cpp:402-405` has no caller from `StartBlock/StopBlock cpp:495-509`); (c) dead fields `AttackDamage/AttackDistance/AttackCooldownSeconds/LastAttackTimeSeconds` (`.h:135-208`) never referenced.

## 4. P3 — Survival

Implementation: `UAstrawildSurvivalComponent` (`.h` 120 / `.cpp` 238).

| Item | Status | Evidence |
|---|---|---|
| Health | [x] | 100/100, authority-gated `ApplyDamage` (`cpp:114-137`), god mode (`cpp:215-219`). |
| Stamina | [~] | Regen 14/s (`cpp:42`); consumed by dodge(22)/heavy(25). No sprint drain (see P2). |
| Hunger | [x] | Decay 0.083/s ≈ 20 min (`cpp:36-40`); starvation damage `cpp:45-49`. |
| Thirst | [x] | Decay 0.14/s ≈ 12 min (header comment claiming ~20 min is wrong, `.h:14-15`). |
| Temperature | [~] | 20°C base + weather offset (`cpp:66-72`); exposure damage outside 4–36°C (`cpp:53-56`). No time-of-day/shelter/biome factors. |
| Status effects | [x] | **Batch 3 (`021f93a` — compile pending): NOW LIVE.** `MakeElementalStatusEffect` factory feeds `AddStatusEffect` from real combat hooks (player weapon hits via `EchoCharacter::ApplyElementalDamage`; creature attacks via `EchoAIController::TryAttackTarget` player branch). DoT ticks + expiry in `Tick`; `GetStatusSpeedMultiplier` consumed by `RefreshMovementSpeed`; `FullRestore`/`SetStatsForRestore` broadcast removals (REVIEW-3 M-2). **VERIFIED AT SOURCE (021f93a — compile pending).** |
| Food | [x] | `ApplyConsumption` (`cpp:139-150`) via `SmartConsume` (`PlayerCharacter.cpp:608-649`). |
| Medicine | [x] | Bandage (heal 40) / Salve (heal 70) items through same path. |
| Death | [x] | `Die()` → `OnDied` (`cpp:221-233`) → GameMode respawn chain. |
| Respawn | [~] | 5s delay, `RestartPlayer` + `HandleRespawn` — but **hardcoded spawn (0,0,150)** (`GameMode.cpp:101`) and the respawn input-loss bug above. |

## 5. P3 — Inventory / Equipment

Implementation: `UAstrawildInventoryComponent` (`.h` 114 / `.cpp` 282).

| Item | Status | Evidence |
|---|---|---|
| Item Definition | [x] | `UAstrawildItemDefinition` + 22 C++-registered items (`ContentLibrary.cpp` — 19 through wave 3, +3 armor in Batch 3); `Element` + `ArmorRating` fields added in Batch 3. |
| Item Instance | [ ] | Storage is `TMap<FName,int32>` (`.h:105`) — no per-instance items, no durability/quality. |
| Stack | [~] | Quantity-only; `MaxStackSize` field exists but **never enforced** (`AddItem cpp:92-119` has no cap logic). |
| Weight | [x] | 120kg cap, 1kg unknown-item fallback, divide-by-zero guarded (`cpp:58-86`). |
| Equipment | [~] | **3 slots as of Batch 3 (`021f93a` — compile pending): weapon + shield + armor (torso)** — `EquippedArmorItemId` (replicated) with routing by `ArmorRating > 0` before the statless fallback, `GetEquippedArmorRating`/`GetEquippedWeaponElement`, additive `OnArmorChanged` delegate. Still a reference, not a transfer; no per-slot unequip UI. Helmet/arms/legs/boots/core module slots still missing (P9 checklist). |
| Inventory UI | [ ] | No inventory screen; HUD shows equipment readout only (`HudWidget.cpp:220-237`). |
| Drop/pickup | [ ] | No drop actor, no pickup class. |
| Transfer | [ ] | No container API. |
| Persistence | [x] | Stacks + equipped ids (weapon/shield/**armor**) saved/restored (`SaveSubsystem.cpp:78-83, 221-239` — armor added in Batch 3, `HasItem`-guarded). |
| Duplication tests | [ ] | None. |

## 6. P4 — Combat

Implementation: `UAstrawildCombatComponent` (`.h` 145 / `.cpp` 305), `AAstrawildDamageTarget`.

| Item | Status | Evidence |
|---|---|---|
| Damage framework | [x] | Elemental weakness ×1.5 / resist 0.2 / flat defense (`EchoCharacter.cpp:179-240`); server-side mitigation (`CombatComponent.cpp:262-273`). |
| Melee | [x] | Light 25 / Heavy 60 + weapon power; multi-sphere sweep 320cm (`cpp:185-260`). |
| Ranged | [ ] | No projectile/ranged path at all. |
| Dodge/parry architecture | [~] | Dodge real (RPC + 0.4s i-frames + launch impulse, `cpp:125-183`). No parry. |
| Hit reaction | [x] | **Batch 3 (`021f93a` — compile pending): stagger implemented** — `AAstrawildEchoCharacter::ApplyStagger` (server-only, clamped ≤ 2 s, zeroes `MaxWalkSpeed` + `SetAIState(Staggered)`); player stagger via `UAstrawildCombatComponent::ApplyStagger` + `OnStaggerStateChanged`. **VERIFIED AT SOURCE.** |
| Stagger | [x] | **Batch 3 triggers (verified at source):** Echo single hit ≥ 20% of `GetMaxHealth()` → 0.8 s; player hit ≥ 35 mitigated (`StaggerDamageThreshold`) → 0.6 s (`PlayerStaggerSeconds`); boss landed hits always stagger 0.6 s. `Think()` gate stops movement + skips decisions while STILL re-arming the think timer. **VERIFIED AT SOURCE (021f93a — compile pending).** |
| Death | [x] | Numeric defeat + loot + EventBus events (`EchoCharacter.cpp:211-236`). |
| Status effects | [x] | **Batch 3 (`021f93a` — compile pending): element-driven and live on both sides.** Shared factory `MakeElementalStatusEffect` (Ember→Burn 4 s DPS `2+5%×hit` / Frost→Chill 3 s ×0.5 / Flora→Poison 6 s 2 DPS / Pulse→Shock 0.8 s ×0.3; None/Light/Ash → nothing); creatures carry replicated `StatusEffects` + server `ApplyStatusTicks` (DoT on `CurrentHealth`; DoT defeats route through the full defeat pipeline); players receive statuses from creature elements via `SurvivalComponent`. **VERIFIED AT SOURCE.** |
| Element system | [x] | 7 elements with weakness/resist on creatures; **Batch 3 closes the weapon-element-override gap** — `GetResolvedAttackElement()` returns the equipped weapon's `Element` when set (Dawn Crystal Blade = Pulse → Shock), falling back to the `AttackElement` tunable (`CombatComponent.cpp:370-384`). **VERIFIED AT SOURCE (021f93a — compile pending).** |
| First hostile enemy | [x] | `Echo_Gloomfang` (`ContentLibrary.cpp:284-290`) — aggro + chase + attack via `AAstrawildEchoAIController` (`EchoAIController.cpp:212-216, 567-607`). |

**Combat authority is genuinely server-side** — 4 reliable Server RPCs with re-validation (`CombatComponent.h:130-140`). Dead replicated property `bReplicatedDodgeTimer` never read (`.h:122-123`).

## 7. P5 — First Complete Echo

Implementation: `AAstrawildEchoCharacter` (`.h` 217 / `.cpp` 584), `AAstrawildEchoAIController` (`.cpp` 635), `UAstrawildCaptureComponent`, `UAstrawildEchoRosterSubsystem`.

| Item | Status | Evidence |
|---|---|---|
| Species Definition | [x] | 7 species with full templates (`ContentLibrary.cpp:235-315`). |
| Instance state | [~] | InstanceId/personality/needs/trust/bond/XP (`EchoCharacter.h:59-103`). No traits, no memories, no nickname. |
| AI Controller | [x] | Pure-C++ FSM, self-rescheduling `Think()` at 0.25s+, sight perception reconfigured per species (`EchoAIController.cpp:20-65`). No BT/StateTree (documented future path). |
| Navigation | [!] | **No navmesh exists anywhere.** Zero `NavMesh` references in Source; bootstrapper spawns no `ANavMeshBoundsVolume`. All `MoveToLocation/MoveToActor` calls will fail in the zero-asset world → **creature locomotion cannot work at runtime** until a runtime navmesh (or movement fallback) is added. |
| Perception | [x] | Sight config 1500/2200cm, 75° (`cpp:23-30`); reacts to players only (`cpp:106`). No hearing/damage senses. |
| Behavior | [x] | 10 of 16 states implemented with real executors (`ExecuteState cpp:265-304`): Explore/Flee/Combat/Follow/Protect/Work/Sleep/SearchFood/Socialize/Idle + Investigate alias. |
| Personality | [x] | 10 archetypes with real modifiers: flee-threshold, aggro-radius, work-speed, obedience (`EchoCharacter.cpp:323-371`). |
| Needs | [x] | Hunger/energy/mood decay with LOD throttling, consequences (HP drain, mood), in-world-hour scaling (`cpp:477-529`). |
| Combat | [x] | Element damage + cooldown, attack via controller (`EchoAIController.cpp:567-607`). |
| Capture | [x] | Multi-factor chance (base/difficulty/resilience/weaken/trust/weather/time — `EchoCharacter.cpp:259-293`), resonator consumption, journal/tracking bonuses (`CaptureComponent.cpp:37-63`). |
| Ownership | [~] | `OwnerPlayerId` set on capture and replicated (`EchoCharacter.h:99`); roster is **single shared GameInstance roster** — no per-player rosters. |
| Follow | [x] | Follow state + command (`EchoAIController.cpp:163-177`). |
| Commands | [~] | 7-command enum; 5 reachable from input (`CyclePartyCommand` `PlayerCharacter.cpp:511-548`). `Retreat`/`HoldPosition` unreachable. **Obedience roll real** (`EchoCharacter.cpp:458-475`). |
| Relationship | [x] | Trust/bond with gameplay effects (capture chance, obedience, feeding ×2 preferred food, passive bond growth). |
| Utility role | [!] | Work affinities defined per species, work-rate math real (`WorkSiteActor.cpp:60-123`) — but **the work-site system is dead**: `AssignWorker`/`CollectOutput` have zero callers, site is not interactable. Captured Echoes can never work. |
| Save/Load | [~] | Roster v2 with 11 fields round-trips (`EchoCharacter.cpp:544-584`). Gaps: party actors **not respawned on load** (SaveSubsystem.cpp:231-245 comment claims respawn, code only destroys+reimports); v1 path hardcodes XP=0 (`EchoCharacter.cpp:538`); roster imported **twice** per load (`SaveSubsystem.cpp:203, 244`); wild populations not saved. |
| Edge-case tests | [ ] | None beyond tautological smoke assertions. |

**Echo-specific bugs:** `OnAIStateChanged` never broadcasts (AI controller writes `CurrentAIState` directly, `EchoAIController.cpp:256-262`); captured "Attack" command can target the owner (no owner exclusion, `cpp:357-363`); `CurrentHealth/Trust/Bond/Level/bCaptured` not replicated (client UI impossible); `ExecuteProtect` runs `GetAllActorsOfClass` every think (`cpp:446`).

## 8. P6 — Echo Platform

| Item | Status | Evidence |
|---|---|---|
| Data-driven species creation | [x] | Definitions + registry; new species = data entry (C++ today, .uasset-ready). |
| Stats | [x] | HP/ATK/DEF/Speed/Stamina/CaptureResilience (`Types.h:63-86`). |
| Traits | [ ] | No trait system. |
| Abilities | [ ] | `AbilityIds` field exists, zero runtime consumers (`DataAssets.h:151`). |
| Habitat | [x] | Preferred biomes on definitions; biome tags exist. |
| Diet | [~] | Preferred foods drive feeding trust ×2; no autonomous eating (SearchFood is a trickle, `EchoAIController.cpp:526-538`). |
| Time behavior | [x] | Activity windows affect sleep state + capture bonus (`EchoCharacter.cpp:295-321`). |
| Weather behavior | [~] | Preferred weather = capture bonus only; no AI response to weather. |
| Work roles | [~] | Affinity data + site math real; **work system unreachable** (see P5). |
| Additional Echo content pipeline | [~] | Adding species is trivial in `ContentLibrary`; no .uasset pipeline yet. |

## 9. P7 — Crafting

Implementation: `UAstrawildCraftingComponent` (`.cpp` 307), `AAstrawildCraftingStationActor`, `UAstrawildCraftingScreenWidget`.

| Item | Status | Evidence |
|---|---|---|
| Recipe definition | [x] | 10 recipes (`ContentLibrary.cpp:183-230`). |
| Ingredients | [x] | Two-pass check-then-consume (`CraftingComponent.cpp:152-167`). |
| Stations | [!] | Station gate + proximity real (`cpp:44-68`) — but the **station interact path does not compile** (`CraftingStationActor.cpp:52`, see P0). |
| Technology requirements | [x] | `RequiredTechId` enforced in `CanCraft` (`cpp:78-85`). |
| Crafting UI | [ ] | `AstrawildCraftingScreenWidget` is an Abstract contract with no asset and no creator (`BUILD_STATUS.md:89-95`); station fallback is the only in-world affordance — and it fails to compile. |
| Output validation | [!] | `AddItem` return ignored (`cpp:286-289`) — **crafted items silently destroyed at weight cap**. |
| Save/load compatibility | [~] | Active craft not persisted (acceptable); outputs already in inventory persist. |

## 10. P7 — Base Building

Implementation: `UAstrawildBuildingComponent` (`.cpp` 293), `AAstrawildBuildingActor`.

| Item | Status | Evidence |
|---|---|---|
| Foundation/Floor/Wall/Roof | [~] | 10 building definitions (`ContentLibrary.cpp:320-355`) incl. foundation/wall; roof/floor/door absent as categories. |
| Storage | [ ] | No storage building. |
| Workstation | [x] | Workbench placeable and functional (station gating works once compile error fixed). |
| Placement preview | [x] | Ghost actor + validity silhouette + ground trace (`BuildingComponent.cpp:59-97, 125-171, 211-219`). |
| Snap | [~] | 200cm grid snap (`cpp:125-153`); no piece-to-piece socket snapping. |
| Collision validation | [x] | Box overlap vs WorldStatic, client + server re-validation (`cpp:155-171, 258-266`). |
| Delete | [x] | **VERIFIED AT SOURCE (Batch 2 / `d5d23c2`).** `UAstrawildBuildingComponent::DismantleBuilding(AActor*)` is server-authoritative (`GetOwnerRole() == ROLE_Authority`), weight-safe (`CanAddItem` gate refuses when the bag is full), and refunds via `UAstrawildInventoryComponent::AddItemSilent` (structurally identical to `AddItem` minus the EventBus publish — dismantling materials do NOT advance `CollectItem` quest objectives). Bound to `EKeys::Z` on `AAstrawildPlayerCharacter` (`DeleteBuildingAction` + `DeleteBuilding` handler, 5 m crosshair trace via `FollowCamera + LineTraceSingleByChannel ECC_Visibility`). HUD toast routed through `AAstrawildPlayerController::Notify`. **Compile/playtest still pending on target machine.** |
| Repair | [ ] | None. |
| Save/load | [~] | Saved with transform/health/charge/switch (`SaveSubsystem.cpp:105-108`). **Building-health-reset bug (H-5) closed in Batch 1** — `FromSaveData` now restores health AFTER `InitializeFromDefinition` re-init. **Per-building power state (Item C) closed in Batch 2** — `FAstrawildBuildingSaveData.bIsPowered` captures `Power->IsBuildingPowered(this)` at save time; `FromSaveData` restores the hint; `UAstrawildSaveSubsystem::LoadWorld` then calls `UAstrawildPowerSubsystem::ResolveGridNow()` on the same frame so the first frame after load is correct. Grid-level `StoredEnergy` (battery state) still NOT saved (H-6 remainder, pending Batch 3). Compile/playtest still pending on target machine. |
| Ownership foundation | [!] | `OwnerPlayerId` never written by any code path. |

**Build mode blocker:** `CycleBuildingDefinition()` (`BuildingComponent.cpp:99`) is **never called** — the player can only place whichever unlocked building happens to be index 0 of a TMap iteration. **MP dupe vector:** materials are consumed client-side before the server RPC (`cpp:237`); the server only refunds on failure, never deducts.

## 11. P8 — Power / Automation

Implementation: `UAstrawildPowerSubsystem` (`.cpp` 218), building definitions with power roles.

| Item | Status | Evidence |
|---|---|---|
| Generator | [x] | "Echo Dynamo" 8/s (`ContentLibrary.cpp:331-336`), grid math real (`PowerSubsystem.cpp:53-58`). |
| Battery | [x] | "Charge Cell" 600 capacity; math real. |
| Power network | [~] | **One global grid** — `ResolveGrid` aggregates all registered buildings world-wide; documented 1200cm connectivity radius is only used in `IsLocationPowered` (`PowerSubsystem.cpp:68-111` vs `.h:13-15`). |
| Consumers | [~] | Draw registered per building; **only WorkSiteActor reacts to power** (`WorkSiteActor.cpp:50-58, 76-81`) — and work sites are dead (P5). Lamps draw power but emit no light logic. |
| Power priority | [x] | Category-sorted greedy resolve: Research > Workstation > Farm > Defense > Decoration (`cpp:115-135`). |
| Power failure | [x] | Brownout detection + `OnPowerStateChanged` broadcast (`cpp:137-174`). |
| Energy UI | [ ] | None. |
| First automation loop | [!] | **Unreachable** — work sites have no input path; the entire automation loop cannot run. |
| Persistence | [~] | **PARTIAL — Batch 2 Item C closed building power persistence:** `AAstrawildBuildingActor::bIsPowered` (UPROPERTY Replicated + DOREPLIFETIME) is written by `ResolveGrid` every 2 s tick and captured into `FAstrawildBuildingSaveData::bIsPowered` at save time. `UAstrawildSaveSubsystem::LoadWorld` calls `UAstrawildPowerSubsystem::ResolveGridNow()` after the building spawn loop so the first frame after load is correct (no brownout flicker). **Grid-level `StoredEnergy` (battery state) still NOT saved** — `H-6` remainder pending in Batch 3. `StoredCharge` per-building is still written-never-read (dead data). **Compile/playtest still pending on target machine.** |

**Cascade note:** Generator/Battery/LampPost all require `Tech_Electrical` (`ContentLibrary.cpp:370-371`) — with the research dead-end (P9), **the power system is unreachable in normal play**.

## 12. P9 — Advanced Technology

| Item | Status | Evidence |
|---|---|---|
| Technology tree | [!] | 6 nodes exist (`ContentLibrary.cpp:360-385`) with prerequisites + costs — but **no legitimate path calls `TryUnlockTech`**: only CheatManager (`CheatManager.cpp:141`) and quest `RewardTechId` (`QuestComponent.cpp:285`) — and **no quest in content sets `RewardTechId`** (verified: zero matches in ContentLibrary). No tech UI exists (`ASTRAWILD_RESEARCH_SYSTEM.md:113`). `Tech_BasicCrafting` (cost 0) isn't even auto-granted. |
| Research system | [x] | Points/prereqs/unlock/save all real (`ResearchSubsystem.cpp:22-118`). Point sources real: journal milestones (+2×4/species) + quest rewards (80 total). |
| Armor framework / all armor slots / Exosuit / modules | [ ] | Nothing exists. Shield/thermal/scanner "modules" exist only as adjacent primitives (shield item, weather temperature, journal scan) — not an equipment framework. |
| Laser/Plasma/Missile weapons / heat-energy model / mods | [ ] | Nothing exists. |
| Utility drone framework / one drone / robot framework / one robot | [ ] | Nothing exists. |
| Technology integrates with exploration | [ ] | Not yet — journal grants points but unlocks dead-end. |

## 13. P10 — World

Implementation: `AAstrawildWorldBootstrapper` (`.cpp` 348), `UAstrawildTimeSubsystem`, `UAstrawildWeatherSubsystem`, `UAstrawildEcosystemSubsystem`.

| Item | Status | Evidence |
|---|---|---|
| World Partition | [ ] | Single flat engine Plane at 160m×160m (`Bootstrapper.cpp:120-140`), comment admits "REPLACE_BEFORE_RELEASE". No WP setup. |
| Day/night | [x] | 24-min day, deterministic, replicated GameState, sun rotation + intensity, day rollover (`TimeSubsystem.cpp:45-88`, `Bootstrapper.cpp:320-339`). |
| Weather | [x] | 8 states, 90-in-world-min transitions, weighted + repeat penalty, temperature + capture effects real (`WeatherSubsystem.cpp:68-168`). **Dangling-ref UB** at `GetProfile` (`cpp:87` — `FindRef` by-value bound to `const&` return). Visibility multiplier has zero consumers. |
| Biome definition | [~] | 5 biome tags; one flat arena; no zones in code. |
| Resource spawning | [x] | 26 nodes, 3 charges each, 30s respawn (`Bootstrapper.cpp:155-172`, `ResourceNode.cpp:37-87`). **Not replicated** (`ResourceNode.cpp:13`) — MP clients see nothing. |
| Creature population | [~] | **PARTIAL — Batch 2 Item A closed the runtime-spawn population-clamp bug.** `UAstrawildHostileSpawnerSubsystem` (NEW `UTickableWorldSubsystem`, 25 s sweep, server-only) refills `Echo_Gloomfang` to `TargetGloomfangPopulation=4` and `Echo_Emberfang` to `TargetEmberfangPopulation=2` around the player pawn every 25 s. REVIEW-2 caught an init-ordering race (BeginPlay's `RegisterWithEcosystem` ran before `EchoDefinition` was set, so `WildCount` was never bumped — see §21); fixed inline by re-`RegisterEcho` after `InitializeFromDefinition`. `UnregisterEcho` still does not decrement `WildCount` (MEDIUM gap, pending Batch 3+). `Bootstrapper.cpp:227-243` still spawns the initial population once at world boot. **Compile/playtest still pending on target machine.** |
| Ecosystem tiers | [~] | Tier map + distance sweep + interval contract real (`cpp:201-236, 295-310`); Tier2/3 semantics (movement-disable, world model) not implemented — only interval stretching. |
| Migration foundation | [ ] | None. |
| Dynamic events | [ ] | EventBus exists with 11 event tags; zero world-event publishers. |
| Streaming/HLOD baseline | [ ] | None. |

## 14. P11 — Dawn Fields Vertical Slice

| Item | Status | Evidence |
|---|---|---|
| Starting area | [x] | Camp with rest point, 2 stations, 2 work sites, 2 NPCs (`Bootstrapper.cpp:258-307`). |
| 3–5 Echo | [x] | 7 species; 9 friendly + 2 hostile spawned (`cpp:186-242`). |
| Hostile creatures | [x] | Gloomfang/Emberfang + dungeon pool. |
| Resources | [x] | 26 nodes. |
| Base location | [x] | Flat arena supports building anywhere. |
| Crafting | [!] | Compile error in station path (P7) + no UI. |
| Research | [!] | Dead-end unlock (P9). |
| Advanced technology unlock | [!] | Unreachable (P9). |
| Dungeon | [~] | Generator + 5-room linear chain + encounters + clear rewards real (`DungeonGeneratorActor.cpp:70-135`) — **never completes**: entry room has no encounters and empty-encounter rooms early-out of the clear check (`DungeonRoomActor.cpp:118`) → `RoomsCleared` maxes at N−1 → `OnDungeonCompleted` unreachable (`Generator.cpp:142-146`). No gates (forward-declared only, `DungeonRoomActor.h:9`); dungeon state not saved. |
| Boss | [!] | `AAstrawildEchoBossCharacter` — 3 phases, enrage, adds, phase damage scaling, defeat cleanup — **fully coded but never spawned anywhere** (zero `SpawnActor<AAstrawildEchoBossCharacter>` in Source). The dungeon "boss" is a plain Gloomfang (`DungeonRoomActor.cpp:91-92`). Boss damage also bypasses player mitigation (`EchoBossCharacter.cpp:218-223`). |
| Main quest slice | [~] | 6-quest chain with 7 implemented objective types, event-driven tracking, auto-chaining, rewards (`QuestComponent.cpp:135-211, 236-289`) — chain code-reachable end-to-end after Batch 1 (C-2 unlock path). **Quest 5 "Defeat 3 Gloomfang" (`Quest_DawnGuard`) chain-completes organically after Batch 2 Item A landed:** `UAstrawildHostileSpawnerSubsystem` keeps `Echo_Gloomfang` population at 4 around the player; the existing death pipeline (`EchoCharacter::OnDefeated → EventBus TAG_Astrawild_Event_HostileDefeated → QuestComponent::ApplyEventToQuest`) auto-increments the kill counter — no new quest wiring was required. `ReachLocation`/`SurviveTime` objective types still unimplemented. Compile/playtest still pending on target machine. |
| Weather | [x] | See P10. |
| Day/night | [x] | See P10. |
| Save/load | [~] | Broad coverage (12 record types). **H-1/H-2/H-3/H-5 closed in Batch 1** — vitals restored on load, party respawns around the player, autosave-loadable "continue" path via `LoadLatest()`, building health preserved after `InitializeFromDefinition` re-init. **Item C closed in Batch 2** — building power state (per-actor) persists via `bIsPowered` and is re-resolved on the first frame after load. Gaps remaining: dungeon state still resets on load (M-7), autosave slot `ASTRAWILD_Auto` reachable via `LoadLatest()` but no load-on-boot setting on by default (`bAutoLoadLatestOnBeginPlay` default off). Compile/playtest still pending on target machine. |
| Full gameplay loop | [~] | **Code-reachable end-to-end without cheats after Batch 1 + Batch 2** (pending target-machine compile). The chain Start → gather → craft → build → research → power → work → dungeon → boss → save → reload → continue is wired; Quest 5 "Defeat 3 Gloomfang" completes via hostile respawn (Item A); base dismantle/refund (Item B); per-building power state persistence (Item C). Compile + runtime + save round-trip verification still required on the target machine before this flips to `[x]`. |

## 15. P12 — NPC / Story

| Item | Status | Evidence |
|---|---|---|
| NPC framework | [~] | `AAstrawildNPCCharacter` — definition-driven, interact starts offered quest (`NPCCharacter.cpp:47-67`). No AI, no schedule, no replication. |
| Dialogue | [ ] | None. |
| Schedule | [ ] | None. |
| Faction foundation | [ ] | 3 faction tags exist; no system. |
| Main quest chain | [~] | 6 quests (see P11); halts at 4. |
| Side quest framework | [ ] | Single active quest only (`QuestComponent.cpp:101-104`). |
| World state | [ ] | None beyond weather/time. |
| Lore discovery | [~] | Journal observation system (4 knowledge tiers, 20s per species). |

## 16. P13 — Multiplayer

| Item | Status | Evidence |
|---|---|---|
| Server authority rules | [~] | Attack/craft/place are server-RPC + re-validated. Eat/feed/equip/capture/party-command/save are client-executed with authority early-returns → **silently no-op for remote clients** (`PlayerCharacter.cpp:513, 552, 610, 653`; `CaptureComponent.cpp:67`). |
| Player replication | [~] | Movement only; no character state props. |
| Creature replication | [~] | 6 props replicated (personality/needs/XP/AIState/command/owner). **Health/Trust/Bond/Level/bCaptured not replicated** — client health bars/capture UI impossible. |
| Inventory authority | [~] | Items map replicated; equip unguarded; weight gate server-only. |
| Capture authority | [!] | No RPC — unreachable for clients. |
| Building authority | [~] | Placement RPC good; client-side material consumption = dupe vector. |
| Quest authority | [~] | Server-side tracking; objective progress never replicated to clients. |
| Shared world state | [x] | GameState 4 props replicated (the reference implementation, `GameState.cpp:11-18`). |
| Save authority | [~] | P1-only (`SaveSubsystem.cpp:66`). |
| Join/leave handling | [ ] | None. |
| 1–4 player co-op test | [ ] | Never tested; no dedicated server target file; `MaxPlayers=4` config only. |
| Additional: client registry | [!] | `BuildDefaults` skipped on NM_Client (`ItemRegistrySubsystem.cpp:19-22`) → **clients have empty item/species registries**. |
| Additional: client events | [!] | EventBus is process-local; clients never receive gameplay events. |

## 17. P14/P15/P16 — Expansion / Optimization / QA

- **P14 Content Expansion:** all `[ ]` — no additional biomes/dungeons/bosses. Expected at this stage.
- **P15 Optimization:** all `[ ]` — no profiling performed (no runnable build). Positive: AI is timer-based not per-frame; ecosystem LOD intervals exist; subsystem ticks throttled.
- **P16 QA/Release:** automated tests `[!]` (tautological majority); all runtime/save/regression/MP tests `[ ]`; placeholder asset audit `[~]` (all visuals are engine basic shapes, tracked as `REPLACE_BEFORE_RELEASE`); license manifest `[x]` (ThirdPartyLicenses.md exists); packaging `[ ]`.

## 18. GLOBAL QUALITY GATE

| Item | Status |
|---|---|
| No Critical issues | [!] — 8 Critical (see Gap Report) |
| No unresolved High issues affecting core loop | [!] |
| Build passes | [!] — does not compile (1 confirmed error; more latent errors possible — never compiled) |
| Game launches | [ ] — unverified (no engine in sandbox) |
| Core loop playable | [!] — blocked without cheats |
| Save/load reliable | [~] — broad but 6 defects |
| Documentation matches implementation | [~] — CREATURE_SYSTEM.md is one wave stale (7 species, herd wired); BUILD_STATUS.md honest |
| Git working tree clean | [x] — clean at audit time |

## 19. System Classification (KEEP / REFACTOR / REPLACE / REMOVE / ADD)

| System | Verdict | Rationale |
|---|---|---|
| WorldBootstrapper (zero-asset strategy) | **KEEP + EXTEND** | Genuinely enables playability; needs navmesh + dungeon/boss wiring |
| ContentLibrary (C++ defaults) | **KEEP** | Data-asset-compatible; counts hardcode drift risk only |
| SaveSubsystem v2 | **KEEP + FIX** | 70% production-grade; fix 6 defects |
| Echo AI (C++ FSM) | **KEEP + FIX** | Working design; fix state-broadcast, owner-target bug, navmesh dependency |
| Combat component | **KEEP** | Server-authoritative, extensible |
| Research subsystem | **KEEP + WIRE** | Logic complete; needs unlock caller (UI/quest) |
| Power subsystem | **KEEP + FIX** | Real math; fix persistence, connectivity, dead consumer logic |
| Building system | **KEEP + FIX** | Needs cycling input, delete, server-side material deduction |
| Work sites | **KEEP + WIRE** | Complete math; needs interactability |
| Dungeon generator | **KEEP + FIX** | Needs entry-room fix, boss spawn, completion consumer |
| EchoBossCharacter | **KEEP + SPAWN** | Fully coded, never used |
| Tautological tests | **REPLACE** | Write real tests |
| Runtime-built IMC | **KEEP** (add gamepad later) | Working zero-asset solution |
| Single-module architecture | **KEEP for slice** | Split modules only when content pipeline arrives |

## 20. Honest Build Status

- **Compile status: NOT RUN (no engine in this sandbox).** One confirmed compile error found by inspection; the codebase must be compiled on the target machine (Windows + UE 5.8) after fixing `CraftingStationActor.cpp:52` — additional latent errors are possible.
- **Runtime status: NOT RUN.** Runtime blockers identified by inspection: no navmesh (all AI movement), respawn input loss, research dead-end, dungeon completion, boss spawn, build-mode cycling.
- Nothing in this audit claims runtime verification. All `[x]` marks above mean "implementation exists with real logic and correct integration, verifiable by inspection" — they do **not** mean compiled-and-run.

---

## 21. Wave 4 Batch 2 status update (post-`d5d23c2`)

This audit was originally written at commit `0402472` (pre-Batch-1). Batch 1 (`7e2d0b4`) closed the 8
Critical gaps and the 6 listed High gaps (H-1, H-2, H-3, H-4, H-5, H-7, H-8) at source level — the audit
rows above were updated inline to reflect that where applicable, and the new compile-blocker include
(`Engine/StaticMeshActor.h` in `AstrawildWorldBootstrapper.cpp`) from REVIEW-1 was committed as `6f14520`.

Wave 4 Batch 2 (`d5d23c2`) closes three additional HIGH items at source level. None are runtime-verified
yet — the compile status remains `NOT RUN (sandbox has no UE engine — must be verified on target
machine)` per the strict rule that no system may be claimed complete or verified without in-engine
evidence.

| Item | Brief | Status | Note |
|---|---|---|---|
| **A — Hostile respawn** | `UAstrawildHostileSpawnerSubsystem` — `UTickableWorldSubsystem`, server-only Tick @ 25 s, `SpawnRadius=1800 cm` ring-biased outward 30–100 %, `FRandomStream` seeded from `WorldSeed` (deterministic SP), targets `Echo_Gloomfang=4` / `Echo_Emberfang=2` | `[x] VERIFIED AT SOURCE` | Implemented in commit `d5d23c2`. REVIEW-2 caught an init-ordering race (population clamp silently broken — see §22 below) and the fix landed inline. Quest 5 "Defeat 3 Gloomfang" chain-completes via the existing death pipeline (`OnDefeated → TAG_Astrawild_Event_HostileDefeated → QuestComponent::ApplyEventToQuest`) — no new quest wiring required. **Compile/playtest still pending on target machine.** |
| **B — Building dismantle** | `UAstrawildBuildingComponent::DismantleBuilding(AActor*)` — server-authoritative, weight-safe (refuses if `CanAddItem` fails), refunds via `UAstrawildInventoryComponent::AddItemSilent` (no false `TAG_Astrawild_Event_ItemCollected`); `AAstrawildPlayerCharacter::DeleteBuildingAction` bound to `EKeys::Z`, 5 m crosshair trace; HUD toast via `AAstrawildPlayerController::Notify` | `[x] VERIFIED AT SOURCE` | Implemented in commit `d5d23c2`. Input log line now reads "19 actions, WASD+mouse+wheel" (was 17). **Compile/playtest still pending on target machine.** |
| **C — Power persistence** | `FAstrawildBuildingSaveData.bIsPowered` (additive — no schema bump), `AAstrawildBuildingActor::bIsPowered` (`UPROPERTY Replicated` + `DOREPLIFETIME`), `UAstrawildPowerSubsystem::ResolveGridNow()` public wrapper for private `ResolveGrid()`, called by `UAstrawildSaveSubsystem::LoadWorld` right after the building spawn loop | `[x] VERIFIED AT SOURCE` | Implemented in commit `d5d23c2`. Old saves deserialize with `bIsPowered=false` and re-resolve within ≤2 s on the first `ResolveGridNow()` call (in practice the same frame, because `LoadWorld` calls it). Grid-level `StoredEnergy` (battery state) is NOT saved — `H-6` remainder pending in Batch 3. **Compile/playtest still pending on target machine.** |

### Batch 1 already-closed H-* items (verified at source)

These were closed at source level in commit `7e2d0b4`; their corresponding audit rows above were
updated inline to reflect the closure. They remain `compile-status: NOT RUN` until the target
machine verifies the build.

- `H-1` Survival vitals restored on load (`SetStatsForRestore`, clamp-safe)
- `H-2` Captured party respawns around the player on load (`SpawnPartyActors`); redundant double-roster import removed
- `H-3` `LoadLatest()` — F9 / cheat / optional boot-continue now load the NEWEST slot
- `H-4` Weather `GetProfile` dangling-reference UB → returns by value
- `H-5` Building damage persists across load (health applied AFTER definition re-init)
- `H-7` Captured "Attack" command can no longer target the owner (owner exclusion in target acquisition)
- `H-8` `OnAIStateChanged` now actually broadcasts (public `SetAIState`, controller routes through it)

---

## 22. REVIEW-2 finding — hostile-spawner registration race (caught and fixed inline)

REVIEW-2 (read-only compile-risk review of Batch 2) surfaced a **MEDIUM-risk runtime bug** in Item A
that the implementation brief did not anticipate. The fix landed in the same commit (`d5d23c2`) so
the production code carries the corrected version; this subsection is the historical record for
future audit pass reviewers.

### Symptom (would have manifested without the fix)

`UAstrawildHostileSpawnerSubsystem::Tick` would have called `Ecosystem->GetWildPopulation(GloomfangId)`
and `GetWildPopulation(EmberfangId)` every 25 s. Both would have returned 0 forever, even with hostile
Echoes visibly roaming the world — because of an init-ordering race between
`AAstrawildEchoCharacter::BeginPlay` and `UAstrawildHostileSpawnerSubsystem::SpawnOneHostile`'s call to
`Echo->InitializeFromDefinition(Definition)`.

### Root cause (init-ordering race)

1. `World->SpawnActor<AAstrawildEchoCharacter>(...)` triggers `BeginPlay` (`EchoCharacter.cpp:64-78`).
2. `BeginPlay` calls `RegisterWithEcosystem()` at line 68 BEFORE the actor's `EchoDefinition`
   UPROPERTY is set (`EchoCharacter.cpp:70-73` only re-runs `InitializeFromDefinition` if
   `EchoDefinition` was already externally assigned pre-spawn, which is not the case here).
3. `UAstrawildEcosystemSubsystem::RegisterEcho` (`EcosystemSubsystem.cpp:167-188`) at this point sees
   `Echo->EchoDefinition` as null → skips the `Population.WildCount += 1` block at lines 180-187
   (it still adds to `RegisteredEchoes` array + `EchoTiers`, so the actor is visible to the
   ecosystem — but its species population counter is not incremented).
4. `UAstrawildHostileSpawnerSubsystem::SpawnOneHostile` then calls
   `Echo->InitializeFromDefinition(Definition)` (line 132), which sets
   `EchoDefinition = InDefinition` (`EchoCharacter.cpp:133`) — but `InitializeFromDefinition` does
   NOT re-call `RegisterWithEcosystem`.
5. Net effect: spawned hostiles are tracked in `RegisteredEchoes` but their species `WildCount` is
   never incremented. `GetWildPopulation` returns 0 every Tick.
6. `Deficit = TargetGloomfangPopulation - 0 = 4` → spawns 4 Gloomfangs every
   `RespawnIntervalSeconds` (25 s). Same for Emberfang (+2 every 25 s). Indefinite population leak.
7. Quest 5 still completes (the death pipeline publishes `TAG_Astrawild_Event_HostileDefeated`
   based on `EchoDefinition->bHostileToPlayers`, independent of `WildCount` bookkeeping), but the
   world floods with hostile Echoes — performance degradation + stale AI actors piling up around
   the player's base camp.

### Fix (one-line, additive, scope-limited to Batch 2 only)

In `Source/AstrawildCore/Private/AstrawildHostileSpawnerSubsystem.cpp::SpawnOneHostile`, immediately
after line 132 (`Echo->InitializeFromDefinition(Definition);`), re-register the spawned Echo with the
ecosystem so the species `WildCount` bumps (now that `EchoDefinition` is set):

```cpp
if (UAstrawildEcosystemSubsystem* Eco = World->GetSubsystem<UAstrawildEcosystemSubsystem>())
{
    Eco->RegisterEcho(Echo);
}
```

The second `RegisterEcho` call hits the predicate check at `EcosystemSubsystem.cpp:174` (skips the
redundant `RegisteredEchoes.Add`), but the `WildCount` bump at lines 180-187 is OUTSIDE that
predicate check, so it now fires correctly. This fix is committed in `d5d23c2` — the production
code carries the corrected version.

### Note on `WorldBootstrapper.cpp:251`

`Echo->InitializeFromDefinition(HostileDef)` in the bootstrapper has the SAME timing pattern but is
unaffected because the bootstrapper only spawns ONCE in `BeginPlay` (no respawn cadence, no
`GetWildPopulation` clamp). The bug only manifested for the new tick-driven `HostileSpawner`.

### Residual LOW-risk items (REVIEW-2 nits — non-blocking)

- `AstrawildHostileSpawnerSubsystem.cpp:42,133` use `UE_LOG(LogAstrawildBuilding, ...)` for spawner
  logging — semantically odd (a spawner isn't a building). Consider `LogAstrawildAI` (already
  declared in `AstrawildLog.h:9`). No functional impact.
- `AstrawildHostileSpawnerSubsystem.cpp:39` — fallback `SpawnStream.Initialize(FMath::Rand())`
  when no GameState — non-deterministic across reloads (loses world-seed reproducibility). Defensive
  only; latent only if subsystem init order changes.
- `AstrawildHostileSpawnerSubsystem.cpp:57-62` — `RespawnAccumulator += DeltaTime` with no upper
  cap. If the game pauses (tab-out for hours), the accumulator grows large; the spawn sweep resets
  to 0 after the first fire, so no infinite loop, just one early double-spawn after a long pause. Fine.
- `AstrawildBuildingComponent.cpp:354-358` — `PublishEvent(TAG_Astrawild_Event_BuildingPlaced, Player,
  Def->DefinitionId, -1, ...)` uses `Amount=-1` to signal "building removed".
  `QuestComponent::ApplyEventToQuest` does not currently subscribe to `BuildingPlaced` for negative
  counts (only positive `Amount` for `PlaceBuilding` advancement), so no false quest advancement.
  The `-1` convention is documented in a comment for future quest authors.
- `AstrawildPlayerCharacter.cpp:763` — `FollowCamera->GetForwardVector() * 500.0f` reaches 5 m
  (500 cm = 5 m). Compile-clean as-is; consider bumping to 800-1000 cm for usability in a future
  round (out of scope for Batch 2).

---

## 23. Wave 5 Batch 3 status update (post-`021f93a`)

Batch 3 ("Depth — combat") closed the three gameplay-feel items scoped for this batch — status
effects, hit reactions (stagger), and the armor framework — plus the weapon-element-override MEDIUM
gap (the "player attack element hardcoded Ash" evidence in §6 above). Console gap-tracker IDs M5/M6/M7
(all marked DONE at `021f93a`) map to these three items. RESEARCH-3 scoped the work; REVIEW-3
(read-only compile-risk review) caught **1 HIGH + 2 MEDIUM** issues, all fixed in the same commit.
None are runtime-verified — compile status remains `NOT RUN (sandbox has no UE engine — must be
verified on UE 5.8 + Antigravity target machine)`.

| Item | Brief | Status | Note |
|---|---|---|---|
| **A — Status effects (element-driven)** | `UAstrawildCombatComponent::MakeElementalStatusEffect` static BlueprintPure factory — one shared element→status vocabulary: Ember→Burn (4 s DoT, DPS `2 + 5% × hit`), Frost→Chill (3 s, speed ×0.5), Flora→Poison (6 s, 2 DPS), Pulse→Shock (0.8 s, speed ×0.3); None/Light/Ash → nothing (`CombatComponent.cpp:327-368`). Creatures: replicated `StatusEffects` + server `ApplyStatusTicks` — DoT damages `CurrentHealth`, expiry removal, and **DoT defeats route through the FULL defeat pipeline** (loot/events/quest credit, `EchoCharacter.cpp:262-313`). Players: `SurvivalComponent` statuses + `GetStatusSpeedMultiplier` (multiplicative; Chill+Shock would stack to 0.15) consumed by `RefreshMovementSpeed`. Application hooks: player weapon hits (`ApplyElementalDamage`, element ≠ None) + creature attacks on the player (`EchoAIController::TryAttackTarget` player branch). New tags `Status.Chilled/Shocked/Staggered` | `[x] VERIFIED AT SOURCE (021f93a — compile pending)` | Closes gap-report M-1 (status system dormant) and Ultimate Gap §3 "Status effects dormant" + §8 "Status effects in combat". **Statuses are NOT persisted** — transient combat state (grep-verified: zero StatusEffect references in SaveSubsystem); deliberate decision. Duskmoth is **Flora** element (poisons) — it is weak TO Frost; no Frost-element species or weapon exists in content yet, so Chill currently has no content source (a Frost weapon/species is future content) |
| **B — Hit reactions (stagger, zero-asset)** | `EAstrawildEchoAIState::Staggered` appended AFTER `Dead` (serialization-safe, `Types.h:207`). `AAstrawildEchoCharacter::ApplyStagger` — server-only, clamped ≤ 2 s, zeroes `MaxWalkSpeed` + `SetAIState(Staggered)`; expiry in Tick restores state AND speed. Triggers: Echo hit ≥ 20% of `GetMaxHealth()` → 0.8 s; player hit ≥ 35 mitigated (`StaggerDamageThreshold`) → 0.6 s (`PlayerStaggerSeconds`); boss landed hits ALWAYS stagger (`EchoBossCharacter.cpp:229-235`). `AIController::Think` stagger gate: `StopMovement` + skip Decide/Execute, think timer re-arm UNCONDITIONAL (naive early-return would kill the AI loop). Player side: `CombatComponent::StaggerRemainingSeconds` + `ApplyStagger` + `OnStaggerStateChanged` + Tick countdown; `RefreshMovementSpeed` zeroes while `IsStaggering`. New tag `State.Creature.Staggered` | `[x] VERIFIED AT SOURCE (021f93a — compile pending)` | Closes Ultimate Gap §3 "Hit reactions/stagger" + §8 "Hit reaction/stagger". REVIEW-3 M-1 (stagger freeze) caught + fixed inline — see §23.1 below |
| **C — Armor framework (3 tiers, diminishing returns)** | `UAstrawildItemDefinition::ArmorRating` new field. `UAstrawildInventoryComponent`: `EquippedArmorItemId` (Replicated + DOREPLIFETIME — 28th replicated prop overall) + routing branch (before the statless fallback) + `GetEquippedArmorRating` + `GetEquippedWeaponElement` + Unequip clears + additive `OnArmorChanged` delegate (2-param `OnEquipmentChanged` signature unchanged for BP stability). `UAstrawildCombatComponent`: static pure `ComputeArmorFraction(Rating, K, MaxFraction) = Rating/(Rating+K)` clamped; tunables `ArmorConstantK=100` / `ArmorMaxFraction=0.6`; `GetMitigatedIncomingDamage` multiplies by `(1−armor)` AFTER dodge/block (dodge still fully avoids, block still reduces first). Save: additive `EquippedArmorId` (no schema bump) + `HasItem`-guarded restore. Content: Fiberweave Vest 20 / Emberhide Jacket 45 / Crystalplate Cuirass 80 (16.7 / 31.0 / 44.4% reduction) + 3 recipes (4/6/9 s, `Tech_Armory` + workbench — `Tech_Armory` now unlocks 5 recipes). HUD `EquipmentText` armor segment; `EquipBest` picks the best armor | `[x] VERIFIED AT SOURCE (021f93a — compile pending)` | Closes Ultimate Gap §5 "Armor framework" row for the torso slot (helmet/arms/legs/boots/core slots remain P9 checklist items). Items 19→22, recipes 10→13. Tests: `+ASTRAWILD.Equipment.ArmorMath` + `+ASTRAWILD.Combat.StatusEffectFactory` — both call production statics (gap-report L-1 now PARTIAL: 2 real tests added; 11 total) |

### 23.1 REVIEW-3 findings (all fixed inline in `021f93a`)

- **H-1 (HIGH — compile blocker, pre-existing since wave 3 `2eeedf8`):** `AstrawildHudWidget.cpp` calls
  members on `UAstrawildInventoryComponent` (`EquippedItemId` / `GetEquippedWeaponAttackPower()` /
  `EquippedShieldItemId` — plus the new Batch-3 `EquippedArmorItemId` / `GetEquippedArmorRating()`)
  while the type was only forward-declared in that translation unit (MSVC C2027/C2079). The include
  closure was audited: no header in the module provides the full class. Fix: added
  `#include "AstrawildInventoryComponent.h"` at `AstrawildHudWidget.cpp:10`. REVIEW-1/REVIEW-2 missed
  it because the offending block predates both batches and the project has never been compiled
  in-sandbox.
- **M-1 (MEDIUM — runtime):** Echo stagger expiry only ran `SetAIState(Idle)`; the Tick speed
  recompute is gated on the STATUS multiplier changing, and stagger does not touch it — with no speed
  status active (the common case, default Ash element), `MaxWalkSpeed` stayed 0 permanently: every
  heavy hit ≥ 20% max HP (trivially triggered, e.g. 74-damage heavy vs a 60-HP creature) permanently
  froze the creature. Fix: the expiry branch explicitly restores
  `Movement->MaxWalkSpeed = CachedStats.MoveSpeed * GetStatusSpeedMultiplier()`
  (`EchoCharacter.cpp:106-119`).
- **M-2 (MEDIUM — runtime):** `SurvivalComponent::FullRestore` / `SetStatsForRestore` called
  `StatusEffects.Reset()` without broadcasting `OnStatusEffectRemoved` — a Chilled/Shocked player kept
  the stale halved speed after resting at a RestPoint, after QuickLoad, or the `AW.FullRestore` cheat.
  Fix: both paths broadcast removal per effect BEFORE `Reset()` (`SurvivalComponent.cpp:192-194, 221-225`).
- LOW-risk residual notes (non-blocking): `CombatComponent` stagger countdown ticks on clients too
  (harmless — server-only state, consistent with the dodge i-frame pattern); player attacks vs the
  Boss bypass the element pipeline (bosses cannot Burn/Chill — design choice); HUD shows
  "Unarmed (0)" for an empty armor slot (cosmetic); stagger restore always lands on `Idle` regardless
  of pre-stagger state (AI re-decides next Think — acceptable).

### 23.2 Test-suite status

`ASTRAWILD.Equipment.ArmorMath` (tier fractions 20/45/80 → 16.7/31.0/44.4%, diminishing-returns
ordering, 1,000,000-rating clamp to 0.6, K=0 degenerate case, block+cuirass ≈ 19.4 of a 100 hit) and
`ASTRAWILD.Combat.StatusEffectFactory` (all 4 element mappings + None/Light/Ash no-op) both exercise
the REAL production static functions — replacing tautological assertions. A pre-existing float-unsafe
`TestEqual` was converted to a tolerance-based `TestTrue`. **11 automation tests total** — 4 call
production code, 7 remain tautological (gap-report L-1 remainder → Batch 4).
