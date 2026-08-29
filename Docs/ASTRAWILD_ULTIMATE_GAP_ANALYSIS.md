# ASTRAWILD — ULTIMATE GAP ANALYSIS

**Date:** 2026-08-30
**Compared against:** `ASTRAWILD_ULTIMATE_PRODUCTION_ROADMAP_V3.md` (long-term), `ASTRAWILD_PRODUCTION_MASTER_PLAN_V2.md` + `ASTRAWILD_PRODUCTION_CHECKLIST_V2.md` (source of truth)
**Evidence base:** `ASTRAWILD_UE5_PRODUCTION_AUDIT.md`, `ASTRAWILD_IMPLEMENTATION_GAP_REPORT.md`
**Constraint honored:** "ห้ามสร้าง content จำนวนมากก่อน architecture ผ่าน" — this analysis prioritizes architecture and loop integrity over content volume. Vertical Slice A (Roadmap §39) is the target shape.

---

## 1. CRITICAL FOUNDATION GAPS

### GAP-F1 — Project has never compiled (build pipeline absent)
- **Current State:** Source-complete, compile status NOT_RUN. 1 confirmed compile error (`AstrawildCraftingActor.cpp:52` — see audit C-1). No CI, no build report, no engine in dev sandbox.
- **Required State:** Repeatable compile on target machine (Windows + UE 5.8); build report maintained; errors fixed at source (never by deleting systems).
- **Missing Implementation:** Fix of C-1; a documented build procedure; first-build error triage.
- **Dependencies:** None — everything depends on this.
- **Files likely affected:** `Source/AstrawildCore/Private/AstrawildCraftingStationActor.cpp`, potentially other latent-error files discovered at first compile.
- **Acceptance Criteria:** `ASTRAWILD.uproject` compiles with zero errors in UE 5.8 editor (target machine); `BUILD_STATUS.md` updated with real result.
- **Test Method:** Engine build + `Scripts/validate_repository.sh`.
- **Priority:** **CRITICAL — Batch 1, item 1.**

### GAP-F2 — No navigation mesh (all AI movement fails at runtime)
- **Current State:** Zero navmesh references in Source; bootstrapper builds a flat ground plane only. All `MoveTo*` pathing will fail.
- **Required State:** Runtime-generated Recast navmesh covering Dawn Fields arena so Echo/Echo AI locomotion works without authored assets.
- **Missing Implementation:** `ANavMeshBoundsVolume` spawn + `FNavigationSystem` build in `AAstrawildWorldBootstrapper`.
- **Dependencies:** GAP-F1 (compile), WorldBootstrapper exists.
- **Files likely affected:** `AstrawildWorldBootstrapper.h/.cpp` (add navmesh section), possibly `Config/DefaultEngine.ini` (nav system settings).
- **Acceptance Criteria:** In PIE, a wild Echo wanders (position changes over time); hostile chases the player; captured Echo follows.
- **Test Method:** PIE observation + `AW.SpawnEcho` + AI state logs; automation: assert navmesh actor exists post-boot.
- **Priority:** **CRITICAL — Batch 1, item 2.**

### GAP-F3 — Respawn input death (death loop broken)
- **Current State:** Pawn BeginPlay early-returns before IMC setup when unpossessed at spawn (`PlayerCharacter.cpp:122-133`); respawned pawn likely has no input.
- **Required State:** Input survives death/respawn for all local players.
- **Missing Implementation:** IMC (re)binding on possess/restart (e.g., in `HandleRespawn` or via `OnActorRestarted`).
- **Dependencies:** None.
- **Files likely affected:** `AstrawildPlayerCharacter.cpp` (BeginPlay/HandleRespawn), `AstrawildGameMode.cpp` (RespawnPlayer ordering).
- **Acceptance Criteria:** Die → respawn → full keyboard/mouse control retained.
- **Test Method:** PIE: die (cheat damage) → respawn → move/attack/interact.
- **Priority:** **CRITICAL — Batch 1, item 3.**

