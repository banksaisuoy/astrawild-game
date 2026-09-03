# ASTRAWILD — MASTER TASK REGISTRY

**Companion to**: `Docs/ASTRAWILD_MASTER_CONTROL.md` v3.0
**Scope**: every task relevant to the Final Run; no orphans, no duplicates, no undocumented blockers.
**Statuses**: PLANNED / IN_PROGRESS / IMPLEMENTED / BUILT / TESTED / UE5_VERIFIED / ACCEPTED / BLOCKED (+ ENGINE-UNVERIFIED qualifier)

> Verification legend: `static` = machine-checked without an engine (this sandbox).
> `engine` = requires the Antigravity Windows/UE5 machine. GLM never claims engine PASS.


> [!WARNING]
> **RECOVERY RESET (2026-09-03)**: the Final Run commits referenced in the table below
> (f310698 / 0ae9764 / aee4cc8 / af30c98) were lost with the sandbox before they could be pushed.
> FR-1..12 statuses were reset to **PLANNED (REDO)** — the File/Verification columns remain valid
> as implementation specs. FR-13/14 artifacts were restored from the glm-staging mirror.
> Re-implementation happens on branch `final-completion` (base f31f5e1 = PR #4 head);
> commit SHAs will be filled in as batches land. Status vocabulary for completed work:
> **IMPLEMENTED** only (BUILT/TESTED/UE5_VERIFIED/ACCEPTED are Antigravity-owned).
---

## A. Final Run tasks (glm/final-run branch)

| ID | Category | Description | Owner | Status | Dependency | Files | Commit | Verification | Blocker | Next |
| :-- | :--- | :--- | :-- | :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| FR-1 | Inventory | RemoveItem qty guard (dup exploit + FindChecked crash); ConsumeItems aggregation; SetItemStacks sanitize | GLM | PLANNED (REDO — lost) | none | InventoryComponent.cpp | f310698 | ASTRAWILD.Inventory.TransactionSafety | none | engine test |
| FR-2 | Save | Future-schema refusal; day-catch-up cap 4096; identity-transform guard; LoadLatest slot fallback; robot chassis persisted; drone single-entry; building fail-closed + refund | GLM | PLANNED (REDO — lost) | none | SaveSubsystem.*, BuildingActor.cpp | f310698 | ASTRAWILD.Save.SchemaV5Ending | none | engine test |
| FR-3 | Quests | One-active-quest guard; silent rewards; negative-amount event guard; element matrix alignment (7 species) | GLM | PLANNED (REDO — lost) | FR-1 | QuestComponent.cpp, ContentLibrary.cpp, ProductionContent.cpp | f310698, aee4cc8 | ASTRAWILD.Quest.FinalRunChain | none | engine test |
| FR-4 | Economy | Silent refunds (craft cancel, placement validation); roster import sanitize; node identity fallback | GLM | PLANNED (REDO — lost) | FR-1 | CraftingComponent.cpp, BuildingComponent.cpp, EchoRosterSubsystem.cpp, ResourceNode.cpp | f310698 | ASTRAWILD.Inventory.TransactionSafety | none | engine test |
| FR-5 | Story | Act 3 content pack: MQ-13..17 + 3 bosses + items/tech/recipe/loot + ending dialogue | GLM | PLANNED (REDO — lost) | FR-3 | ProductionContent.cpp/.h | 0ae9764 | ASTRAWILD.Quest.FinalRunChain, ASTRAWILD.Echo.FinalRunBosses, ASTRAWILD.Tech.SkiffEngineering, ASTRAWILD.Dialogue.EndingChoice | none | engine test |
| FR-6 | Ending system | EAstrawildEndingState + SetEndingState + weather pin + TriggerEndingId consequence + HUD banner + save V5 | GLM | PLANNED (REDO — lost) | FR-5 | GameState.*, DataAssets.h, DialogueComponent.cpp, WeatherSubsystem.cpp, SaveSubsystem.*, HudWidget.*, Types.h | 0ae9764 | ASTRAWILD.Save.SchemaV5Ending, ASTRAWILD.Dialogue.EndingChoice | none | engine test |
| FR-7 | World gen | Eye of the Maelstrom dungeon + portals/markers; Glass Tyrant world boss; Dawnstead marker; zone helpers | GLM | PLANNED (REDO — lost) | FR-5 | WorldBootstrapper.cpp/.h | 0ae9764 | Scripts/validate_final_run.py (static) | none | engine test |
| FR-8 | Traversal | Stratos Coil ceiling gate (120m→160m); skiff mesh binding; world-seed ground probe | GLM | PLANNED (REDO — lost) | FR-5 | SkiffActor.cpp/.h | 0ae9764, aee4cc8 | static (mesh path resolves) | none | engine test (mesh orientation) |
| FR-9 | Buildings | Floor/Roof/Door/StorageCrate + door toggle + crate deposit/withdraw + save | GLM | PLANNED (REDO — lost) | none | ContentLibrary.cpp, BuildingActor.*, BuildingComponent.cpp, Types.h | aee4cc8 | static (category population check) | none | engine test |
| FR-10 | Villages | 5 NPC dialogue trees (Wren/Borin/Bram/Jori/Nima) + Azure Shallows POI | GLM | PLANNED (REDO — lost) | none | ProductionContent.cpp | aee4cc8 | static (registry checks) | none | engine test |
| FR-11 | Feedback | Capture toast + A_Echo_Capture_Success audio; boss display names | GLM | PLANNED (REDO — lost) | none | CaptureComponent.*, EchoBossCharacter.*, HudWidget.cpp | 0ae9764, aee4cc8 | static | none | engine test |
| FR-12 | Tests | +6 world-free contracts (63 total) | GLM | PLANNED (REDO — lost) | FR-1..6 | AutomationTests.cpp | 0ae9764 | static count; engine run pending | none | engine test |
| FR-13 | Validation | validate_final_run.py (content/LFS/asset-path/test-count checks) | GLM | RESTORED (static) | none | Scripts/validate_final_run.py | (recovery commit) | baseline run recorded in recovery | none | re-run turns green as FR-1..12 land |
| FR-14 | Docs | MASTER_CONTROL v3.1 (recovery) + this registry + HANDOFF + READINESS | GLM | RESTORED | FR-1..13 | Docs/*.md | (recovery commit) | review | none | Antigravity review |

> Baseline automation count on f31f5e1 = **57** (c65d734 line). The 6 Final Run tests
> (63 total) were lost — re-added by FR-12 during the redo.

## B. Antigravity integration tasks (engine machine)

| ID | Category | Description | Owner | Status | Dependency | Verification | Blocker | Next |
| :-- | :--- | :--- | :-- | :-- | :-- | :-- | :-- | :-- |
| AG-1 | Git | Pull/merge final-completion; push main; close PR #4 as absorbed | Antigravity | PLANNED | FR-14 | git log / PR state | credentials | execute after AG-2..5 |
| AG-2 | Build | MSVC compile of final SHA (0 errors) | Antigravity | PLANNED | FR-* | raw build log | UE 5.8.2 machine | run |
| AG-3 | Tests | 63/63 automation green | Antigravity | PLANNED | AG-2 | raw automation log | none | run |
| AG-4 | Playtest | PIE golden path: MQ chain → Eye → Sovereign → ending A/B → post-game; save/load round-trip | Antigravity | PLANNED | AG-3 | raw PIE log + per-checkpoint trace | none | run |
| AG-5 | Package | Cook+package exit 0; packaged exe boots to MainMap | Antigravity | PLANNED | AG-2 | raw UAT log | FZ-A1 recurrence watch | run |
| AG-6 | Fix loop | Any engine-only defect → smallest fix on a branch; architectural problems return to GLM | Antigravity | PLANNED | AG-4/5 | fix commits | none | as found |

## C. Carry-over tasks (pre-Final-Run state, tracked to closure)

| ID | Category | Description | Owner | Status | Notes |
| :-- | :--- | :--- | :-- | :-- | :-- |
| CV-1 | Assets | 115 ArtPack .uasset to Git LFS | Antigravity | ACCEPTED | f31f5e1; GLM verified 459/459 LFS objects resolve |
| CV-2 | Hardening | GLM source hardening SH-01..04 + 57 tests | Antigravity | TESTED (declared) | c65d734; re-run with 63 at AG-3 |
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
