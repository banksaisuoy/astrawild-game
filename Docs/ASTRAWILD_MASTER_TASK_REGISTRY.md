# ASTRAWILD — MASTER TASK REGISTRY

**Companion to**: `Docs/ASTRAWILD_MASTER_CONTROL.md` v5.0
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

> Automation suite: **109 world-free contract tests** (57 baseline + 4 hardening from
> BATCH-1 + 6 Final Run from BATCH-2 + 5 final-audit regressions + 12 GDP + 15 SCP +
> 3 FCR + 7 depth passes DP-3..DP-9).
> Full inventory: `Docs/ASTRAWILD_TEST_INVENTORY.md` (rows 1-109). One authoritative value
> per metric — enforced by the validator's §11 census gates.

## B. Antigravity integration tasks (engine machine)

| ID | Category | Description | Owner | Status | Dependency | Verification | Blocker | Next |
| :-- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| AG-1 | Git | Pull/merge final-completion; push main; close PR #4 as absorbed | Antigravity | PLANNED | FA-5 | git log / PR state | credentials | execute after AG-2..5 |
| AG-2 | Build | MSVC compile of final SHA (0 errors) | Antigravity | PLANNED | FR-* | raw build log | UE 5.8.2 machine | run |
| AG-3 | Tests | 109/109 automation green (count read from the repo — the validator exact gate pins it) | Antigravity | PLANNED | AG-2 | raw automation log | none | run |
| AG-4 | Playtest | PIE golden path: MQ chain → Eye → Sovereign → ending A/B → post-game; save/load round-trip; door/crate interactions | Antigravity | PLANNED | AG-3 | raw PIE log + per-checkpoint trace | none | run |
| AG-5 | Package | Cook+package exit 0; packaged exe boots to MainMap | Antigravity | PLANNED | AG-2 | raw UAT log | FZ-A1 recurrence watch | run |
| AG-6 | Fix loop | Any engine-only defect → smallest fix on a branch; architectural problems return to GLM | Antigravity | PLANNED | AG-4/5 | fix commits | none | as found |

## C. Carry-over tasks (pre-Final-Run state, tracked to closure)

| ID | Category | Description | Owner | Status | Notes |
| :-- | :--- | :--- | :--- | :--- | :--- |
| CV-1 | Assets | 115 ArtPack .uasset to Git LFS | Antigravity | ACCEPTED | f31f5e1; GLM verified 459/459 LFS objects resolve |
| CV-2 | Hardening | GLM source hardening SH-01..04 + 57 tests | Antigravity | TESTED (declared) | c65d734; re-run with 99 at AG-3 |
| CV-3 | Input | Playable input/camera fix chain 520c78e+df8df83 | Antigravity | TESTED (declared) | re-verify at AG-4 |
| CV-4 | QA | Gamepad actuation (V-31) | Antigravity | BLOCKED | physical controller hardware |
| CV-5 | Economy | Duskmoth has no loot / berry faucet thin (FZ-ECO-2/3) | GLM | CLOSED (source) | FCR Phase 17: Duskmoth DefeatLoot added (Dawnbloom x1 + Fiber x2); numeric balance stays PIE-tuning |
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

## E. Gameplay Depth Pack (GDP — user-directed expansion, post-freeze)

| ID | Area | Deliverable | Owner | Status | Notes |
|----|------|-------------|-------|--------|-------|
| GDP-1 | Echo combat | Ability engine: 44 templates + per-species loadouts (authored AbilityIds + derived element/role/family kits), level gates, replicated cooldowns, AI combat casting, T-key party cast, Shell/negative-DPS status semantics | GLM | IMPLEMENTED | `AstrawildAbilityLibrary` + `AstrawildEchoCharacter` + `AstrawildEchoAIController` + HUD line; ENGINE-UNVERIFIED |
| GDP-2 | Locomotion | Land/Water/Flying classes with derivation rule, MOVE_Flying + 3D steering for flyers, sea-zone speed tuning for swimmers | GLM | IMPLEMENTED | Deterministic; covers all 210+ species; ENGINE-UNVERIFIED |
| GDP-3 | Player growth | 5 attributes + XP wiring at combat/capture/craft/survival sites, 7 milestone skills with smart-cast ladder (Y), passive bonuses consumed by existing systems, save round-trip | GLM | IMPLEMENTED | Additive v5 field; ENGINE-UNVERIFIED |
| GDP-4 | NPC affinity | 0-100 tiers, talk/trade gains with daily gate, up to 15% vendor discount, per-NPC-id persistence | GLM | IMPLEMENTED | Additive v5 field; ENGINE-UNVERIFIED |
| GDP-5 | Quality | 12 automation contracts (84 cumulative at GDP time — now 99 with SCP), validator gate, docs updated | GLM | IMPLEMENTED | `ASTRAWILD.Ability.*`, `ASTRAWILD.Locomotion.Derivation`, `ASTRAWILD.Attributes.*`, `ASTRAWILD.NPC.Affinity*` |


## §F SCP — Systems Completion Pack (2026 session)