### GAP-F4 — Research/technology unlock dead-end
- **Current State:** `TryUnlockTech` has no legitimate caller (cheats only; no quest sets RewardTechId; no UI; root tech never granted).
- **Required State:** Player can spend research points in normal play; root tech auto-granted; tech gates (recipes/buildings/quests 4-6) reachable.
- **Missing Implementation:** (a) auto-grant `Tech_BasicCrafting` at session start; (b) Research Desk building interact → unlock available tech (points spent server-side); (c) HUD feedback.
- **Dependencies:** Research subsystem (complete), Research Desk building definition (exists), building placement (works).
- **Files likely affected:** `AstrawildResearchSubsystem.cpp`, `AstrawildBuildingActor.cpp` (or new ResearchDesk interact branch), `AstrawildGameMode.cpp` or PlayerController (auto-grant), `AstrawildHudWidget.cpp` (points display).
- **Acceptance Criteria:** New session → BasicCrafting known; journal to 100% on a species (+8 pts) + quest rewards → can unlock Cooking at the Research Desk; `Recipe_CookedMeat` becomes craftable.
- **Test Method:** PIE without cheats: observe 1 species fully, complete quests 1-2, interact Research Desk, unlock Cooking, cook meat.
- **Priority:** **CRITICAL — Batch 1, item 4.**

### GAP-F5 — Save/continue loop defects (6 issues bundled)
- **Current State:** Vitals saved-not-restored; party not respawned on load; roster double-import; autosave slot never loaded; no load-on-boot; building health resets.
- **Required State:** Save → quit → reload → world matches (player, vitals, party, buildings, research, quests, journal); continue path exists.
- **Missing Implementation:** `LoadWorld` restore-vitals; party respawn from roster; single import; `LoadLatest()` helper (autosave-if-newer logic); load-on-boot or main-menu continue hook; `FromSaveData` health preservation.
- **Dependencies:** GAP-F1.
- **Files likely affected:** `AstrawildSaveSubsystem.cpp` (primary), `AstrawildBuildingActor.cpp`, `AstrawildEchoRosterSubsystem.cpp`, `AstrawildGameMode.cpp`.
- **Acceptance Criteria:** F5 save with damaged vitals + captured party + damaged building → F9 → vitals/party/building state match pre-save.
- **Test Method:** PIE scripted sequence + future automation save round-trip test.
- **Priority:** **CRITICAL — Batch 1, item 9.**

## 2. HIGH PRIORITY GAPS

### GAP-H1 — Dungeon completion + boss integration
- **Current State:** Generator + rooms + encounters + clear rewards real; entry room can never clear → completion event unreachable; phased boss class fully coded but never spawned; boss damage bypasses player mitigation; completion event has no consumers; dungeon state not saved.
- **Required State:** Full dungeon run: enter → clear rooms → phased boss → completion event → rewards; state persists.
- **Missing Implementation:** Empty-room auto-clear; boss room spawns `AAstrawildEchoBossCharacter`; damage routed through mitigation; `OnDungeonCompleted` consumer (quest objective/loot/RP); dungeon save record (or documented regeneration policy).
- **Dependencies:** GAP-F1, GAP-F2 (boss uses movement), combat component (exists).
- **Files likely affected:** `AstrawildDungeonGeneratorActor.cpp`, `AstrawildDungeonRoomActor.cpp`, `AstrawildEchoBossCharacter.cpp`, `AstrawildQuestComponent.cpp`, `AstrawildSaveSubsystem.cpp`.
- **Acceptance Criteria:** PIE: walk dungeon, all rooms clearable, boss fight transitions phases at 66%/33%, defeat → completion event fires → reward granted.
- **Test Method:** PIE run with combat logging; verify phase transitions + completion event + loot.
- **Priority:** **HIGH — Batch 1, items 7-8.**

