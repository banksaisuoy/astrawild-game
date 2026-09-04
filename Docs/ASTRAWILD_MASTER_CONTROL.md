# ASTRAWILD — MASTER CONTROL (CANONICAL SINGLE SOURCE OF TRUTH)

**Document Version**: 4.1 (DEPTH PASSES begun — creature visual strategy complete; integration matrix + gameplay/world/NPC/dungeon depth passes in flight per TASK_REGISTRY §I)
**Custodian**: GLM 5.3 — Lead Programmer / Game Architect
**Runtime verification authority**: Antigravity (exclusive — GLM never claims runtime PASS)
**Baseline chain**: `main` (94a398c) ⊂ `agent/antigravity-ue5-v2` (f31f5e1 — PR #4 head) ⊂ `final-completion` (THE integration branch, ALL batches + FINAL-AUDIT A/B/C/D + GDP + SCP + FINAL COMPLETION RUN + ASSET ACQUISITION PACK pushed)
**Last Updated**: 2026 (Depth Pass DP-1 — creature visual strategy; see TASK_REGISTRY §I)

---

> [!IMPORTANT]
> **CANONICAL DOCUMENT DECLARATION**: This file is the SOLE ACTIVE planning/control document
> for ASTRAWILD. Every historical roadmap file listed in §12 is HISTORICAL/SUPERSEDED.
> When any other document conflicts with MASTER_CONTROL, MASTER_CONTROL wins.
> This version supersedes: the Antigravity MASTER_CONTROL v2.0 (was 136 lines, absorbed),
> the GLM staging MASTER_CONTROL v1.7 (mirror retained in glm-staging for reference),
> and the v3.1 recovery edition.

> [!NOTE]
> **REDO COMPLETE (v3.2)**: the Final Completion Run re-implemented FR-1..12 on branch
> `final-completion` (base f31f5e1, per binding rule) and pushed EVERY batch to GitHub:
> BATCH-0 recovery 99e4105 · BATCH-1 hardening 61c45e6 · BATCH-2 Act 3/endings/save V5
> 93ee929 · BATCH-3 polish/canon b9c1bd6 · docs batch (this commit). The static validator
> runs **46/46 ALL CHECKS PASSED**; the automation suite holds **67 world-free contract
> tests**. Everything remains ENGINE-UNVERIFIED until the Antigravity machine runs
> AG-2..5 (HANDOFF §20). Game definition, story canon and specs stay LOCKED as v3.0 —
> this run changed no design, only re-landed and completed it.
> **Push rule (binding, user-issued — honored)**: push after every completed batch.

> [!NOTE]
> **ASSET ACQUISITION PACK (v3.9)**: 6 verified-CC0 Kenney packs (Impact/Interface/Sci-fi
> Sounds + Nature/Space/Blaster Kits) acquired into `ArtSource/Audio/Kenney_*/` and
> `ArtSource/Models/Kenney_*/` — 1,071 source files (43.4 MB), LICENSE_VERIFIED, deduped,
> format-validated (WAV PCM/GLB/JSON+deps), idempotent tooling in
> `Scripts/download_assets.py|.ps1`. **No gameplay code, soft-path binding or existing
> ArtSource asset was touched**; new pack subfolders sit OUTSIDE the flat auto-import
> folders, so the `import_all.py` contract and the zero-asset boot guarantee are unchanged.
> Status is IMPORT_READY (NOT UE5_VERIFIED) — import/binding decisions belong to the
> Antigravity integration run per `Docs/ASSET_ACQUISITION_REPORT.md` §9. Both static
> validators re-ran PASS at the acquisition commit. Full ledger: TASK_REGISTRY §H.

> [!NOTE]
> **ASSET ACQUISITION BATCH 2 (v4.0 — wayfinder-approved, live user authorization)**:
> 9 more verified-CC0 Kenney packs closing the P0 gaps from the acquisition gap analysis —
> Particle Pack (96 transparent VFX sprites), UI Pack: Sci-Fi (690 PNGs + 2 fonts),
> Survival Kit (80 GLB), City Kit Industrial (38), Modular Space/Dungeon Kits (41+40 dungeon
> tiles), Animated Characters: Survivors (4 FBX retarget reference), Skyboxes (5 equirect),
> Crosshair Pack (1,600 reticles) → `ArtSource/Textures/Kenney_*/` (new category) and
> `ArtSource/Models/Kenney_*/`. Batch-2: 2,607 files / 32.4 MB; **combined 15 packs /
> 3,678 accepted / 3,360 IMPORT_READY / 75.8 MB**. 54 in-pack hash-duplicates skipped,
> 172 curation rejects, 0 missing deps, 0 blocked. Same guardrails as v3.9: no code, no
> bindings, no existing asset touched, subfolders outside the flat auto-import, IMPORT_READY
> ≠ UE5_VERIFIED. Quaternius (Drive delivery/QAL), OGA (needs license gate), Poly Haven/
> ambientCG (P2) all deferred-with-reasons. Full ledger: TASK_REGISTRY §H AA-6..AA-9.

---

## 0. Status vocabulary (binding)

```text
PLANNED → IN_PROGRESS → IMPLEMENTED → BUILT → TESTED → UE5_VERIFIED → ACCEPTED
                                                              └→ BLOCKED
ENGINE-UNVERIFIED = implemented + statically validated, but never compiled/run in a real
                    engine (this sandbox has no UE5/MSVC — GLM never fakes these).
```

Runtime evidence classes (Antigravity-owned): raw engine log > synthesized summary > claims.
A declared PASS without a raw log is a CLAIM, not evidence.

## 1. Permanent agent roles

| Agent | Role | Responsibilities |
| :--- | :--- | :--- |
| **GLM** | Lead Programmer | C++, architecture, save/load, quests, AI, economy, tests; this document |
| **Qwen** | Technical Art (optional) | Materials/meshes/animation polish — never blocks the project |
| **Antigravity** | Integration & QA | Windows UE 5.8.2 build, 63-test automation run, playtest, package, push |
| **Sonnet/Reviewers** | Auditors | Findings are inputs; only REAL BUG / STALE DOC / UNPROVEN CLAIM classes act on |

## 2. THE FINAL GAME (locked definition)

**ASTRAWILD** — third-person sci-fi survival open-world creature-tech RPG.

Core fantasy: explore an alien frontier · survive · discover Echo creatures · build a home ·
capture and bond with Echoes · use them in work and combat · research technology ·
automate · push into deadlier regions · learn what the First Dawn colony became ·
enter dungeons · defeat bosses · break (or keep) the Maelstrom Cage · live in the aftermath.

**The complete playable loop (all links live in source):**
NEW GAME → EXPLORE → SURVIVE (hunger/thirst/temp) → SCAN → DISCOVER ECHO → FIGHT / AVOID →
CAPTURE (Resonator) → GATHER → CRAFT (58 recipes) → BUILD BASE (26 pieces incl. floor/roof/door/storage/terminal/turret/farm/pen/incubator) →
POWER → ASSIGN ECHO → AUTOMATE (work sites + robots + drone) → RESEARCH (17 techs, RP-earnable) →
UPGRADE (T0→T5 gear) → QUEST (MQ-01..MQ-17 chain) → DUNGEON (×3) → BOSS (×3 + final) →
REWARD → RETURN → SAVE (schema V5) → CONTINUE → **ENDGAME: Eye of the Maelstrom →
The Drowned Sovereign → homecoming → ENDING CHOICE (A/B)** → **POST-GAME** (banner + open world).

No cheat-command dependency anywhere in the chain.

## 3. World canon

- **12 zones** (Types.h enum, 4×3 grid, 3.2km × 2.4km), starter Dawn Fields (threat 1) → PearlseaReef/Stormcrest (threat 4).
  All zones have ≥1 POI (Azure Shallows got POI_ShallowsSextant in the Final Run).
- **229 Echo species** (19 authored + 6 evolution targets + 204 bestiary-generated rows).
  The historical "214" was a documentation error — never use it.
- **Element system**: 6 elements, weakness chain **Flora→Ember, Ember→Frost, Frost→Pulse, Pulse→Light;
  Light and Ash have no weakness** (weakness ×1.5, same-element resist ×0.80 — unified across the wild-Echo
  and boss pipelines in the final audit). All 19 authored species, all 204 bestiary-generated rows and all
  4 bosses obey this matrix (FR-3 + final audit H-1/H-2/H-4). The Glass Tyrant's Light weakness is the one
  documented encounter DESIGN exception (Ash otherwise has no weakness — see ProductionContent.cpp).
- **2 villages** (Dawnstead 8 NPCs + Driftwood Landing 3), all 11 NPCs now have dialogue trees.
- **3 dungeons**: Hollow Underlight (5 rooms, Warden), Sunken Vault (4 rooms, Colossus),
  Eye of the Maelstrom (5 rooms, Drowned Sovereign — Final Run).
- **Content totals** (machine-checked census — the single authoritative set, enforced by
  `Scripts/validate_final_run.py` §11 equality gates and re-derived live by the
  engine-side "live census" registry log): **78 items, 58 recipes, 17 techs, 17 quests,
  229 Echo species, 26 buildings, 11 loot tables, 13 POIs, 9 world events, 11 NPCs,
  11 dialogue trees, 8 weapon profiles, 10 resource nodes, 8 work sites, 3 robots**.
  Historical doc counts (67 items / 49 recipes / 12 POIs / 17 buildings) were stale —
  superseded. The automation suite holds **106 world-free contract tests**.

## 4. Final story canon (IMPLEMENTED — was frozen spec v1.7 §11)

The storm over Stormcrest is the **Maelstrom Cage**, built by the drowned civilization to hold
the **Drowned Sovereign** — whose dream the Echoes are. Three anchors hold the cage
(Frostveil Signal Source = the Silence, Sunscar Mirage Stone = the Furnace, Stormcrest Array = the Crown).

| Act | Quests | Content | Status |
| :--- | :--- | :--- | :--- |
| **Act 1 — Awakening** | MQ-01..MQ-07 | survival/capture/combat onboarding, Hollow Underlight + Underlight Warden | LIVE (pre-existing) |
| **Act 2 — The Tidebreaker Road** | MQ-08..MQ-12 | Ember Ridge → isles, skiff, Sunken Vault + Vault Colossus, tech climb | LIVE (pre-existing) |
| **Act 3 — The Storm Crown** | MQ-13..MQ-17 | three anchors + Glass Tyrant, Stratos Coil skiff gate (120m→160m), Eye of the Maelstrom, Drowned Sovereign (2000 HP, Pulse/Light, 3-phase + enrage + adds), homecoming | **IMPLEMENTED (Final Run commit 0ae9764)** |

**Endings** (Maren's final dialogue, gated on MQ-17 completion, one-way):
- **A — The Dawn That Stays** (break the cage): weather pinned Clear forever; sky opens.
- **B — The Storm That Sleeps** (befriend the cage): the storm remains as a chosen shield.
Both roll the HUD ending banner and enter **post-game**: world events, hunts, economy and
dungeons keep running; the ending state persists (save schema V5, `EndingState`).

## 5. Repository & branch topology

```
main (94a398c) ──┐
                 ├─ agent/antigravity-ue5-v2 (f31f5e1, PR #4 open) ── final-completion (HEAD, ALL
                 │     batches + FINAL-AUDIT A/B/C/D pushed)             ← THE integration branch
                 └─ (historical branches: master, release/vertical-slice-v1, PR #1..#3)
```

**PR #4 reconciliation** (directive §27 — classified, not blindly merged):
`520c78e`+`df8df83` input fix = ALREADY-IN-MAIN-LINE · `c65d734` GLM hardening 57/57 = ALREADY-IN-MAIN-LINE ·
`f31f5e1` 115 LFS assets = NEEDED (M0 truth recovery — **LFS verified: all 459 objects resolve, sizes match**) ·
docs/evidence logs = UNVERIFIED claims superseded by re-run on the final SHA.
**`final-completion` = PR #4 content + re-implemented Final Run work. Merging it into main subsumes PR #4.**
(The original `glm/final-run` branch never reached GitHub and no longer exists.)

**Push status**: every batch through FINAL-AUDIT-C is pushed to `origin/final-completion`
(the PAT-less era ended — the binding push-after-every-batch rule is being honored live).
Delivery path: Antigravity pulls `final-completion` (§1 of the HANDOFF) and pushes after
engine integration.

## 5b. GAMEPLAY DEPTH PACK (GDP — user-directed, post-freeze expansion)

User directive (2026-10): the frozen core ran, but the creatures had no per-species combat identity, the player had no growth systems, and the NPCs had no relationship layer — "a game made for testing, not a real one." GDP adds the depth layer WITHOUT touching the frozen canon:

- **GDP-1 Echo Abilities (every creature fights like itself)**: 44 code-default ability templates (24 element-flavored, 4 role kits, 8 family signatures, 8 authored-species signatures). Authored species carry curated `AbilityIds`; all other species derive a deterministic 4-ability loadout from element + role + family. Level-gated unlocks, cooldowns replicated for HUD, AI casts in combat (heal/shield when hurt, offense otherwise), player party-cast on **T**, bosses excluded (own choreography). `Shell` status = real 50% damage halving; negative-DPS statuses = heal-over-time.
- **GDP-2 Locomotion (land / water / flying)**: `EAstrawildLocomotionClass` on EchoDefinition (Auto = derive). Avian family/plan + Floating bodies fly (MOVE_Flying + direct 3D steering, no navmesh); Aquatic family + sea-zone species swim (+40% speed in sea zones, -15% on land). All 210+ species classified by one deterministic rule.
- **GDP-3 Player Attributes + Skills (สเตตัส + สกิวคน)**: five attributes (Might/Vigor/Agility/Instinct/Craft, level 1-10, XP from the actions themselves — hitting, capturing, crafting, surviving). Passive bonuses feed the existing systems (melee dmg, max HP, stamina regen, move speed, capture chance, craft speed, Masterwork 15% refund). Seven milestone skills with a smart-cast ladder on **Y** (PowerStrike/Whirlwind/Dash/SecondWind/HuntersFocus/Masterwork/Overcharge). Saved (additive v5 field, sanitized import).
- **GDP-4 NPC Affinity (สัมพันธภาพ)**: 0-100 affinity per NPC (Stranger → Acquaintance → Friend → Confidant), gained by talking (+2/day) and trading (+1/day), grants up to 15% vendor discount, saved per NPC id (additive v5 field). Schedules already existed (patrol day / campfire night — Batch 8); affinity completes the living-village layer.

Input contract grows 26 → 28 actions (T = party ability cast, Y = player smart-cast; gamepad: right-stick click = party cast). Save schema stays V5 (both new fields are additive arrays; pre-GDP saves load as fresh states). 12 new automation contracts (72 → 84).

## 6. Engine verification evidence ledger (Antigravity-owned)

| Gate | Status | Notes |
| :--- | :--- | :--- |
| MSVC build @ 8313c61 | DECLARED PASS (raw log) | superseded — rebuild on final SHA required |
| Automation 57/57 @ c65d734 | DECLARED PASS (raw log) | 106 tests now (99 SCP-era + 3 FCR regressions + 1 DP-3 resonance + 1 DP-4 skill loadout + 1 DP-5 boss special sets + 1 DP-6 base depth) — re-run required |
| Final-audit static validation | **PASS 46/46 (this sandbox)** | re-run at AG-2 per HANDOFF §4 |
| Cook & package | FAILED at 8313c61 (UBT ExitCode 6) per own log | FZ-A1 blocker — re-run on final SHA |
| Packaged exe runtime | STALE binary evidence (FZ-A2) | re-run on final SHA |
| PIE playable @ 520c78e | DECLARED (boot-level credible) | re-verify story chain on final SHA |

## 7. Final Run implementation matrix (source-level, honest)

Legend: IMPLEMENTED = code written + statically validated. Engine verification pending (§6).

| ID | System | State | Notes |
| :--- | :--- | :--- | :--- |
| W-1 | Player lifecycle (input/camera/Manny/sprint/dodge/respawn) | LIVE | PR #4 line |
| W-2 | Survival (hunger/thirst/temp/status) | LIVE | |
| W-3 | Inventory (weight/stack/equip/slots) | LIVE (PR #4) — FR hardening LOST, redo | negative-qty exploit redo |
| W-4 | Crafting (stations/timed/output guard) | LIVE + FR-0015 refund fix | |
| W-5 | Building (grid/validate/dismantle/save) | LIVE + FR-0007/13 + shell completion | floor/roof/door/storage added |
| W-6 | Power (grid/brownout/battery) | LIVE | audited-OK |
| W-7 | Automation (roster/work sites/robots/drone) | LIVE + FR-0010/16 | chassis persists, roster sanitized |
| W-8 | Research (17 techs/RP economy) | LIVE | full tree affordable (≥318 RP vs 298 spend + Act-3 quests) |
| W-9 | Quests (MQ-01..17 chain) | LIVE + FR-0011/12 + Act 3 | one-active guard; rewards silent |
| W-10 | Capture pipeline | LIVE + feedback (toast+audio) | |
| W-11 | Echo platform (AI/personality/work/evolution) | LIVE | 229 species |
| W-12 | Combat (melee/ranged/elements/status/boss kit) | LIVE | |
| W-13 | Dungeons (×3) | LIVE + Eye of the Maelstrom | saved state, gates, loot |
| W-14 | Bosses (Warden/Colossus/Tyrant/Sovereign) | LIVE | display names fixed |
| W-15 | World (12 zones/weather/events/POIs) | LIVE + Azure POI | |
| W-16 | NPC/villages/dialogue | LIVE + 5 trees | all 11 NPCs conversational |
| W-17 | Skiff | LIVE + Stratos Coil + mesh binding | ceiling gate = Act 3 |
| W-18 | Save/Load | LIVE + schema V5 + FR-0004..10 | day cap, slot fallback, robot chassis |
| W-19 | HUD/UI | LIVE + ending banner + boss labels | |
| W-20 | **Ending + post-game** | **LIVE (FR-5/FR-6 + audit G-2 gate fix)** | ending gated on MQ-17 per canon |
| W-21 | Content pipeline (ArtSource/LFS/import) | LIVE | 459 LFS objects verified |
| W-22 | Tests | 102 world-free contracts | ENGINE-UNVERIFIED until run |

## 8. Verification queue for Antigravity (one-time final integration)

1. `git fetch && git checkout final-completion` (or merge into main — subsumes PR #4)
2. Build: `Engine\Build\BatchFiles\Build.bat AstrawildEditor Win64 Development -project=<repo>\ASTRAWILD.uproject`
3. Run automation: 102/102 expected (incl. `ASTRAWILD.Quest.FinalRunChain`, `ASTRAWILD.Dialogue.EndingChoice`, `ASTRAWILD.Inventory.TransactionSafety`, `ASTRAWILD.Save.SchemaV5Ending`, `ASTRAWILD.Quest.OneShotBackFill`, the 12 GDP contracts: `ASTRAWILD.Ability.*` x5, `ASTRAWILD.Locomotion.Derivation`, `ASTRAWILD.Attributes.*` x4, `ASTRAWILD.NPC.Affinity*` x2, and the 15 SCP contracts: `ASTRAWILD.SCP.*` — full list in `ASTRAWILD_TEST_INVENTORY.md` rows 1-102)
4. PIE smoke: MQ chain HUD tracker · save/load round-trip (schema 5 stamp in log) ·
   `AW.FastForward` to MQ-13+ if needed → verify anchor POIs, Eye Gate at 150 m with coil skiff,
   Sovereign fight, ending banner, post-game weather pin (Ending A).
5. Package: `RunUAT BuildCookRun` — exit 0 required (previous FZ-A1 failure must not recur).
6. Capture raw logs into `Docs/ENGINE_LOGS/raw/` with the final SHA in the filename.
7. Push: `git push origin final-completion:main` (fast-forward if possible; PR #4 closes as absorbed).

## 9. Coding & git rules (binding)

Server-authoritative mutation · event-bus publication for quest-visible facts ·
AddItemSilent for refunds/rewards (no false CollectItem credit) · fail-closed restores ·
additive-only save schema changes · appended-only enums · one active quest ·
world-free automation tests for every fix · smallest-logical-change commits referencing FR-ids.

## 10. Definition of COMPLETE (directive §31 — current standing)

| Pillar | Standing |
| :--- | :--- |
| Gameplay core loop closed | YES (source) — engine verification pending |
| Progression start→endgame | YES — MQ-01..MQ-17 + ending, no dead objectives (static audit) |
| Content asset paths | YES — 459 LFS objects + procedural fallbacks + import pipeline |
| Story reaches defined ending | YES — two endings + post-game state |
| Campaign + endgame dungeons | YES — Underlight/Vault/Eye |
| Bosses with encounter logic + quest integration | YES — 4 bosses incl. final |
| Save persistence model | YES — schema V5, every major system persisted |
| UI player-accessible | YES — HUD/screens incl. ending banner |
| AI complete source paths | YES (echo/hostile/boss) |
| Automation deterministic | YES — scripts + 102 contracts + this document |
| Documentation single control | YES — this file |
| Task registry | YES — ASTRAWILD_MASTER_TASK_REGISTRY.md |
| P0 source blockers | NONE KNOWN (static level) |

**Overall status: NOT_READY (RECOVERY — v3.1).** The Final Run source was lost before push;
FR-1..14 must be re-implemented on `final-completion` (specs fully preserved). When the redo
reaches the v3.0 source state, status returns to READY_FOR_FINAL_BUILD and the one-time
engine integration pass (§8) converts this to GAME-COMPLETE or returns engine-specific defects.

## 11. Known engine-unverified items (honest ledger)

- Door visual state on pure clients (bIsSwitchedOn has no OnRep) — single-player/listen-server correct.
- Imported skiff mesh orientation (glTF Y-up→Z-up assumption) — cosmetic; collision hull unaffected.
- 106 automation tests never executed in a real engine.
- Package/cook success at the final SHA (FZ-A1 failure was at 8313c61).
- Dungeon generator float-precision at 400 m altitude (Eye) — probes use world height; watch PIE log.
- Dedicated-server co-op paths (H-9 batch) remain single-player-first as designed.

## 12. Historical document classification

SUPERSEDED by this file: ASTRAWILD_MASTER_CONTROL.md v2.0 (Antigravity) ·
GLM staging MASTER_CONTROL v1.7 (mirror in glm-staging).
HISTORICAL (read-only reference): ASTRAWILD_PROJECT_MASTER_PLAN_v1 · PRODUCTION_MASTER_PLAN_V2 ·
PRODUCTION_V2_MASTER_PLAN · PLAYABLE_BUILD_MASTER_PLAN_V4 · ULTIMATE_PRODUCTION_ROADMAP_V3 ·
ULTIMATE_GAP_ANALYSIS · IMPLEMENTATION_GAP_REPORT · GL53_SOURCE_AUDIT ·
GLM53_UE5_IMPLEMENTATION_TASKLIST_V5 · BUILD_READINESS_REPORT · MILESTONE_REPORT ·
PROJECT_MASTER_STATUS_AND_GLM_HANDOFF · ENGINE_VERIFICATION_QUEUE · MASTER_PLAN/ (8 files) ·
CONTENT_PACK/* · all system design docs under Docs/ (accurate per their commit date).

## 13. Control ledger

| Date | Entry |
| :--- | :--- |
| 2026-09-02 | v2.0 (Antigravity): 136-line control + task registry, M0 in progress |
| 2026-09-03 | **v3.0 (GLM Final Run)**: sandbox reset recovered (fresh clone); PR #4 audited & subsumed; LFS truth-verified (459/459); 3 source batches landed on glm/final-run (P0/P1 hardening, Act 3 story completion with endings + post-game, world polish); schema V5; 63 tests; static validation suite green; this document supersedes v2.0/v1.7 |
| 2026-09-03 | **v3.1 (GLM RECOVERY)**: second sandbox reset destroyed the unpushed `glm/final-run` work tree — Final Run source LOST (docs survived in glm-staging). Working branch recreated as `final-completion` from PR #4 head f31f5e1 per binding user rule; control docs restored into repo; registry statuses reset to PLANNED (REDO); push-after-every-batch rule adopted; game design/canon unchanged |
| 2026-09-03 | **v3.2 (GLM FINAL COMPLETION)**: FR-1..12 redo landed batch-by-batch on final-completion (BATCH-0..5, all pushed) · Act 3 + 2 endings + post-game + schema V5 + 17 building pieces + 11/11 NPC dialogue · 46/46 static checks · 67 tests · READY_FOR_FINAL_BUILD (source-side) declared · content manifest issued (459/459 LFS, 65/65 /Game refs) |
| 2026-09-03 | **v3.3 (GLM FINAL SOURCE COMPLETION PASS)**: user-ordered full-repo audit (Phases A–V) executed — 5 parallel deep audits (loop/player, echo/save, quest/boss, world/automation, input/UI/MP/perf); **2 CRITICAL + ~13 HIGH + ~25 MEDIUM defects found and fixed** in FINAL-AUDIT-A (1be6e20: drone compile/crash, POI/boss one-shot quest back-fill, MQ-17 ending gate per canon, view-axis ranged aiming, crafting screen wiring, echo owner identity, robot chassis save, camp respawn, CampKitchen spawn, MainMap default map) and FINAL-AUDIT-B (69a1d65: element canon unified across 204 bestiary rows + authored roster + bosses, echo health persistence, species DefeatLoot live, research import sanitize, AI perception-forgotten + fight-back, stranded-party recall, keyboard screen closes, FastForward cheat) · +5 regression contracts (a5aa74d, 72 tests) · docs reconciled to ONE truth (this pass) · canon UNCHANGED (implementation fixed to match canon) · READY_FOR_FINAL_BUILD re-affirmed (source/repository side) |
| 2026-10-XX | **v3.4 (GLM GAMEPLAY DEPTH PACK)**: user-directed depth expansion — GDP-1 Echo ability engine (44 templates, per-species loadouts, AI casting, T-key party cast) · GDP-2 locomotion classes (Land/Water/Flying, true flight) · GDP-3 player attributes + 7 milestone smart-cast skills (Y) + save fields · GDP-4 NPC affinity tiers with vendor discounts + save fields · 12 new automation contracts (72 → 84) · canon UNCHANGED · READY_FOR_FINAL_BUILD re-affirmed (source/repository side) |
| 2026 | **v3.5 (GLM SCP)**: vULTIMATE plan-vs-repo audit closed 15/17 missing systems across 6 batches (SCP-1..6, 99 tests); pooling + TeamAgent deferred with engine-verify-first reasons (see §5c) |
| 2026 | **v3.6 (GLM FINAL GAME COMPLETION RUN — Phase 0)**: registry reconciliation executed against actual source — test count unified to **102** everywhere; authoritative content census established (76 items / 56 recipes / 229 species / 26 buildings / 17 techs / 17 quests / 11 loot / 13 POIs / 9 events / 11 NPCs / 11 dialogues / 8 weapons / 10 nodes / 4 sites / 3 robots) and enforced by new validator §11 equality gates; ContentLibrary completion log converted to a LIVE registry census (hardcoded counts removed — source defect fixed); registry gained GetNumQuests/GetNumLootTables/GetNumNPCs/GetNumRobots accessors; TEST_INVENTORY/READINESS/REGISTRY synchronized to the one truth |
| 2026 | **v3.7 (GLM FCR — Phase 1 audit + fixes)**: 5-agent deep audit of the GDP+SCP code (never previously audited) found 2 CRITICAL compile blockers + 17 HIGH + 13 MEDIUM + 15 LOW defects — ALL verified against source and fixed in FCR-1-A (9bca989: save subsystem pawn-member compile errors, sanity const violation, flying locomotion possess race, party friendly fire, wild bolt damage, echo XP wiring, NPC origin march, offline mint, crop infinite yield, mount stuck states, IVs/Lucky live, shop hours) and FCR-1-B (30e9e44: dead ability kits, status payloads, DDA party direction, combo boss resolution, garrison enforcement, per-player spoilage, validator empty guard, perf user-pin respect) + FCR-1-C (this commit: +3 regression contracts, suite 102, exact validator gate); R2/R7 full-repo sweeps clean; input/recipe/quest-producer cross-checks clean |
| 2026 | **v3.8 (GLM FCR — Phases 2-18 COMPLETE)**: mechanical verification sweep of every player-facing pillar (Phases 2-12 ALL PASS) · cross-cutting invariants clean (R2/R7 sweeps, input map 28 actions no-dup, zero dead recipe stacks, all objective types have producers) · performance tick scan clean · Phase 17 deferred review: CV-5 economy CLOSED (Duskmoth loot), CV-4/CV-6/SCP-7 stay deferred with reasons (none block READY) · pipeline idempotency contract issued (HANDOFF §20a) · suite 102 with exact gate · **READY_FOR_FINAL_BUILD re-affirmed at the final FCR SHA** — one-time engine integration (AG-1..6) remains the exclusive conversion gate |
| 2026 | **v4.0 (GLM ASSET ACQUISITION BATCH 2 — wayfinder charted, live user-approved)**: acquisition decision layer charted as a wayfinder map (`.scratch/` outside repo, 6 decision tickets; gap-analysis + OGA-policy research resolved by parallel subagents — Kenney full-catalog walk 214 packs, OGA YES-WITH-CONSTRAINTS) → batch-2 approved 9 CC0 packs (Particle/UI-SciFi/Survival/City-Industrial/Modular-Space/Modular-Dungeon/Animated-Characters/Skyboxes/Crosshair) · pipeline extended (Textures category with sub-path-preserving dests, FBX/TTF validators, rel-path curation, incremental manifest merge) · flat-dest collision bug caught in-run and fixed (0 BLOCKED at close) · 2,607 new files / 32.4MB, combined 15 packs / 3,678 / 3,360 IMPORT_READY / 75.8MB · idempotency re-proven · HANDOFF §20b acquired-asset checklist issued for the engine run · READY_FOR_FINAL_BUILD unchanged (IMPORT_READY ≠ UE5_VERIFIED) |


### §5c SCP — Systems Completion Pack (v3.5, session 2026)

Plan-vs-repo audit of the vULTIMATE 14-phase directive found 17 missing systems;
six SCP batches closed 15 of them (source-complete, additive; 99 tests at SCP time — 102 at FCR, 103 after DP-3, 104 at DP-4, 105 at DP-5, 106 at DP-6):
SCP-1 DataValidator/AssetFallback/ErrorReporter + Durability/Spoilage (a7a827f) ·
SCP-2 Base Terminal + Creature Sanity (394ac81) · SCP-3 Mounting (edc6b08) ·
SCP-4 Dual-Tech Combos + DDA (6cd29e4) · SCP-5 NPC Schedules + Crops + Offline
Production + Turrets (bbe2e3c) · SCP-6 Genetics + Performance Manager (9864cce).
Deferred with reasons: object pooling (engine-verify destroy path first),
IGenericTeamAgentInterface (co-op perception layer). Full matrix:
Docs/ASTRAWILD_SYSTEMS_COMPLETION_PACK.md.
