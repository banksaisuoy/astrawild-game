# ASTRAWILD GLM 5.3 — SOURCE AUDIT (FINAL PRODUCTION RUN, PHASE 0)

> Auditor: GLM 5.3 lead repo agent · Scope: **all** `Source/`, `Config/`, `ASTRAWILD.uproject`,
> `Scripts/`, `Docs/` (101 source files read end-to-end by three independent audit passes).
> Baseline commit at audit time: `07f97c7` (Batch 7 — The Shattered Vale).
> Status vocabulary (V5): SOURCE_IMPLEMENTED / SOURCE_VERIFIED / ENGINE_VERIFIED / BLOCKED.
> **Nothing in this repo has ever been compiled** — every status below is SOURCE_IMPLEMENTED at best.

---

## 0. Executive summary

The repository is a single-module (`AstrawildCore`) UE 5.8 C++ codebase of ~19,300 LOC that
implements the creature-collection survival loop **end-to-end in source**: 6-zone procedural world,
survival vitals, inventory/equipment (3 slots), melee combat with elemental statuses, capture,
roster/party, work-site automation, power grid, research gating, crafting, building, dungeon with
gated progression, 3-phase boss, quests, vendor economy, journal/knowledge, checksummed save (v2).

**Two hard compile blockers (C-1, C-2) were found by static review and fixed in this run
(commit `fix: resolve compile blockers`).** Beyond those, include hygiene, delegate signatures,
constructor patterns, UHT ordering (`.generated.h` last), and replication boilerplate are
unusually clean for never-compiled code. The dominant residual risk is the standing
**never-compiled blocker** — only an in-engine build (Antigravity, UE 5.8, Windows) can clear it.

---

## 1. Compile-risk findings (static review)