### GAP-H2 — Base building UX incompleteness
- **Current State:** Placement/preview/validation/snap(save)/server-RPC all real; but piece cycling never callable (only 1 arbitrary placeable), no delete, no repair, materials deducted client-side (MP dupe), ownership never populated.
- **Required State:** Player selects among unlocked pieces; can delete (with partial refund); ownership recorded (foundation for MP permissions).
- **Missing Implementation:** Cycle input + HUD piece name; delete mode (X?) with refund; server-side material deduction; `OwnerPlayerId` population at place.
- **Dependencies:** GAP-F4 (tech gates buildings).
- **Files likely affected:** `AstrawildBuildingComponent.h/.cpp`, `AstrawildPlayerCharacter.cpp` (input), `AstrawildHudWidget.cpp`.
- **Acceptance Criteria:** PIE: B → cycle pieces → place foundation, wall, workbench; delete wall → partial materials back; save/load preserves all.
- **Test Method:** PIE interaction test + save round-trip.
- **Priority:** **HIGH — Batch 1, item 5 (cycling only); delete/refund Batch 2.**

### GAP-H3 — Work/automation unreachable
- **Current State:** Work sites fully coded (rate = affinity × personality × mood × energy × power) but zero callers; not interactable; collect never invoked; power multiplier consumer only.
- **Required State:** E on site collects output; assignment path exists (interact assigns nearest idle captured Echo, or command target); first automation loop runs (Echo → site → items → collect).
- **Missing Implementation:** `IAstrawildInteractable` on `AAstrawildWorkSiteActor`; assign/collect logic in Interact; HUD prompt.
- **Dependencies:** GAP-F2 (Echo must path to site), capture system (works).
- **Files likely affected:** `AstrawildWorkSiteActor.h/.cpp`.
- **Acceptance Criteria:** PIE: capture Lumewisp → assign to Gathering site → wait → E → fiber in inventory; unpowered site produces at ×1/×0 per definition.
- **Test Method:** PIE timed observation (site tick logs) + inventory assert.
- **Priority:** **HIGH — Batch 1, item 6.**

### GAP-H4 — Gamepad support absent
- **Current State:** Runtime IMC is keyboard/mouse only (17 actions, no gamepad bindings).
- **Required State:** Checklist P2 "Controller support" — full pad mapping in the runtime-built IMC.
- **Missing Implementation:** Gamepad key mappings (Gamepad_Left2D/Right2D, face buttons) mirroring the 17 actions.
- **Dependencies:** None.
- **Files likely affected:** `AstrawildPlayerCharacter.cpp` (`BuildRuntimeInputDefaults`).
- **Acceptance Criteria:** PIE with pad: move/camera/sprint/jump/interact/attack/dodge/build all respond.
- **Test Method:** PIE with controller; `EnhancedInput` debug print.
- **Priority:** HIGH — Batch 2.

### GAP-H5 — Quest chain integrity
- **Current State:** 6 quests, 7/9 objective types implemented, event-driven; chain halts at quest 4 until GAP-F4; `ReachLocation`/`SurviveTime` unimplemented + their events never published; hostile respawn absent (quest 5 "defeat 3 Gloomfang" strained: 1 wild + dungeon pool).
- **Required State:** All 6 quests completable without cheats; all objective types in content implemented; hostiles respawn (ecosystem or timer).
- **Missing Implementation:** After F4: publish `Event.LocationReached` (dungeon entry trigger), implement SurviveTime objective or remove from content; hostile respawn (simple timer-based world respawn respecting ecosystem counts).
- **Dependencies:** GAP-F4 (chain), GAP-H1 (dungeon objectives).
- **Files likely affected:** `AstrawildQuestComponent.cpp`, `AstrawildEcosystemSubsystem.cpp`, `AstrawildWorldBootstrapper.cpp`, `AstrawildGameplayTags.cpp` (if new tags).
- **Acceptance Criteria:** Full chain FirstLight→ShepherdsDawn completes in one PIE session without cheats.
- **Test Method:** PIE full-chain run; quest logs at each completion.
- **Priority:** HIGH — Batch 2.

## 3. GAMEPLAY GAPS (loop-feel systems)