| ID | System | Phase | Status | Commit |
|---|---|---|---|---|
| SCP-1a | DataValidator (static + registry + checksum) | 1.2 | IMPLEMENTED | a7a827f |
| SCP-1b | AssetFallback + ErrorReporter | 2.1/2.3 | IMPLEMENTED | a7a827f |
| SCP-1c | Durability + Spoilage + tools + Repair Bench/Ice Box | 12 | IMPLEMENTED | a7a827f |
| SCP-2 | Base Terminal + Creature Sanity + healthcare | 9 | IMPLEMENTED | 394ac81 |
| SCP-3 | Mount/Rider + socket contract | 5.3 | IMPLEMENTED | edc6b08 |
| SCP-4 | Dual-Tech combos + DDA | 6.3/3.2 | IMPLEMENTED | 6cd29e4 |
| SCP-5 | NPC schedules + crops + offline production + turret | 7/8/11 | IMPLEMENTED | bbe2e3c |
| SCP-6 | Genetics + Breeding/Incubator + Perf manager | 10/13.1 | IMPLEMENTED | 9864cce |
| SCP-7 | Object pooling + TeamAgent + RPC limiter | 13.2/4 | DEFERRED | engine-verify first |

## §G FINAL GAME COMPLETION RUN (current session — user-ordered autonomous completion)

Directive: continue autonomously until the repository honestly reaches
READY_FOR_FINAL_BUILD; reconcile every contradictory registry value from actual
source; audit + fix; keep canon locked; deterministic handoff.

| ID | Area | Deliverable | Owner | Status | Notes |
|----|------|-------------|-------|--------|-------|
| FCR-0 | Registry reconciliation | One authoritative value per metric: test count unified to **102** (from AutomationTests.cpp + validator); content census established (76 items / 56 recipes / 229 species / 26 buildings / 17 techs / 17 quests / 11 loot / 13 POIs / 9 events / 11 NPCs / 11 dialogues / 8 weapons / 10 nodes / 4 sites / 3 robots); census enforced by validator §11 equality gates; live engine census log added (hardcoded stale counts removed from ContentLibrary) | GLM | IMPLEMENTED | commit in this run; ENGINE-UNVERIFIED (log fires at engine boot) |
| FCR-1 | Source audit | Fresh full audit of GDP+SCP code (baca0f6 + a7a827f..f9892b6 — not covered by the prior 5-audit pass) + fix every real defect found | GLM | COMPLETE | 5 parallel agents found 2 CRITICAL + 17 HIGH + 13 MEDIUM + 15 LOW; every one verified against source and fixed in FCR-1-A (9bca989) / FCR-1-B (30e9e44) / FCR-1-C (aea01ed: +3 regression contracts, suite 102) |
| FCR-2 | Player experience verification (Phase 2 checklist) | movement/camera/sprint/stamina/jump/dodge/interact/melee/ranged/scan/capture/inventory/equipment/consume/build/dismantle/repair/death/respawn/save/load/pause — 28 runtime actions, no duplicate bindings | GLM | VERIFIED (static) | mechanical checklist ALL PASS + input map integrity |
| FCR-3 | Survival/inventory/crafting verification (Phase 3) | all pillars present; exploit paths closed by FCR-1 fixes (negative-qty, offline mint, crop regrow, refund gaps); zero dead recipe stacks | GLM | VERIFIED (static) | recipe cross-check script |
| FCR-4 | Echo platform verification (Phase 4) | ONE architecture confirmed (single AAstrawildEchoCharacter, no duplicates); capture→work→save loop live; locomotion possess-race fixed | GLM | VERIFIED (static) | R2 sweep + audits |
| FCR-5 | Combat verification (Phase 5) | full matrix present; friendly fire + wild bolt damage + combo boss resolution fixed this run | GLM | VERIFIED (static) | |
| FCR-6 | Base/power/automation (Phase 6) | BUILD→POWER→ASSIGN→WORK→PRODUCTION→OUTPUT→STORAGE chain live; garrison caps enforced; offline gates closed | GLM | VERIFIED (static) | |
| FCR-7 | World 12 zones (Phase 7) | zone data + runtime generation + transitions verified (prior audits + validator) | GLM | VERIFIED (static) | |
| FCR-8 | NPC/quest/story (Phases 8-9) | MQ-01..17 + endings A/B + post-game; all 11 objective types have matchers + producers; schedule origin-march + profession + shop-hours fixed this run | GLM | VERIFIED (static) | |
| FCR-9 | Dungeons/bosses/skiff (Phases 10-11) | 3 dungeons, 4 bosses, skiff + Stratos Coil gate verified | GLM | VERIFIED (static) | |
| FCR-10 | UI/UX (Phase 12) | all player-facing screens present; ability/combo toasts wired this run | GLM | VERIFIED (static) | |
| FCR-11 | Content + pipeline (Phases 13-14) | manifest current (459 LFS + ArtSource direct); import idempotent (does_asset_exist guards); pipeline idempotency contract documented (HANDOFF §20a) | GLM | VERIFIED (static) | |
| FCR-12 | Test quality (Phase 15) | 102 tests, exact validator gate, 3 new FCR regressions, no tautologies (all drive real code paths) | GLM | COMPLETE | |
| FCR-13 | Performance audit (Phase 16) | tick scan clean (turret cadence-gated, spoilage one-pass); O(N^2) CastPartyAbility hoisted; perf manager respects user pins | GLM | COMPLETE | |
| FCR-14 | Deferred review (Phase 17) | CV-4 gamepad BLOCKED (hardware) stays · CV-5 economy CLOSED (Duskmoth loot) · CV-6 co-op P3 stays by design · SCP-7 pooling/TeamAgent/RPC stays deferred (no static evidence it is required; ownership checks already prevent friendly fire) | GLM | COMPLETE | no deferred item blocks READY_FOR_FINAL_BUILD |

