# ASTRAWILD — MASTER TASK REGISTRY

**Companion to**: `Docs/ASTRAWILD_MASTER_CONTROL.md` v3.2
**Scope**: every task relevant to the Final Run; no orphans, no duplicates, no undocumented blockers.
**Statuses**: PLANNED / IN_PROGRESS / IMPLEMENTED / BUILT / TESTED / UE5_VERIFIED / ACCEPTED (+ ENGINE-UNVERIFIED qualifier)

> Verification legend: `static` = machine-checked without an engine (this sandbox).
> `engine` = requires the Antigravity Windows/UE5 machine. GLM never claims engine PASS.

> [!NOTE]
> **REDO COMPLETE (2026-09-03, Final Completion Run)**: FR-1..12 were re-implemented
> on branch `final-completion` and PUSHED per batch (binding user rule — zero unpushed
> batches). Every implementation commit below is live on GitHub. The static validator
> (`Scripts/validate_final_run.py`) runs **46/46 ALL CHECKS PASSED** at the final state.
> Engine verification (AG-2..5) remains Antigravity-owned and pending.

## A. Final Run tasks (final-completion branch — REDONE & PUSHED)

| ID | Category | Description | Owner | Status | Dependency | Files | Commit | Verification | Blocker | Next |
| :-- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| FR-1 | Inventory | RemoveItem qty guard (dup exploit + FindChecked crash); ConsumeItems aggregation; SetItemStacks sanitize | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | none | InventoryComponent.cpp | 61c45e6 | ASTRAWILD.Inventory.TransactionSafety | none | engine test |
| FR-2 | Save | Future-schema refusal; day-catch-up cap 365; identity-transform guard; LoadLatest slot fallback; building fail-closed + refund (+ v5 chain + ending restore) | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | none | SaveSubsystem.*, BuildingActor.cpp | 61c45e6, 93ee929 | ASTRAWILD.Save.ConsistencyContracts, ASTRAWILD.Save.SchemaV5Ending | none | engine test |
| FR-3 | Quests | One-active-quest guard; silent rewards; negative-amount event guard; element matrix alignment (7-species canon) | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FR-1 | QuestComponent.cpp, ContentLibrary.cpp, ProductionContent.cpp | 61c45e6, b9c1bd6 | ASTRAWILD.Quest.FinalRunChain, ASTRAWILD.Quest.ImportSafety | none | engine test |
| FR-4 | Economy | Silent refunds; roster import sanitize; node identity fallback | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FR-1 | CraftingComponent.cpp, BuildingComponent.cpp, EchoRosterSubsystem.cpp, ResourceNode.cpp | 61c45e6 | ASTRAWILD.Inventory.TransactionSafety, ASTRAWILD.Echo.RosterImportSafety | none | engine test |
| FR-5 | Story | Act 3 content pack: MQ-13..17 + 3 bosses + items/tech/recipe/loot + ending dialogue | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FR-3 | ProductionContent.cpp/.h | 93ee929 | ASTRAWILD.Quest.FinalRunChain, ASTRAWILD.Echo.FinalRunBosses, ASTRAWILD.Tech.SkiffEngineering, ASTRAWILD.Dialogue.EndingChoice | none | engine test |
| FR-6 | Ending system | EAstrawildEndingState + SetEndingState + weather pin + TriggerEndingId consequence + HUD banner + save V5 | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FR-5 | GameState.*, DataAssets.h, DialogueComponent.cpp, WeatherSubsystem.cpp, SaveSubsystem.*, HudWidget.*, Types.h | 93ee929 | ASTRAWILD.Save.SchemaV5Ending, ASTRAWILD.Dialogue.EndingChoice | none | engine test |
| FR-7 | World gen | Eye of the Maelstrom dungeon + portals/markers; Glass Tyrant world boss; Dawnstead marker; zone helpers | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FR-5 | WorldBootstrapper.cpp/.h | 93ee929 | Scripts/validate_final_run.py (static — 8/8 wiring checks) | none | engine test |
| FR-8 | Traversal | Stratos Coil ceiling gate (120m→160m); skiff mesh binding; world-seed ground probe | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FR-5 | SkiffActor.cpp/.h | 93ee929 | ASTRAWILD.Skiff.CeilingGate; static mesh-path resolve | none | engine test (mesh orientation) |
| FR-9 | Buildings | Floor/Roof/Door/StorageCrate + door toggle + crate deposit/withdraw + save | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | none | ContentLibrary.cpp, BuildingActor.*, BuildingComponent.cpp, Types.h | b9c1bd6 | static (category population + validator 46/46) | none | engine test |
| FR-10 | Villages | 5 NPC dialogue trees (Wren/Borin/Bram/Jori/Nima) + Azure Shallows POI | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | none | ProductionContent.cpp, ContentLibrary.cpp | b9c1bd6 | static (registry checks + validator) | none | engine test |
| FR-11 | Feedback | Capture toast + A_Echo_Capture_Success audio; boss display names | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | none | CaptureComponent.*, EchoBossCharacter.*, HudWidget.cpp | 93ee929 | ASTRAWILD.Echo.FinalRunBosses | none | engine test |
| FR-12 | Tests | +6 world-free contracts (61 → 67 total; validator gate ≥63 ✓) | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FR-1..6 | AutomationTests.cpp | 93ee929 | static count 67; engine run pending | none | engine test |
| FR-13 | Validation | validate_final_run.py — **46/46 ALL CHECKS PASSED** at final state | GLM | PASS (static) | none | Scripts/validate_final_run.py | (each batch) | full run output in worklog | none | re-run at AG-2 |
| FR-14 | Docs | MASTER_CONTROL v3.2 + this registry + HANDOFF + READINESS + TEST_INVENTORY | GLM | UPDATED | FR-1..13 | Docs/*.md | (docs commit) | review | none | Antigravity review |

### A.2 Final source completion pass (FINAL-AUDIT — 2026-09-03, all pushed)

| ID | Category | Description | Owner | Status | Dependency | Files | Commit | Verification | Blocker | Next |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| FA-1 | Audit | Full-repository audit Phases A–O (5 parallel subagent reports: gameplay loop/player, echo/save, quest/boss, world/automation, input/UI/MP/perf) | GLM | COMPLETE | none | /home/z/my-project/audit/*.md (session evidence) | 1be6e20 | audit reports (in sandbox worklog) | none | — |
| FA-2 | P0/P1 fixes | 11 defects: drone Owner compile/crash (C-1), POI one-shot quest stall (G-1), MQ-17 ending gate (G-2), boss defeat back-fill (G-3), view-axis aiming (F-01), crafting screen wiring (F-02), echo owner identity (H-1), robot chassis save (H-2), camp respawn, CampKitchen spawn, MainMap default map (H-3) | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FA-1 | 19 files | 1be6e20 | ASTRAWILD.Quest.OneShotBackFill etc. | none | engine test |
| FA-3 | Medium/low | 20 defects: element canon unification (151 bestiary rows + 4 species + boss resist), echo health persist, species DefeatLoot live, research import sanitize, AI perception + fight-back + stranded recall, worker presence, screen key closes, Thai strings, config cleanups, FastForward cheat, evolution hook | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FA-2 | 40 files | 69a1d65 | static (validator 46/46) + tests below | none | engine test |
| FA-4 | Regression | +5 world-free contracts: OneShotBackFill, DefeatCountImportSafety, DismantleIsNotPlacement, Research.ImportSafety, Save.FinalAuditContracts | GLM | IMPLEMENTED (ENGINE-UNVERIFIED) | FA-2/3 | AutomationTests.cpp | a5aa74d | static count 72; engine run pending | none | engine test |
| FA-5 | Docs | Phase Q reconciliation: 72-test truth everywhere, dead glm/final-run references fixed in HANDOFF, control list corrected, readiness gate re-checked | GLM | UPDATED | FA-2..4 | Docs/*.md | (this docs commit) | review | none | Antigravity review |

> Automation suite: **72 world-free contract tests** (57 baseline + 4 hardening from
> BATCH-1 + 6 Final Run from BATCH-2 + 5 final-audit regressions). Full inventory:
> `Docs/ASTRAWILD_TEST_INVENTORY.md`.

## B. Antigravity integration tasks (engine machine)

| ID | Category | Description | Owner | Status | Dependency | Verification | Blocker | Next |
| :-- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| AG-1 | Git | Pull/merge final-completion; push main; close PR #4 as absorbed | Antigravity | PLANNED | FA-5 | git log / PR state | credentials | execute after AG-2..5 |
| AG-2 | Build | MSVC compile of final SHA (0 errors) | Antigravity | PLANNED | FR-* | raw build log | UE 5.8.2 machine | run |
| AG-3 | Tests | 72/72 automation green | Antigravity | PLANNED | AG-2 | raw automation log | none | run |
| AG-4 | Playtest | PIE golden path: MQ chain → Eye → Sovereign → ending A/B → post-game; save/load round-trip; door/crate interactions | Antigravity | PLANNED | AG-3 | raw PIE log + per-checkpoint trace | none | run |
| AG-5 | Package | Cook+package exit 0; packaged exe boots to MainMap | Antigravity | PLANNED | AG-2 | raw UAT log | FZ-A1 recurrence watch | run |
| AG-6 | Fix loop | Any engine-only defect → smallest fix on a branch; architectural problems return to GLM | Antigravity | PLANNED | AG-4/5 | fix commits | none | as found |

## C. Carry-over tasks (pre-Final-Run state, tracked to closure)

| ID | Category | Description | Owner | Status | Notes |
| :-- | :--- | :--- | :--- | :--- | :--- |
| CV-1 | Assets | 115 ArtPack .uasset to Git LFS | Antigravity | ACCEPTED | f31f5e1; GLM verified 459/459 LFS objects resolve |
| CV-2 | Hardening | GLM source hardening SH-01..04 + 57 tests | Antigravity | TESTED (declared) | c65d734; re-run with 67 at AG-3 |
| CV-3 | Input | Playable input/camera fix chain 520c78e+df8df83 | Antigravity | TESTED (declared) | re-verify at AG-4 |
| CV-4 | QA | Gamepad actuation (V-31) | Antigravity | BLOCKED | physical controller hardware |
| CV-5 | Economy | Duskmoth has no loot / berry faucet thin (FZ-ECO-2/3) | GLM | PLANNED (P2) | balance polish batch — post-engine-verify |
| CV-6 | MP | Dedicated-server co-op (H-9) | GLM | PLANNED (P3) | single-player-first by design; portals/skiff client paths early-return |
| CV-7 | Docs | Historical doc banners | GLM | ACCEPTED | HISTORICAL/SUPERSEDED stamps per MASTER_CONTROL §12 |

## D. Deferred-by-design (explicitly out of Final Run scope)

| Item | Reason |
| :-- | :-- |
| Vess/Ione NPC skins for Act 3 | quests chain automatically; Maren carries the ending — new NPCs are cosmetic scope |
| SQ-23 side quests | post-game content batch after engine verification |
| Ending cinematics | ending = world-state + banner + dialogue by design (no Sequencer dependency) |
| 214-species unique meshes | data-driven bestiary + body plans + ArtPack hero species per §5 scope rule |
| NG+ rules | post-game loop covers the same fantasy without a new mode |

---

**Orphan check**: none (every task has an owner + status + next).
**Contradiction check**: none (single active quest rule documented; element matrix single-sourced).
**Duplicate check**: none (Act 3 content registered once; PR #4 content classified, not re-implemented).