| Gap | Current State | Required State | Missing Implementation | Dependencies | Files | Acceptance | Test | Priority |
|---|---|---|---|---|---|---|---|---|
| Status effects dormant | API complete, zero callers | Poison/burn from combat & environment; speed multiplier consumed | Apply-on-hit hooks (Echo attacks, element rules); movement integration | Element system (exists) | `AstrawildCombatComponent.cpp`, `AstrawildEchoAIController.cpp`, `AstrawildSurvivalComponent.cpp`, `PlayerCharacter RefreshMovementSpeed` | Ember hit applies burn DoT; frost slows | PIE combat + stat assert | MEDIUM (Batch 3) |
| Sprint stamina drain | Gate-only, no drain | Sprint consumes stamina, exhausts to walk | Drain in tick while sprint flag active | Survival (exists) | `PlayerCharacter.cpp`/`SurvivalComponent.cpp` | Sprint until stamina <5% → forced walk | PIE timed run | MEDIUM |
| Block movement penalty dead | Never refreshed on toggle | Blocking slows movement | Call `RefreshMovementSpeed` on block change | None | `PlayerCharacter.cpp` | Block → speed ×0.55 | PIE | MEDIUM (fold into Batch 1 fixes if trivial) |
| Hit reactions/stagger | None | Enemies react on hit (flinch/stagger); player knockback | Stagger state in Echo FSM + interrupt; montage hooks (animation-free version = state + scale flash) | GAP-F2 | `AstrawildEchoCharacter.cpp`, `AstrawildEchoAIController.cpp` | Echo briefly stops moving when hit | PIE | MEDIUM (Batch 3) |
| Capture feedback | Result delegate has no UI subscriber | HUD/toast on capture attempt/result; cooldown shown before resonator waste | HUD hook + cooldown check ordering fix | HUD (exists) | `AstrawildHudWidget.cpp`, `AstrawildCaptureComponent.cpp` | Failed roll shows message; no resonator burn when out | PIE | MEDIUM |
| Ranged combat | None (melee only) | At least one ranged option before advanced tech (per V2 "bows if fit fiction") — or documented skip to energy weapons | Projectile actor + fire input + replication | GAP-F1 | New `AstrawildProjectileActor`, `CombatComponent` | Hit a target at range server-authoritatively | PIE | MEDIUM (Batch 3+, or defer to energy weapon) |

## 4. CREATURE GAPS (Echo platform vs Roadmap §7)

| Gap | Current State | Required State | Missing Implementation | Dependencies | Files | Acceptance | Test | Priority |
|---|---|---|---|---|---|---|---|---|
| Utility role unreachable | Work math real, no input path | First Complete Echo has working utility role | GAP-H3 fix | H3 | `WorkSiteActor` | Batch 1 item 6 | — | **HIGH (Batch 1)** |
| Traits/memories absent | Personality/needs/trust/bond only | Instance traits (per-species pool), simple memory (last fed/fought) for relationship texture | Trait struct + roll on capture + modifiers | Echo save v2 (extend) | `AstrawildTypes.h`, `EchoCharacter`, save | Captured Echo has 1-2 traits affecting stats | PIE + save round-trip | LOW (post-slice) |
| Abilities unused | `AbilityIds` field dormant | ≥1 active creature ability (heal/taunt/burst) usable in combat | Ability execution hook + cooldown on command | Combat (exists) | `EchoCharacter`, command path | Commanded Echo uses ability with effect | PIE | LOW (post-slice) |
| Population model inert | Counters broken for spawned Echoes; no respawn; no depletion consequences | Working counts, respawn, population pressure at Tier 3 | Register-after-init fix; unregister decrement; respawn timer | Ecosystem (exists) | `EcosystemSubsystem.cpp`, spawn ordering | Wild counts match world; Gloomfang respawns after N minutes | PIE + log assert | **HIGH (Batch 2)** |
| Not-replicated creature state | Health/trust/bond/level/bCaptured local-only | Clients see creature health/captured state | Extend replication + OnRep broadcasts | MP milestone | `EchoCharacter.cpp` | Client HUD reads Echo health | 2-PIE test | MEDIUM (MP batch) |
| AI state delegate dead | `OnAIStateChanged` never fires | UI/audio react to state changes | Route through `SetAIState` | None | `EchoAIController.cpp` | Subscriber receives changes | PIE log | MEDIUM (fold into Batch 1 H-8) |
| Attack-owner bug | Commanded Echo can target player | Owner excluded from party target acquisition | Filter owner in fallback acquisition | None | `EchoAIController.cpp:357-363` | Attack command never damages owner | PIE | **HIGH (Batch 1 quick fix)** |