## §H ASSET ACQUISITION PACK (2026 session — free CC0 source assets)

Directive: acquire, validate, deduplicate, organize and document legally usable
FREE assets (Kenney priority-1) that materially improve ASTRAWILD's
visual/audio quality — WITHOUT touching gameplay code, bindings or the
existing ArtSource contract. Sources: kenney.nl official publisher downloads
only; per-pack CC0 license verified on each pack page + in-archive License.txt.

| ID | Area | Deliverable | Owner | Status | Notes |
|----|------|-------------|-------|--------|-------|
| AA-1 | Pipeline | Reusable acquisition tooling `Scripts/download_assets.py` + `Scripts/download_assets.ps1`: approved-URL-only downloader (retry/partial-detection/zip integrity), SHA256 dedup, path-traversal/symlink/zip-bomb-safe extraction, WAV/OGG/GLB/GLTF/PNG format validators, OGG→WAV 16-bit PCM conversion (originals preserved), idempotent commit (never overwrite; differing content = BLOCKED) | GLM | IMPLEMENTED | 6 packs processed; second-run idempotency verified (identical stats, no duplicates) |
| AA-2 | Audio | Kenney CC0 packs: Impact Sounds (130), Interface Sounds (100), Sci-fi Sounds (73) → `ArtSource/Audio/Kenney_*/` (Ogg originals + Wav conversions + License.txt) | GLM | IMPORT_READY | UE5 imports the WAVs; OGG kept as provenance (UE5 does not import OGG) |
| AA-3 | Models | Kenney CC0 packs: Nature Kit (314 GLB — biome/farm/village/ruins dressing), Space Kit (107 GLB — dungeon/ancient-tech dressing, turret candidates), Blaster Kit (40 GLB + colormap — CANDIDATE_REPLACEMENT weapon pool) → `ArtSource/Models/Kenney_*/GLB/` | GLM | IMPORT_READY | FBX/OBJ/DAE/STL format duplicates + 2D previews dropped at selection; space-kit character/vehicle/rocket models excluded (deferred scope) |
| AA-4 | Documentation | `ASSETS_CREDITS.md` + `ASSET_MANIFEST.json` (root), `Docs/ASSET_ACQUISITION_REPORT.md` + `Docs/ASSET_ACQUISITION_MANIFEST.json`, `Docs/ThirdPartyLicenses.md` rows for all 6 packs | GLM | COMPLETE | 1,071 accepted records / 763 import-ready / 43.4 MB; 4 in-pack duplicates detected+skipped; 61 files rejected by curation; 0 missing dependencies; 0 blocked |
| AA-5 | Guardrails | No `.uasset`/`.umap` fabricated; existing ArtSource assets untouched (never auto-replaced); pack subfolders are outside the flat auto-import folders so `import_all.py` contract unchanged; both static validators re-run PASS at the acquisition commit | GLM | VERIFIED (static) | Engine import/binding belongs to the Antigravity one-time integration (IMPORT_READY ≠ UE5_VERIFIED) |
| AA-6 | Textures P0 | Batch 2 (wayfinder-approved): Kenney CC0 Particle Pack (96 transparent-background VFX sprites + pre-rotated frames; the baked-black-bg duplicate set excluded by curation) + UI Pack: Sci-Fi (690 panel/button/icon PNGs across 6 color families × Default/Double states + Kenney Future/Narrow TTF fonts for UMG) → `ArtSource/Textures/Kenney_*/` | GLM | IMPORT_READY | Feeds the P0 combat-VFX and UI-art gaps from the acquisition gap analysis; SVG vector sources and preview/sample images dropped at selection |
| AA-7 | Models P0/P1 | Batch 2: Kenney CC0 Survival Kit (80 GLB — camps/fires/crates/tools, all 12 zones), City Kit Industrial (38 GLB — Ember Ridge/Stormcrest/research props), Modular Space Kit (41 GLB — dungeon modular tiles), Modular Dungeon Kit (40 GLB — stone/ancient tiles), Animated Characters: Survivors (4 FBX — 1 medium humanoid + idle/run/jump animations, retarget reference) → `ArtSource/Models/Kenney_*/` | GLM | IMPORT_READY | ACS ships NO GLB (classic FBX + 2D-skin pack — research inference corrected at acquisition against the actual archive); FBX validated by container magic, mesh/rig check belongs to engine import |
| AA-8 | Textures P1 | Batch 2: Kenney CC0 Skyboxes (5 equirectangular PNGs — day/morning/night/alien/space) + Crosshair Pack (1,600 reticle PNGs in 4 styles × 2 resolutions, sub-path preserved) → `ArtSource/Textures/Kenney_*/` | GLM | IMPORT_READY | Tilesheet atlases dropped (duplicates of the individual PNGs); Sample renders/Previews excluded; reticles replace the text-glyph HudWidget crosshair at integration |
| AA-9 | Pipeline extension | Batch 2 tooling: new `Textures` category with sub-path-preserving destinations (style/state folders reuse base filenames — flat dests collided, caught and fixed with 0 remaining BLOCKED), FBX + TTF validators, rel-path curation matching, incremental manifest merge (`--packs` subset runs still regenerate the single authoritative manifest; stale-pack records dropped) | GLM | IMPLEMENTED | Idempotency re-proven on batch-2: re-run identical stats (3,678/3,360/75.8MB, 0 blocked), no new files; merge carried batch-1 records verbatim (1,071/43.4MB match the v3.9 commit exactly) |