| ID | Severity | File : line | Finding | Disposition this run |
|----|----------|-------------|---------|----------------------|
| C-1 | 🔴 Critical | `AstrawildGameMode.h:49`, `.cpp:83,119` | Raw pointer `Bootstrapper` used with `.Get()` → hard compile error (no member `Get` on raw ptr). | **FIXED** — member is now `TObjectPtr<AAstrawildWorldBootstrapper>` (also fixes GC hygiene). |
| C-2 | 🔴 Critical | `AstrawildCraftingComponent.cpp:154-162` | Server RPCs implemented as `ServerRequestCraft(...)` instead of `ServerRequestCraft_Implementation(...)` → UHT thunk redefinition + missing `_Implementation` (LNK2001). | **FIXED** — renamed both bodies to `_Implementation`. |
| C-3 | 🟠 High | `AstrawildInventoryComponent.h:95-100` | `EquippedItemId` / `EquippedShieldItemId` registered via `DOREPLIFETIME` but UPROPERTYs lacked `Replicated` → replication layout rejection / slots never replicate. | **FIXED** — `Replicated` specifier added. |
| C-4 | 🟠 High | `AstrawildDungeonGateActor.h:48` | `bOpen` declared `Replicated` but `OnRep_bOpen()` exists → clients never re-apply collision; gate stays sealed on clients in net play. | **FIXED** — now `ReplicatedUsing=OnRep_bOpen`. |
| C-5 | 🟡 Medium | `AstrawildEventBusSubsystem.h:26`, `AstrawildEchoCharacter.h:111` | `TWeakObjectPtr` in Blueprint-exposed UPROPERTYs (UHT 5.8 policy on weak-ptr BP exposure to verify). | **FIXED proactively** — event payload `Instigator` is `TObjectPtr<AActor>`; work-site ref kept weak but hidden from BP (visible only). |
| C-6 | 🟡 Medium | `AstrawildGameMode.cpp:95-99` | `SetTimerForNextTick` lambda captures raw `this` (GameMode) without weak guard. | **FIXED** — rebound via `FTimerDelegate::CreateUObject`. |
| C-7 | 🟡 Medium | `AstrawildItemRegistrySubsystem.cpp:133` | Unguarded `World->GetGameInstance()->...` (editor worlds can return null). | **FIXED** — null-checked chain. |
| C-8 | 🟡 Medium | `AstrawildEcosystemSubsystem.h`, `AstrawildPowerSubsystem.h` | `FObjectKey` used as TMap key relying on transitive include. | **FIXED** — explicit `#include "UObject/ObjectKey.h"`. |
| C-9 | 🟡 Medium | `AstrawildEcosystemSubsystem` reg. order | Echoes spawned by bootstrapper register with ecosystem **before** `InitializeFromDefinition` sets species → `WildCount` under-reported (HostileSpawner re-registers, wild spawns don't). | **FIXED** — registration re-run on initialize; subsystem tolerant to re-register. |
| C-10 | 🟢 Low | `AstrawildResourceNode.cpp:79`, `AstrawildRestPoint.cpp:68` | Mixed-language UI strings (Thai vs English HUD). | **ACCEPTED** — intentional bilingual UX; documented. |
| C-11 | 🟢 Low | `Build.cs` | `GameplayAbilities` / `GameplayTasks` declared but unused; `StateTree` plugin enabled but unused. | **ACCEPTED** — harmless (kept for GAS roadmap); noted in readiness report. |
| C-12 | 🟢 Low | `AstrawildAutomationTests.cpp:81` | One tautological `TestTrue(..., true)` placeholder. | **FIXED** — replaced by real scanner/quest assertions this run. |

Verified clean (no action): `.generated.h` last-include ordering in all 42 UHT headers;
`GENERATED_BODY()` in every UCLASS/USTRUCT; all 15 dynamic delegate bindings match signatures;
`GetLifetimeReplicatedProps` class-name macros correct; constructors follow
`CreateDefaultSubobject`/`SetupAttachment` conventions; Enhanced Input includes complete;
perception delegate signature correct; no subsystem access in constructors; no threads/async
anywhere; 20 automation tests reference only real systems.

---

## 2. Gameplay-loop coverage (23 stages)

Loop: NEW GAME → EXPLORE → SURVIVE → FIND ECHO → SCAN → COMBAT → CAPTURE → INVENTORY →
GATHER → BUILD BASE → GENERATE POWER → ASSIGN ECHO → AUTOMATION → RESEARCH → ADVANCED
TECHNOLOGY → DUNGEON → BOSS → REWARD → RETURN BASE → SAVE → QUIT → LOAD → VERIFY.

| Stage | Status before this run | Gap | This run |
|-------|------------------------|-----|----------|
| NEW GAME | PARTIAL — GameMode boots bootstrapper; no menu | no menu/quit flow | Pause menu (ESC): Resume / Save / Quit (loop stage QUIT) |
| EXPLORE | COMPLETE (Batch 7 six zones) | — | — |
| SURVIVE | COMPLETE | — | — |
| FIND ECHO | COMPLETE (10 species + spawners) | — | — |
| SCAN | PARTIAL — passive observe only; `Interaction.Scan` tag dead | no active scanner | **Scanner framework**: scanner equipment item, hold-to-scan key, HUD scan readout, scan speeds up journal progress |
| COMBAT | COMPLETE melee only | no ranged | **Ranged path**: laser weapon (projectile), energy cell ammo, server-authoritative |
| CAPTURE | COMPLETE | — | — |
| INVENTORY | PARTIAL — no screen | invisible to player | **Inventory screen (TAB/I)**: stacks, weight, equipment, use/consume |
| GATHER | COMPLETE | node depletion not saved | deferred (regen-from-seed acceptable) |
| BUILD BASE | COMPLETE (13 pieces) | — | — |
| GENERATE POWER | COMPLETE | grid battery not persisted | **Grid charge persisted** (see SAVE) |
| ASSIGN ECHO | COMPLETE in-play | assignments lost on save | **Assignments + site output persisted** |
| AUTOMATION | COMPLETE (2 pre-placed sites) | sites not buildable | **Work sites buildable** (Gathering Hub / Auto-Forge building defs) + Utility Robot worker |
| RESEARCH | COMPLETE gating, no UI | auto-buys cheapest | **Research screen** at Research Desk: full tree, costs, prereqs, player choice |
| ADVANCED TECH | PARTIAL (armor ×3, melee ×3) | no helmet/exosuit/laser/drone/robot | **Full advanced framework**: helmet+exosuit slots (5 slots total), thermal insulation, energy capacitor, laser, drone, robot |
| DUNGEON | COMPLETE (5 rooms) | puzzle room is cosmetic | accepted for vertical slice (documented) |
| BOSS | PARTIAL — phases + enrage only | no telegraphs/weak points/hazards/HP bar | **Boss overhaul**: telegraphed AoE specials, weak-point core (×2 dmg window), arena hazards on phase 2/3, HUD boss bar |
| REWARD | COMPLETE | — | — |
| RETURN BASE | COMPLETE (exit portal) | — | — |
| SAVE | gaps | work sites, battery, rest points | **All persisted** (schema v3, additive migration v1→v2→v3) |
| QUIT | MISSING | — | Pause menu Quit |
| LOAD | gaps (same as save) | — | closed with SAVE |
| VERIFY | PARTIAL — 20 tests, never run | — | +N new tests; all require engine run |

---

## 3. Save-coverage matrix (PHASE 16 requirements → implementation)

Requirement list from the production directive. ✔ = persisted in `UAstrawildSaveSubsystem`
(schema v3 after this run), with the record that carries it.

| Requirement | Saved? | Carrier |
|---|---|---|
| Player transform | ✔ | `FAstrawildPlayerSaveData` |
| Vitals (health/stamina/hunger/thirst/temp) | ✔ | `PlayerSurvival` snapshot |
| Inventory stacks + weight | ✔ | `FAstrawildItemStackSaveData[]` |
| Equipment (weapon/shield/armor/helmet/exosuit/scanner) | ✔ | equipment ids, HasItem-guarded restore |
| Echo roster (species/level/xp/trust/bond/needs/personality) | ✔ | `FAstrawildEchoSaveData[]` (v2) |
| Party membership + transforms | ✔ | party flag + respawn around player |
| Work assignments (Echo→site) | ✔ **this run** | `FAstrawildWorkSiteSaveData` |
| Work-site stored output | ✔ **this run** | same record |
| Buildings (def/transform/health/switch) | ✔ | `FAstrawildBuildingSaveData[]` |
| Building health | ✔ | `CurrentHealth` in building record |
| Power grid state + battery charge | ✔ **this run** | `FAstrawildPowerGridSaveData` (grid charge + per-building charge wired) |
| Research (unlocked techs + points) | ✔ | research record |
| Quest progress (active + objectives) | ✔ | quest record |
| Journal / knowledge / scans | ✔ | journal record |
| Dungeon state (cleared rooms, completion) | ✔ | dungeon record |
| Zone discovery | ✔ | `FAstrawildZoneSaveData` (v2 additive) |
| Rest-point activation | ✔ **this run** | rest record (now iterated by `SaveWorld`) |
| Resource-node depletion | ✖ deferred | world regenerates deterministically from seed (documented policy) |
| Drone/robot companions | ✔ **this run** | `FAstrawildDroneSaveData` / robot as building-backed worker |

---

## 4. Config / .uproject / build files verdict

- `.uproject`: UE 5.8, one Runtime module (`AstrawildCore`), plugins `EnhancedInput`,
  `ProceduralMeshComponent`, `GameplayTags` (+ unused `StateTree`) — **complete for the code**.
- `DefaultEngine.ini`: `GlobalDefaultGameMode=/Script/AstrawildCore.AstrawildGameMode` ✔;
  `GameDefaultMap=/Engine/Maps/Entry` (engine built-in — zero-asset runtime world boots from
  GameMode BeginPlay; packaged-build startup listed in the verification queue); renderer
  (Nanite/TSR/VSM), navmesh-around-invokers ✔, IpNetDriver ✔.
- `DefaultGame.ini`: title, `MaxPlayers=4`, mouse axes only.
- **No `DefaultInput.ini`** — input is built entirely at runtime
  (`BuildRuntimeInputDefaults`, documented in `ASTRAWILD_INPUT_REFERENCE.md`); this run adds
  **gamepad mappings** to the same runtime path.
- `Build.cs` / both `Target.cs`: 5.8-correct (`BuildSettingsVersion.V5`, `IncludeOrderVersion.Unreal5_8`);
  every used API's module is declared.

---

## 5. Documentation drift (reconciled this run)

1. `ANTIGRAVITY_START_HERE.md` / `ANTIGRAVITY_BUILD_CHECKLIST.md` previously mandated creating
   BPs, data assets and `L_Prototype.umap` — superseded by the zero-asset runtime world.
   **A "SUPERSEDED" banner now redirects** to the runtime-world flow; the asset-creation path is
   kept only as the optional editor-landscape alternative (`.r16` import).
2. V5 tasklist (`ASTRAWILD_GLM53_UE5_IMPLEMENTATION_TASKLIST_V5.md`) was stale — only Phase 11
   had checkmarks. **Status marks synchronized** with BUILD_STATUS ledger + this run's batches.
3. Required V5 report files now exist: this audit, `ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md`,
   `ASTRAWILD_BUILD_READINESS_REPORT.md`, `ASTRAWILD_MILESTONE_REPORT.md` (+ updated
   `ASTRAWILD_PRODUCTION_CHECKLIST_V2.md`, `BUILD_STATUS.md`).
4. Count drift corrected in BUILD_STATUS (tests, docs, LOC).

---

## 6. Residual risk register (after this run)

| Risk | Sev | Mitigation |
|------|-----|------------|
| Never compiled (whole repo) | Blocker | Antigravity first build; fix-forward policy Phase B; verification queue §1 |
| UHT surprises on 5.8 (native tags count, weak-ptr exposure) | Medium | Proactive C-5 fix; conservative API surface |
| Entry map as GameDefaultMap in packaged builds | Medium | Verification queue item; PIE unaffected |
| Runtime-generated navmesh correctness at scale | Medium | Invoker-based generation + verification queue playtest |
| MP beyond listen-server (remote-client shop/dismantle RPCs) | Low | Documented single-player-first policy (H-9/H-12) |
| Placeholder visuals (`REPLACE_BEFORE_RELEASE` ×6) | Accepted | Vertical slice ships on engine primitives by design |

---

## 7. Verdict

With C-1…C-4 fixed, the source side is a coherent, compile-conservative UE 5.8 codebase whose
gameplay loop is closed **in source**. Remaining STOP-CONDITION blockers are engine-side only
(compile + playtest), tracked exhaustively in `ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md`.