## 5. TECHNOLOGY GAPS (P9 Advanced Technology vs Roadmap §4-6)

| Gap | Current State | Required State (Vertical Slice A) | Missing Implementation | Dependencies | Files | Acceptance | Test | Priority |
|---|---|---|---|---|---|---|---|---|
| Tech unlock entry point | Dead-end (GAP-F4) | Research Desk interaction unlocks techs | Batch 1 item 4 | — | — | — | — | **CRITICAL (Batch 1)** |
| Armor framework | Nothing | Modular slots (helmet/chest/arms/legs/boots/core) + stats + module slots | `UAstrawildArmorDefinition` + `FAstrawildEquipmentSlot` extension + equip UI path | Inventory equipment slots (2 today) | `AstrawildDataAssets.h`, `InventoryComponent`, content | Equip 2 armor pieces with stat effect; save/load | PIE + save | MEDIUM (Batch 3) |
| Energy weapon #1 (laser) | Nothing | One laser weapon: energy-cell ammo, overheat model, ranged projectile | Energy ammo item + weapon definition + `AstrawildProjectileActor` + heat gate | Ranged combat gap; power system (charge at base) | New files + `CombatComponent` | Laser fires 20 shots, overheats, recharges at powered base | PIE | MEDIUM (Batch 3-4) |
| Drone framework + 1 drone | Nothing | Utility drone: scout/scan/mark resources, battery, follow/hold | `AAstrawildDroneCharacter` + command states + battery drain + scan pulse (reveal nearest nodes) | Tech tree node (Tech_Drones); navmesh F2 | New files | Deployed drone marks 3 nearest resource nodes for 30s; battery drains; returns | PIE | LOW (Batch 4+) |
| Robot framework + 1 robot | Nothing | Worker robot: hauls work-site output to storage automatically | `AAstrawildRobotCharacter` + patrol/haul states + power maintenance | H3 work sites; storage building (missing) | New files | Robot moves output from site to storage without player | PIE timed | LOW (Batch 5+) |
| Weapon mod framework | Nothing | Attachment slot altering stats | Mod definition + apply path | Weapon framework | DataAssets | Mod changes damage/heat | PIE | LOW (post-slice) |
| Tech_AdvancedEnergy dead node | Unlocks nothing | Gates laser/drone or is removed | Content update | Energy weapon work | `ContentLibrary.cpp` | Node unlock visibly enables recipe/building | Content test | MEDIUM |

## 6. WORLD GAPS (P10 vs Roadmap §10-12)

| Gap | Current State | Required State | Missing | Dependencies | Files | Acceptance | Test | Priority |
|---|---|---|---|---|---|---|---|---|
| World Partition/HLOD | Flat plane, comment REPLACE_BEFORE_RELEASE | WP-enabled map with streaming for Dawn Fields at minimum | Project map + WP setup (editor-authored, target machine) | Engine access (user machine) | New Content map, `DefaultEngine.ini` | Map opens, streams cells, bootstrapper still applies | PIE travel | HIGH (requires target machine) |
| Biome/zoning | One flat arena | Dawn Fields zones (meadow/lakeshore/ruin edge) with distinct spawn tables | Zone struct + per-zone spawn tables in bootstrapper | None (can stay procedural) | `WorldBootstrapper.cpp`, zone data | Different zones spawn different species mixes | Log assert per zone | MEDIUM (Batch 3) |
| Weather visibility effect | Multiplier computed, zero consumers | Fog/storm reduces AI sight + journal range | Perception range scaling + journal range scaling by multiplier | Weather (exists) | `EchoAIController.cpp`, `JournalSubsystem.cpp` | Fog halves perception distance | PIE stat | MEDIUM |
| World events | EventBus + 11 tags, no world publishers | ≥1 dynamic event (migration/Storm/anomaly) observable | Event scheduler + publisher + HUD notice | Time/Weather (exist) | New `AstrawildWorldEventScheduler` or in TimeSubsystem | Storm event changes weather + spawns anomaly echo | Timed PIE | LOW (Batch 4) |
| Ecosystem Tier 2/3 semantics | Interval-stretch only | Tier2 movement-simplified; Tier3 statistical population model | Distant Echo despawn-to-record + respawn-on-approach | Population model fix (H) | `EcosystemSubsystem` | 200m+ Echo stop rendering but population persists | PIE + log | MEDIUM (Batch 4) |
| Resource node replication | Not replicated | MP clients see/harvest nodes | `SetReplicates(true)` + harvest RPC | MP milestone | `ResourceNode.cpp` | 2-PIE harvest sync | 2-PIE | MEDIUM (MP batch) |