Rejected (documented in the report): Sci-Fi RTS (2D sprite pack — REJECTED_FORMAT), Digital Audio (8-bit aesthetic), UI Audio (duplicate role vs Interface Sounds), RPG Audio (fantasy-specific), Kenney 2D creature/character family (Monster Builder/Animal/Alien UFO/Robot/Fish/Toon/Shape — REJECTED_FORMAT, no 3D creature catalog exists at Kenney), Quaternius newer packs (REJECTED_LICENSE — QAL forbids redistribution). OpenGameArt researched and DEFERRED (machine-parseable per-asset CC0 licenses + anonymous direct downloads verified — viable for a future batch once the downloader gains a per-asset license gate); Quaternius Ultimate Animated Animals DEFERRED (CC0 page, but Google Drive folder delivery is not a direct URL); Poly Haven/ambientCG NOT_ACQUIRED (site-wide CC0 verified, realistic style = P2 upgrade path).

**Batch-2 totals** (approved via wayfinder ticket 03, live user approval): 9 packs / 2,607 accepted files / 32.4 MB — combined with batch 1: **15 packs / 3,678 accepted / 3,360 IMPORT_READY / 75.8 MB** (602 audio + 661 models + 2,396 PNG + 2 TTF + 17 metadata); 54 in-pack duplicates hash-skipped; 172 files rejected by curation; 0 missing dependencies; 0 blocked. Storage: 75.8 MB of the 10 GB soft cap.

## §I DEPTH PASSES (2026 session — user directive "MAKE IT A REAL GAME")

Directive (user, 2026-09-05): ASTRAWILD must become a real, complete game — not a
technically-complete source project. Batch-2 acquisition is done (§H); the remaining
sequence is: CREATURE VISUAL STRATEGY → CONTENT INTEGRATION PREP → GAMEPLAY DEPTH
HARDENING → WORLD DEPTH PASS → NPC/RELATIONSHIP PASS → DUNGEON DEPTH PASS → FINAL
CONTENT READINESS + SOURCE AUDIT → READY_FOR_FINAL_BUILD. All passes EXTEND the
existing GDP/SCP architecture (no second architecture, no canon redesign); census
counts may change only inside coherent batches that update EXPECTED_CENSUS + all
census docs together; engine-side verification remains Antigravity-exclusive.