## 7. BASE/AUTOMATION GAPS (P7-P8 vs Roadmap §14-15)

| Gap | Current State | Required | Missing | Dependencies | Files | Acceptance | Test | Priority |
|---|---|---|---|---|---|---|---|---|
| Piece cycling | Never callable | Cycle unlocked pieces | Input + HUD (Batch 1 item 5) | F4 | BuildingComponent | Cycle visible in HUD | PIE | **CRITICAL (Batch 1)** |
| Delete/refund | None | Delete placed piece, partial refund | Delete mode + refund + server authority | H2 | BuildingComponent | Delete returns 50% mats | PIE + inventory assert | HIGH (Batch 2) |
| Storage building | None | Storage container building with deposit/withdraw | Storage definition + container component + interact UI-lite | Inventory transfer API (missing) | New `StorageBuildingActor` | Store/retrieve stacks; save/load | PIE + save | MEDIUM (Batch 3) |
| Power connectivity | Global grid | 1200cm proximity networks | Graph clustering in ResolveGrid | Power (exists) | `PowerSubsystem.cpp` | Two distant bases have independent grids | Log assert | MEDIUM |
| Power persistence | StoredEnergy lost | Grid charge saved | Save record for grid(s) | Save schema bump | `SaveSubsystem`, `PowerSubsystem` | Battery charge survives reload | Save round-trip | HIGH (Batch 2) |
| Lamp consumer behavior | Draws power, no light logic | Powered lamp emits light (intensity toggle) | Point-light child + on/off by power state | Power events (exist) | `BuildingActor.cpp` | Lamp lights when powered, dark in brownout | PIE night | MEDIUM (Batch 3) |
| Automation loop | Dead sites | Echo works site → storage fills → robot hauls (full loop later) | H3 fix first; robot later | H3 | WorkSite | Batch 1 item 6 | — | **CRITICAL (Batch 1) → LOW (robot)** |
| Farming | FarmPlot/feed-trough definitions only | Crops grow in plots, trough feeds party automatically | Growth component + trough feeding trigger | Time (exists) | New `FarmPlotActor` logic | Berry plot yields in X in-world hours | Timed PIE | LOW (Batch 4) |

## 8. COMBAT GAPS (P4 vs Roadmap §5, §20-21)

| Gap | Current State | Required | Missing | Dependencies | Files | Acceptance | Test | Priority |
|---|---|---|---|---|---|---|---|---|
| Boss integration | Class exists, never spawned | Boss fight reachable (Batch 1 item 8) | Spawn + mitigation routing | F1/F2 | DungeonRoomActor, EchoBossCharacter | Phased fight completable | PIE | **CRITICAL (Batch 1)** |
| Hit reaction/stagger | None | Flinch/stagger on hit | FSM interrupt + brief state lock | F2 | Echo AI/Character | Hit Echo pauses movement 0.5s | PIE | MEDIUM (Batch 3) |
| Status effects in combat | Dormant | Elemental status application | On-hit apply (see Gameplay) | Element system | Combat, Survival | Ember→burn, Frost→slow | PIE | MEDIUM (Batch 3) |
| Weak points | None | Boss weak point (e.g., core when exposed in phase 2) ×2 damage | Hit-zone check (bone/mesh section or distance-to-core) | Boss integration | EchoBossCharacter | Hitting core doubles damage | PIE | LOW (Batch 4) |
| Telegraphs | None (cooldown-only swings) | Wind-up visual (color/scale pulse — asset-free) | Telegraph state with scale/color change before swing | Boss integration | EchoBossCharacter | Player can dodge after telegraph | PIE | MEDIUM (Batch 3) |
| Ranged/energy weapons | None | See Technology section | Projectile framework | — | New files | — | — | MEDIUM (Batch 3) |
| Dodge client prediction | Server-only (round-trip latency) | Local prediction + server reconcile | Client-side launch + server confirm | MP milestone | CombatComponent | Remote dodge looks instant | Net PIE | LOW (MP batch) |

## 9. MULTIPLAYER GAPS (P13 vs Roadmap §27, §36)

| Gap | Current State | Required | Missing | Dependencies | Files | Acceptance | Test | Priority |
|---|---|---|---|---|---|---|---|---|
| Client registry empty | BuildDefaults skipped on NM_Client | Clients can resolve items/species | Client-side defaults build (registry is static data — safe) | None | `ItemRegistrySubsystem.cpp` | Client `FindItem` non-null | 2-PIE | **HIGH (MP batch 1)** |
| Non-RPC gameplay actions | eat/feed/equip/capture/command authority-guarded only | Server RPCs for all mutating player actions | Server RPC layer + client prediction hooks | None (architecture ready) | PlayerCharacter, CaptureComponent | Remote client captures an Echo | 2-PIE | **HIGH (MP batch 1)** |
| Creature state replication | 6 props; health/trust/bond/level/bCaptured missing | Full UI-relevant state replicated | Replicate + OnRep | — | EchoCharacter | Client sees Echo HP bar | 2-PIE | HIGH (MP batch 1) |
| EventBus client reach | Process-local broadcast | Quest/journal progress visible to clients | Replicate quest/journal via PlayerState/GameState channel or RPC deltas | Quest authority | QuestComponent, PlayerState (new) | Client quest tracker updates | 2-PIE | MEDIUM (MP batch 2) |
| Building dupe vector | Client-side material burn | Server-side deduction | Move ConsumeItems into ServerPlaceBuilding | None | BuildingComponent | No dupe across 2-PIE | MP test | **HIGH (MP batch 1)** |
| Resource node visibility | Not replicated | Replicated nodes + harvest RPC | Replication + RPC | — | ResourceNode | Client harvests node | 2-PIE | MEDIUM (MP batch 1) |
| Per-player save | P1 only | All players persisted (or documented host-only policy) | Multi-controller save records + roster-per-player | Save schema | SaveSubsystem | P2 inventory restores | 2-PIE save round-trip | MEDIUM (MP batch 2) |
| Dedicated server target | None (no .Target.cs) | If dedicated in scope: ASTRAWILDServer target | Target file + config | Decision needed | Source targets | Server builds headless | Build | LOW (decision: listen-server first per V2) |
| Join/leave handling | None | Seamless join (spawn at rest point), leave (pawn cleanup) | GameMode hooks | Rest points (exist) | GameMode, PlayerController | Join mid-session works | 2-PIE | MEDIUM (MP batch 2) |
| Iris/anti-cheat validation | Not started | Server validation of all valuable ops (V2 §9) | Validation layers | RPC layer | various | Cheat attempts rejected | Net test | LOW (post-slice) |

## 10. PERFORMANCE GAPS (P15 vs Roadmap §37)

| Gap | Current State | Required | Missing | Priority |
|---|---|---|---|---|
| No profiling ever | No runnable build | CPU/GPU/RAM baseline on defined PC | First compile + PIE profiling session (target machine) | HIGH (after Batch 1 compile) |
| `GetAllActorsOfClass` in Protect state | O(n) per think at 4Hz per Protective Echo | Party cache on player character | Cache captured-party list (update on capture/load) — **fold into Batch 2** | MEDIUM |
| Per-frame subsystem lookups | `HandleNeedsDecay` resolves subsystems each tick | Cached pointers | Cache in BeginPlay | LOW |
| Ecosystem O(n) scans | Linear prey/herd scans per think | Spatial partition (grid hash) when population > threshold | Simple cell grid in ecosystem | LOW (post-slice) |
| Tick audit | Not performed | Documented tick budget per subsystem | Audit doc after first runnable build | MEDIUM |
| AI simulation tier validation | Intervals real; Tier2/3 semantics missing | Measured cost delta across tiers | After World Gaps tier work | LOW |