| ID | Area | Deliverable | Owner | Status | Notes |
|----|------|-------------|-------|--------|-------|
| DP-1 | Creature visuals | `Docs/ASTRAWILD_CREATURE_VISUAL_STRATEGY.md` — Tier A/B/C system over the 229 species: 20 bespoke (12 hero+evolution meshes exist, 8 new: 4 bosses + Gloomfang/Lumewisp/Sprigling/Auroraling), ≈55 archetype-rig Tier B via shared ArtSourceGen rig library (8 body plans), ≈154 procedural Tier C with strengthened identity; deterministic tier rules; import/binding per HANDOFF §20b | GLM | COMPLETE | SOURCE-VERIFIED strategy (v1.2); all mesh work lands as IMPORT_READY via ArtSourceGen only — never .uasset forgery + P0 boss meshes ×4 delivered (V25-C1) + P0 story meshes ×4 (V25-C2) — Tier-A bespoke set complete (14 echo meshes IMPORT_READY) + boss opt-in skeletal path with cone fallback (source-side) + HANDOFF §20c verbatim binding-patch sequence (assets-first, binding-second) |
| DP-2 | Integration matrix | `Docs/ASTRAWILD_CONTENT_INTEGRATION_MATRIX.md` — 14-category readiness matrix + per-pack "where is this used" tables + P0/P1 gap rows | GLM | COMPLETE | 14 categories (native = STATIC_VALIDATED, packs = IMPORT_READY, boss/NPC-body/Tier-A meshes = MISSING) + 15-pack tables with NOT_INTEGRATION_SCOPE palette subsets (crosshairs/UI families/audio extras) + batch-2 purpose mapping + gap-closure ledger; nothing BOUND — binding ENGINE_UNVERIFIED per §20b; §20b "602 WAV" corrected to 301 importable WAV + 301 OGG provenance |
| DP-3 | Echo depth | locomotion signature abilities (53 templates: +6 water/aerial +3 family; Water/Flying species carry a 7th signature pick), 15-pair party element resonance (mitigation/ability-power/status-potency wired into ApplyElementalDamage + ExecuteAbility), water mounts (Aquatic quadruped/serpent sea-riders: MOVE_Swimming in sea zones, SPACE/CTRL dive/surface, shore walks; mount contract test + sea-rider assertions + resonance test 103) | GLM | COMPLETE | test count 102→103 (gate + 6 docs updated together); legacy 3-arg derivation pinned at six by test 101 |
| DP-4 | Player depth | player-chosen 3-slot skill loadout (build identity) on `UAstrawildAttributeComponent` — `BoundSkills` (None = empty slot) + `BindSkillToSlot`/`ClearSlot`/`IsSkillBound`/`GetBoundSkills` with unlock-gate enforcement at bind time; non-empty loadout narrows the Y smart-cast ladder to the bound skills, all-empty (fresh/pre-DP-4 saves) keeps the legacy all-unlocked ladder exactly; selection surface = ESC pause-menu SKILL LOADOUT section (3 cycling slot buttons, pure-C++ UMG, HUD toast feedback) + HUD skill line bound-count readout; save v5 additive (rides the first `FAstrawildAttributeSaveData` row, sanitized import) | GLM | COMPLETE | test 104 `ASTRAWILD.DP4.SkillLoadout`; count 103→104 (validator gate + 5 docs updated together); enum declaration moved above the attribute save struct (additive reorder, no semantic change) |
| DP-5 | Combat depth | creature weak-point windows (Large/Huge wild game only: replicated `bWeakPointExposed` + server ~20s/4s cadence, ×1.5 direct hits before defense subtraction, existing element light pulses during the window, OnRep mirrors, Tiny/Small/Medium + bosses fail closed) · weakness-hit readability (new `OnWeaknessHit` multicast on the ×1.5 branch + "WEAKNESS HIT" HUD toast via the Notify→PushNotification path + ArtPack-bound energy impact cue + log) · per-boss special sets (`EAstrawildBossSpecialSet` + pure `ResolveBossSpecialSet(DefeatEventTargetId)` + params table: bolt count/cadence, blast count/radius, hazard wave count/dps, summon species — shared TickSpecials reads the set; default = byte-exact legacy tuning; wired in InitializeFromBossDefinition + dungeon boss room + Tyrant world spawn) | GLM | COMPLETE | test 105 `ASTRAWILD.DP5.BossSpecialSets`; count 104→105 (validator gate + 5 docs updated together); no new special mechanic types (same four primitives recombined); audio reuses the existing `/Game/Audio/A_Weapon_Impact_Energy` binding (validator check 8 clean) |
| DP-6 | Base depth | work-site coverage for the 11 work types (4 new data-row sites in `BuildWorkSites`: Site_TidebreakerDepot Transport/TidebreakerIsles + Site_VerdantLab ResearchAssist/VerdantReach + Site_StormcrestDynamo PowerGeneration/StormcrestHighlands + Site_HollowBulwark Defense/HollowApproach — all definition-placed outside the camp, covering the 4 highest species-affinity uncovered types; 8 of 10 actionable types covered, Crafting Assistance + Construction documented exceptions) · research branch wiring audited (census premise "7 branchless techs" was stale — `MakeTech` has set `Tech->Branch` since v2-batch-1; DP-6 ships per-tech rationale comments + the 17/17 assignment pin) · production→progression field consumables (Field Ration: timed `Status.RationVigor` stamina-regen through the survival status-effect system via additive `StaminaRegenPerSecond` + `OnConsumeStatus` item field; Pulse Tonic: 30s bottled Hunter's-Focus capture window via additive `CaptureFocusSeconds` — both applied by `ApplyFieldConsumableEffects` on the two consumption paths; each has a recipe mirror + a producing site: depot consumes kitchen meat + farm berries → rations, lab brews tonics) | GLM | COMPLETE | test 106 `ASTRAWILD.DP6.BaseDepth` (registry-backed world-free census — new pattern: ownerless `BuildDefaults`); count 105→106 (validator gate + 5 docs updated together); census deltas: items 76→78, recipes 56→58, work sites 4→8 (techs/loot untouched); Dynamo Hall outputs EnergyCells ×2 (power loop), Bulwark outputs HerbalSalves (combat feed) |
| DP-7 | World depth | zone events for the 7 bare zones (7 new `BuildWorldEvents` rows, one per previously-bare zone — Mist Tide DuskMarsh forced fog + Sprigling flora boost · Cinder Fall EmberRidge Emberfang boost + 3 bonus EmberAsh vents · Dune-Buried Cache Sunscar Loot_POIAncient + 3 RP · Reef Bloom AzureShallows Brinefin×4 + 3 bonus SeaPearl beds · Wreck Surge TidebreakerIsles Loot_POIRuin + hostile Dawnfang×2 pressure · Storm Front Stormcrest forced storm + Magmawing×3 pulse herd · Pearlsong PearlseaReef rare Pearlcrest×1 + 3 RP — all from the EXISTING effect vocabulary, kinds reused, existing loot tables, no new effect types) · per-zone hazard identity (`EAstrawildZoneHazard` appended-only enum + additive `HazardType`/`HazardPressure` descriptor fields; ColdPressure/HeatPressure = ±°C thermal layer ON TOP of the global weather offset consumed by `UpdateTemperature` through the existing cold/heat threshold + insulation bands, AshLung = passive stamina-regen suppression clamped at zero net regen — Frostveil −12° colder than Dawn Fields, Sunscar +12°, Ember Ridge +10°, Hollow Approach ash lung 6, 10 of 12 zones carry an identity, Dawn Fields + Glimmerwood stay hazard-free by design) · zone secrets (4 scanner-gated `SignalSource` POIs mirroring the FrostveilSignalSource gating pattern — Undergate Vault HollowApproach, Machine Coffin Sunscar, Hold Room TidebreakerIsles, Tidecache PearlseaReef — real Loot_POIAncient + 6 RP on discovery) | GLM | COMPLETE | traversal differentiators stay as-is (Eye Gate altitude + sea-zone identities already differentiate traversal; not extended this batch — directive scoped events/hazards/secrets); test 107 `ASTRAWILD.DP7.WorldDepth` (pure zone-table hazard pins + ownerless `BuildDefaults` registry census); count 106→107 (validator gate + docs updated together); census deltas: world events 9→16, POIs 13→17 (loot/species/nodes untouched — events reuse existing tables/species/node ids) |
| DP-8 | NPC depth | affinity-gated dialogue evolution (additive `RequiredMinAffinity` on `FAstrawildDialogueChoice` — 0 = ungated, fail-closed beside the quest/flag conditions in `EvaluateChoiceConditions`; the dialogue component tracks the TALKING NPC via transient weak ptr set/cleared by OpenDialogue/CloseDialogue so the gate reads the LIVE relationship; pure `MeetsAffinityGate` static + 3 evolved trees: Trader Tam Friend-50 supply-line beat that bridges into the shop + Elder Rowan Confidant-75 "old doors" deep lore + Fisher Nima Friend-50 rare-goods beat with shop bridge) · regional knowledge lines (4 NPCs × honest world info in the existing authoring style: Tam trade-route resource map, Rowan far-lands rare-material map + Frostveil star-falls, Guard Sela ungated per-zone hazard map + Acquaintance-25 patrol chart of the three dungeon gates, Farmer Jori zone-event weather almanac — every fact pinned by the real zone-resource/event tables) · all gated choices are one-time beats with real consequences (forbidden flags + research points / bOpenShop shop bridges — existing consequence verbs only, no new types) | GLM | COMPLETE | census stays 11 NPCs / 11 dialogue trees (depth, not clones — user directive); schedule-aware lines SKIPPED by scope rule: the choice-condition vocabulary has NO time/hour concept and adding one would force a new condition type (the task forbids it) — the schedule system itself (IsServiceOpen) is untouched and remains shop-hour gating only; test 108 `ASTRAWILD.DP8.AffinityDialogue` (pure gate resolver + fail-closed component path + live registry census pin of the 4 gated trees); count 107→108 (validator gate + docs updated together) |
| DP-9 | Dungeon depth | per-dungeon room themes (additive `EAstrawildDungeonTheme` on the room template resolved by pure `ResolveDungeonTheme(DungeonId)` from the STABLE save-mapping ids — bootstrapper literals byte-identical, unknown ids fail closed to the legacy unthemed shell; `MakeThemeProfile` data table: tinted floor + perimeter side walls via the ResourceNode MID idiom, themed accent point light, deterministic room-local ArtPack dressing scatter seeded per room+theme (≤6-attempt rejection keeping the encounter clear-space clean, VISUAL ONLY — no blocking scatter in combat rooms, ISM built once at shell time — zero per-tick cost, ONLY existing ArtPack biome/node binding paths — no new /Game/ refs, validator check 8 clean); Underlight = darkest tint + tight ×0.85 footprint + low 260cm oppressive slabs + cliff-shard/spore-bush dressing, Sunken Vault = wide ×1.3 flooded halls + 190cm walls + moss-boulder + glow-reed dressing + water-film floor accent (existing `WaterPlaneActor` through the additive `BuildPlaneAtZ` — thin 20cm walkable film, never a progression blocker at the gate line), Eye = tall 520cm monolith shells + granite rocks elongated ×2.6 into standing pillars + spore-bush/ancient-vein (tech) accents + pulsing storm accent light on the room's existing 0.5s tick) · puzzle room substance = resonance pillars (3 interactable `AAstrawildResonancePillarActor` per puzzle room mirroring the portal interactable pattern — traceable collision, replicated lit state, prompt carries the required order numeral; the ROOM runs the sequence state machine over pure tested verbs: attunement must follow pillar order I→II→III within a 45s window, wrong order OR window expiry resets all three, sequence solved + light guard defeated = room clear → gate unseals through the existing room-clear path; fail-open when pillars fail to spawn so progression never stalls; pillars lock lit on clear/restore) · per-dungeon room hazards while uncleared (server-side on the room's existing tick, cleared rooms shed the effect — statuses explicitly removed, pulse tiles destroyed): Underlight = mild ash-lung 4/s stamina-regen suppression through additive `FAstrawildStatusEffect.StaminaRegenPenaltyPerSecond` consumed CLAMPED at zero net regen (the DP-7 zone-verb contract, summed with the zone penalty), Sunken Vault = `Waterlogged` 0.8× movement slow through the existing Chill/Shock `SpeedMultiplier` status vocabulary, Eye = periodic energy-pulse tiles (3 tiles / 9s cadence / 170cm radius / 3 dps / 4.5s lifetime, deterministic rotating ring) reusing the existing `AAstrawildBossHazardActor` arena-hazard pattern (armor-mitigated, self-dissipating) | GLM | COMPLETE | boss differentiation deliberately NOT re-touched (DP-5 per-boss special sets remain the boss identity — room hazards are room-side, mild by design and don't alter boss tuning); traversal mechanics stay as-is per DP-7's scope note (Eye Gate altitude + sea-zone identities differentiate traversal); room-owned spawnlings (pillars/tiles/water accents) die with the room via EndPlay so regeneration never leaks; room shell visuals are server-authored in the shared single-player/listen-server world — same posture as the pre-DP-9 floor plate (Template not replicated); test 109 `ASTRAWILD.DP9.DungeonIdentity` (pure statics: 3 canonical ids → 3 distinct themes + unknown fail-closed, profile pairwise-distinct identity data + hazard bands + ArtPack-resolvable dressing vocab + unthemed legacy default, pillar sequence verbs + window contract); count 108→109 (validator gate + docs updated together); census UNCHANGED (dungeon/room counts are not census metrics — species/quests/POIs untouched) |
| DP-10 | Final gate | content readiness matrix re-verified at 14 categories against the post-depth-pass source (ECHOS: 14 Tier-A meshes IMPORT_READY + locomotion signature identity; PLAYER: skill loadout live; NPCS: affinity-gated dialogue; DUNGEONS: themed rooms/puzzles/hazards source-side; VFX/UI/WEAPONS unchanged; statuses stay honest — nothing became BOUND by docs), full source audit at tip (both validators PASS: validate_repository.sh v2 + validate_final_run.py 61 checks incl. the 109-test exact gate + 15 census equality gates; doc-consistency sweep across live docs; ArtSource manifest-vs-disk cross-check 120/120 zero drift; git tree clean, no generated files), readiness report re-affirmed READY_FOR_FINAL_BUILD (source-side) with the residual ledger + SOURCE-VERIFIED/STATIC-VERIFIED/ENGINE-UNVERIFIED separation, HANDOFF coherence pass (109 tests as repo truth, §20b/§20c referenced from the §20 sequence, 12 golden-path verify items, 12-point stop-condition list §21), MASTER_CONTROL v5.0, registry §I closed (no orphans) | GLM | COMPLETE | READY_FOR_FINAL_BUILD declared after the 12-point stop condition held at every point; census unchanged at the DP-10 gate (78 items / 58 recipes / 8 sites / 16 events / 17 POIs / 17 techs / 229 species / 11 NPCs / 11 trees / 8 weapons / 10 nodes / 3 robots / 26 buildings / 12 zones / 11 loot / 109 tests); docs-only commit — zero gameplay code touched |

**§I closing note (residual ledger — the honest remainder after DP-1..DP-10):**

- **(a) Tier-B archetype rig library** (~55 spawned-wild/Huge/Epic+ species on procedural
  PMC bodies with strengthened material identity) — P1 source-side art backlog owned by
  `ASTRAWILD_CREATURE_VISUAL_STRATEGY.md` §10; graceful degradation until it lands; not a
  final-build blocker.
- **(b) Engine-side import/binding queue** — HANDOFF §20b (Kenney packs +
  fitness/retarget/tone checks) and §20c (Tier-A/boss creature-mesh binding patch);
  IMPORT_READY ≠ bound; executes only in the Antigravity one-time run (AG-2..5).
- **(c) Open decisions awaiting engine evidence** — Kenney tone verdict, weapon
  replacement (compare-first), particle sprite fitness, ACS retarget, skybox-space
  consumer, UI family/reticle picks (matrix §4b queue D1-D7; each has a pre-committed
  keep-procedural fallback).

No §I row is PENDING or orphaned: DP-1..DP-10 all COMPLETE. Engine-side conversion
remains Antigravity-exclusive (§B AG-1..AG-6).

---

## §J LAN CO-OP PACK (LCP — current session, user directive "PERSONAL LAN CO-OP + FREE ASSET PRODUCTION MODE")

Directive (user, this session): ASTRAWILD is a private personal 4-player LAN co-op game
(1 listen-server host + 3 LAN clients, host-authoritative, free assets with verified
licenses only, no asset-collage identity drift). This REOPENS the source scope after
DP-10's freeze (MASTER_CONTROL v6.0 §1b): every depth-pass canon stays UNCHANGED; all
LCP work extends the existing server-authoritative replication foundation (no parallel
networking architecture, no dedicated server, no MMO systems). Readiness verdict is
SUSPENDED until §J closes, then READY_FOR_FINAL_BUILD re-declares with the expanded
stop-condition list (HANDOFF §21 + new §22 LAN acceptance test). Spec + PART-3 audit:
`Docs/ASTRAWILD_LAN_COOP_SPEC.md`.

| ID | Area | Deliverable | Owner | Status | Notes |
|----|------|-------------|-------|--------|-------|
| LCP-1 | Scope + audit | LAN_COOP_SPEC (product decision §1, PART-3 source audit at 00354da: 43 replicated props/14 classes, 8 Server RPCs, 0 Client RPCs; client world BROKEN-for-clients, co-op save MISSING, session flow MISSING, quest/research/roster client sync PARTIAL — every verdict grep-grounded) + MASTER_CONTROL v6.0 (§1b multiplayer target + scope-guard + ledger + overall status IN PROGRESS-LCP) + MULTIPLAYER.md wave-5 re-audit header + §J ledger opened | GLM | COMPLETE | docs-only batch — zero source changes; DP-era freeze lifted for LCP batches only; engine gates unchanged |
| LCP-2 | Client world build | DELIVERED: bootstrapper replicates + client path (GameState bWorldSeedSynced gate -> BuildClientCosmeticWorld: lighting/terrain/sea/landmarks via dedicated CosmeticStream (seed^salt) so scatter is machine-order-independent; dressing waits for the replicated exclusion-bubble actors (gate = 2 villages+3 generators+9 portals+N POI markers+2 skiffs, 15s timeout fail-open with warning)) + atmosphere/sun/flicker tick locally on built clients (weather read from replicated WeatherState through the new static GetVisibilityMultiplierForState) + gameplay actor replication: ResourceNode (NodeDefinitionId/RemainingQuantity/bInfinite + OnRep visual mirror + 0.5Hz), NPCCharacter (NpcDefinitionId + registry resolve + appearance rebuild), VillageActor (identity props + BuildVillage on all machines), RestPoint (WorldObjectId/bActive), CraftingStationActor (StationId), POIMarkerActor (PoiId + OnRep beacon), DungeonRoomActor (Template + RoomIndex + OnRep themed-shell rebuild — closes the DP-9 "Template not replicated" client gap) | GLM | COMPLETE | tests 109→111 (ASTRAWILD.LCP2.ClientWorldPolicy + ASTRAWILD.LCP2.DressingGate; validator gate + master-control/readiness/handoff/test-inventory updated together); single-player/listen-host behavior preserved (authority path untouched, village BuildVillage now runs on all machines incl. authority = same result); no census changes |
| LCP-3 | Interaction/trade routing | DELIVERED: `ServerInteract` on PlayerCharacter (the single client→server interact choke point — server re-validates alive/interface/range ×1.5 then runs the SAME authority ladder via the extracted ExecuteInteractIntent: dismount-first → mount → generic interactable → capture → evolve, byte-identical for host/standalone) + first Client RPCs on PlayerController (ClientNotify/ClientOpenVendorShop/ClientOpenVendorDialogue/ClientOpenCraftingScreen + NotifyPlayer delivery helper) + ServerVendorTrade (distance ×600 + quantity sanity → authority-guarded TryPurchase/TrySell + result feedback) + ServerSubmitDialogueChoice (ResolveValidatedChoice structural fail-closed + server-side EvaluateChoiceConditions + authority ApplyChoiceConsequences; widget submits node+index) + replicated mount/pilot identity on the pawn (ReplicatedPilotedSkiff/ReplicatedMountedEcho + OnRep weak mirrors) + ServerSkiffPilotInput/ServerMountRiderInput relays (Unreliable, tracked vertical/boost state so held keys never cancel) + NPC/station screen-open routing + capture/evolve feedback via NotifyPlayer + dialogue choice rows carry indices + cheat execs blocked on NM_Client (AreCheatsAllowed) | GLM | COMPLETE | tests 111→113 (ASTRAWILD.LCP3.ServerRoutingSurface reflection contract + ASTRAWILD.LCP3.DialogueValidation fail-closed resolver; validator gate + 5 docs updated together); TryPurchase/TrySell were already authority-guarded — this batch adds the routing they were waiting for || LCP-4 | Per-player persistence | save V6 additive per-player blocks (inventory/equipment/survival/transform/attrs/skills/roster ownership/party/quests/affinity; host block = legacy fields) + roster `OwnerPlayerId` partition + stable PlayerKey + late-join/reconnect restore | GLM | PENDING | +2 tests; additive-only schema rule honored |
| LCP-5 | Client state sync | QuestComponent replication (per-PC state to owning client, HUD delegates fire via OnRep) + research + roster mirrors on GameState (client GameInstance subsystems import on OnRep — read paths unchanged) + unlock/objective notifications | GLM | PENDING | +2 tests |
| LCP-6 | LAN session flow | `UAstrawildLANSessionSubsystem` (HostLANGame → ServerTravel ?listen + autoload; FindLANGames → UDP beacon listener; JoinLANGame/ConnectDirect → ClientTravel) + pause-menu LAN panel + HUD mode line (SINGLE PLAYER / LAN HOST / LAN CLIENT) | GLM | PENDING | +2 tests (beacon packet encode/decode + address parsing); Build.cs +Sockets+Networking |
| LCP-7 | Free-asset ledger | `Docs/ASTRAWILD_FREE_ASSET_LEDGER.md` (every acquired asset: source/URL/license/license URL/date checked/usage/attribution/status per user PART 10/19 columns) + approved Quaternius Ultimate pack acquisition attempts (verify CC0 per pack before download; Poly Haven/Freesound = recorded coverage decisions) | GLM | PENDING | extends Scripts/download_assets.py; LICENSE_UNCLEAR never enters the repo |
| LCP-8 | Final gate | validators re-run ×2 (test gate 109→N with all census docs updated together) + HANDOFF §22 LAN acceptance test (host + 3 clients golden path) + READINESS re-affirmation with expanded stop-condition list + §J close + worklog | GLM | PENDING | docs + validator only; no new features after this gate |

**§J ground rules** (from the directive, binding): server-authoritative mutation
everywhere; host owns the world save; clients never grant themselves items/XP/research/
quest completion/boss rewards/Echo ownership/building ownership; 4-player performance
target only; free assets = license-verified CC0-or-clearly-permitted only; no UE5
integration in this sandbox (Antigravity one-time final run remains the exclusive
engine gate, now with §22).