## 11. QA GAPS (P16 vs Roadmap §38)

| Gap | Current State | Required | Missing | Priority |
|---|---|---|---|---|
| Tautological tests | 7/9 assert local math | Tests instantiate real components | Rewrite: inventory add/remove via component; survival damage→death; capture bounds via real `ComputeCaptureChance`; mitigation via real function | **HIGH (Batch 2)** |
| Save round-trip test | None | Automation: build state → save → load → assert | World-based automation test (editor context) | **HIGH (Batch 2)** |
| Runtime smoke test | None | Scripted PIE pass/fail of core loop | Smoke checklist + automation hooks | MEDIUM (Batch 2-3) |
| Duplication tests | None | Dupe attempts fail (MP) | Test cases in MP batch | MEDIUM |
| Crash/corruption tests | None | Corrupt save rejected cleanly (checksum path exists) | Test corrupt files load-refusal | MEDIUM |
| Placeholder asset audit | Tracked by REPLACE_BEFORE_RELEASE comments | Manifest of every placeholder | Extract to `ASTRAWILD_ASSET_MANIFEST.md` updates | LOW |
| Build report | BUILD_STATUS.md honest ("never compiled") | Real build results per batch | Update after each target-machine compile | **CRITICAL discipline** |

## 12. CONTENT GAPS (P14 vs Roadmap §39 — *post-architecture only*)

| Gap | Current State | Required (launch scope) | Priority |
|---|---|---|---|
| Echo species count | 7 | Vertical Slice A: 3-5 complete (have 7 shallow-complete; deepen first: traits/abilities/utility) | First deepen existing (Creature gaps) before adding |
| Biomes | 1 flat Dawn Fields | Slice A: Dawn Fields only — polish; Slice C+: Rainforest, Salt Plains, Snowline, Veldara | Post-slice (WP required first) |
| Dungeons | 1 linear code dungeon | Slice A: 1 (fix completion first) | Batch 1-2 |
| Bosses | 1 coded (unspawned) | Slice A: 1 | Batch 1 |
| Quests | 6 (halts at 4) | Slice A: main chain complete | Batch 2 |
| NPCs | 2 (quest-giver + inert vendor) | Vendor functional (buy/sell with Loot_VendorStarter); dialogue-lite | Batch 3 |
| Items/recipes | 19 items / 10 recipes | Sufficient for slice; extend with armor/energy content as tech framework lands | Follow tech batches |
| Story/lore | Journal + CODE_DEFAULT descriptions | Environmental terminals (ancient recordings) — 3-5 lore entries | Batch 4+ |

---

## VERTICAL SLICE A READINESS — dependency-ordered batch plan

**Batch 1 (current): Core Loop Unblocked** — C-1..C-8 + H-4 + save repair pack. After Batch 1, the loop Start→gather→craft→build→research→power→work→dungeon→boss→save→reload is code-reachable end-to-end (pending target-machine compile).

**Batch 2: Integrity** — quest chain completion (hostile respawn, remaining objective types), building delete/refund, power persistence, real tests (component tests + save round-trip), ecosystem population fix, gamepad.

**Batch 3: Depth** — status effects live, hit reactions, telegraphs, armor framework, storage building, laser weapon, biome zoning, weather-visibility.

**Batch 4: Breadth** — drone, farming, world events, Tier2/3 semantics, NPC vendor/dialogue, lore terminals, weak points.

**Multiplayer batch (parallel after Batch 2):** client registry, RPC layer, creature replication, building server-side deduction, node replication, join/leave.

**Rule reminder:** No batch starts while the previous batch has unresolved Critical defects. Content expansion waits for architecture pass. Compile → Test → Runtime → Save/Load → Checklist → Document → Commit after every batch.
