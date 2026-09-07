# ASTRAWILD — MASTER CONTROL (CANONICAL SINGLE SOURCE OF TRUTH)

**Document Version**: 9.8 (VISUAL EXPERIENCE PASS COMPLETE — **SOURCE_PRODUCT_FROZEN re-declared**. VIS-001 delivered the cute sci-fi creature experience layer over the v9.6 base: the CUTE/COOL/STRANGE charm spectrum (`EAstrawildVisualBand` + the pure `ComputeVisualBand` rule; live distribution Cute 71 / Cool 41 / Strange 92 across the 204-row bestiary, all three bands in every zone), per-instance personality presentation (roster line + body-language playback rate), the PMC baby-schema cute pass (head fold + surface-projected eye pair — the review-fixed real bug), the 12-zone fauna composition contract (strategy v2.0 §15), and the Aniimo-class research reference with identity guard (§16). Independent 5-question review returned 4 genuine findings — ALL applied (1 real bug + 3 stale claims; packet §9). Census UNCHANGED (15 gates, 229 species); tests 133→134; TRUE_MISSING 0; no new assets. Post-freeze rule re-arms: evidence-driven only (actual UE5 integration failures / runtime bugs / engine-discovered visual defects). The only remaining work = the one-time Antigravity engine integration (§20 + §20b-e + §22 + V2-36; fresh machines start at `Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md`). Prior v9.7 record:)
**Document Version**: 9.6 (FRESH-MACHINE PLAYBOOK PACK — user directive 2026-09-06 "คู่มือ AI บนเครื่องเปล่า": ship a complete blank-machine onboarding pack so ANY fresh Windows machine + its AI agent can go from nothing to fully playing the game — `Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md` (install spine P0→P13: OS/disk/RAM/GPU checks → Git+LFS/Python/VS2022/UE5.8.2 installs → clone+LFS+validators → build → import → PIE → 133 tests → full gameplay verification → save stress → package → LAN → evidence) + `Scripts/fresh_machine_preflight.ps1` (automated readiness gate with make-ready fix references) + `Docs/ASTRAWILD_FRESH_MACHINE_CHECKLIST.json` (43-step machine-readable tick-off) + `Docs/ASTRAWILD_FRESH_MACHINE_AI_DIRECTIVE.md` (paste-ready agent prompt) + the 8 wrapper .ps1 scripts made env-adaptive (UE_ROOT/ASTRAWILD_UPROJECT/ASTRAWILD_REPO/ASTRAWILD_ARCHIVE/ASTRAWILD_PACKAGED_EXE/ASTRAWILD_AUTOMATION_OUTPUT — legacy E:\ defaults preserved exactly, so the documented reference-machine commands still run verbatim). HANDOFF stale test counts 124/126→133 closed in the same batch; README/queue/HANDOFF/LIVE_STATE/registry pointers synced. Validators ALL PASS at the FMP tip. Prior v9.5 record:)
**v9.6 (FRESH-MACHINE PLAYBOOK PACK — user directive 2026-09-06 "คู่มือ AI บนเครื่องเปล่า ทำแผน ทำรายการ คำสั่งสั่ง AI เครื่องเปล่าๆ ไล่ตั้งแต่ติดตั้ง เช็คพื้นที่ เช็คโปรแกรม อะไรไม่พร้อมก็ทำให้พร้อม"): delivered as ONE logical commit (task FMP-1). Four new onboarding artifacts (playbook spine P0→P13 / preflight gate / 43-step JSON checklist / paste-ready agent directive) + 8 env-adaptive wrapper scripts (defaults unchanged — the exact stale-number/false-alarm class this project keeps killing, now applied to PATHS: a fresh machine could never run Build.ps1/Test.ps1 with their hardcoded E:\ layout; the reference machine still can) + HANDOFF current-facing test-count fixes (§9 124→133, §20 step 3 126→133, GDP chain extended to 133-at-DCP) + README/queue/LIVE_STATE/registry pointers. Everything engine-side remains ENGINE-UNVERIFIED — the playbook's whole purpose is converting that honestly on the real machine.**
**v9.5 (DEFERRED COMPLETION PACK — user directive 2026-09-06 "งานที่ถูก defer ทำให้ครบหมด... ตีกรอบเอง เอาที่เล่นได้ก่อน": every deferred-by-design item delivered source-side playable-first — DCP-1 five post-game side quests + one-time NPC offers (quests 17→22) · DCP-2 NG+ (StartNewGamePlus carryover/reset authority, cycle scaling +10% hostile/+15% research capped 5, pause entry — first real bPostGameActive consumer) · DCP-3 ending cinematics (pure-C++ staged shots + letterbox/fade, OnEndingTriggered wired) · DCP-4 Vess/Ione Act 3 NPC pair with real survivor bodies (NPCs/trees 11→13) · DCP-5 toast cue (first PlaySound2D) + journal per-species detail view (knowledge-gated) · DCP-6 gamepad smart-cast chord LB+X · DCP-7 direct-mesh coverage 42→51/229 (9 boss-spare binds) + PCR4 latent 39-assert fixed. Tests 126→133; validators ALL PASS at every commit e90a773..1c1563c; everything ENGINE-UNVERIFIED until ENGINE-RUN-1. Prior v9.4 record:)
**v9.3 (ASSET OVERHAUL & PRODUCTION PIPELINE — user executive directive: NO PLACEHOLDERS / NO PALETTE SWAPS. The interim Procedural Recolor era is CLOSED: every staged mesh in ArtSource/Meshes is now a REAL, uniquely-shaped CC0 model (109 unique source models for 109 assets — strict 1:1, enforced by the fetch script's palette-swap guard). Delivery: Scripts/fetch_free_assets.py (curated catalog + LIVE-VERIFIED Kenney remote-download path + glTF→GLB self-contained conversion + manifest regeneration → 189 entries / 189 present / 0 pending) · 3 survivor armor tiers (T1 Scavenger/T2 Astraite/T3 Singularity Exosuit, rigged 62-bone humanoids with the full locomotion+gun clip set) · 16 real base archetypes (Quaternius monsters/animals: Dragon/Ghost/Wolf/Yeti/Golem…) + 6 hero Echoes + 36 Tier-B species incl. the 3 production bosses (theme-aware deterministic unique-model draw) + 14 showcase bosses · 5 geometry-distinct weapons (Kenney blasters 820/668/1386/1056/1506 verts) · 4 vehicles (Dawn Skiff hover + ground rover + 2 showcase) · 4 ore nodes (crystal clusters + ancient monolith) · 21 env/flora/ruins props (Kenney nature + Quaternius ruins FBX) · import_all.py upgraded (clip_map AM_ renames + PBR node emissive + weapon Muzzle/survivor Weapon_R sockets + direct-binding coverage incl. clips) · build_showcase_map.py + run_overhaul.py + Setup_And_Play.bat (one-click Windows: locate UE → run pipeline → open showcase map). 11 superseded procedural GLBs purged (those species render via real theme-base + unique mutation spec — real geometry, never a recolor). Tier-B table 39→36 in code+test+validator; validators ALL PASS (manifest 100% present / 0 pending / no-dup-sources / CC0 license fields). Engine import itself stays NOT_RUN on this sandbox (no UE) — V2-36 added to the queue: run Setup_And_Play.bat on the Windows machine.)
**v9.5 (DEFERRED COMPLETION PACK — user directive 2026-09-06: "งานที่ถูก defer ทำให้ครบหมด... ตีกรอบเอง เอาที่เล่นได้ก่อน"): every deferred-by-design item delivered source-side, playable-first — (1) DCP-1 SQ-23 batch: 5 standalone post-game quests + 5 one-time NPC offers gated on Quest_FirstDawnAgain (quests 17→22, single-active-quest rule respected); (2) DCP-2 NG+: StartNewGamePlus reset/carryover authority (carries attributes/journal/affinity/defeat-counts/top-3-bond echoes; resets the story incl. the crown choice; same-Vale-by-design seed keep; +10% hostile scale & +15% research per cycle, capped 5; pause entry visible only post-game — the FIRST real bPostGameActive consumer); (3) DCP-3 ending cinematics: pure-C++ staged camera sequence (letterbox + fade + 3 shots + title cards, 18.7s skippable) wired to OnEndingTriggered (was ZERO subscribers) + 0.5s client poll; (4) DCP-4 Vess/Ione: Act 3 NPC pair with distinct real survivor bodies (soft-path fail-closed; census 11→13 NPCs/trees); (5) DCP-5: toast cue (first PlaySound2D, A_UI_Confirm fail-closed) + journal per-species detail view (knowledge-gated pure builder + clickable rows); (6) DCP-6: gamepad smart-cast chord LB+X (UInputModifierChordAction, zero committed bindings changed); (7) DCP-7: direct-mesh coverage 42→51/229 (9 boss-spare binds, Printf-convention paths) + PCR4 latent 39-assert bug fixed (would have failed the first real engine run). Tests 126→133; validators ALL PASS at every commit; all ENGINE-UNVERIFIED until ENGINE-RUN-1. Commits e90a773..1c1563c.**

**v9.2 (FINAL-EXECUTION TRUTH RE-VERIFICATION — every v9.1 claim independently re-proven with actual commands at tip 4daa113; docs are not evidence, filesystem/git/validators are: (1) LFS count corrected 459→**491/491** (OID-matched sha256, 236.5 MB, fsck exit 0 — the SCI commit's 32 LFS source files made the v9.1 "459/459" stale in live surfaces incl. the HANDOFF §3 engine pre-flight; fixed everywhere current-facing, version history keeps era-correct values); (2) manifest counts corrected → 175 entries / 112 present / 63 pending (47 GLB-backed Echo IDs + 16 SK_Base rows, all opt-in engine-import by design, TRUE_MISSING 0); (3) five more unbannered false-100% historical docs bannered (ANTIGRAVITY_TO_GLM_HANDOFF, PRODUCTION_V2_WORKLOG, PROJECT_MASTER_STATUS_AND_GLM_HANDOFF, ANTIGRAVITY_RUNTIME_FAILURES, CP-00_INDEX); (4) mutation wiring re-verified end-to-end — 21/21 mutator methods defined, all call sites genuine (ApplyThemeMaterial@TryActivateSkeletalBody etc.); (5) validators re-run ALL PASS (126-test gate, 15 census gates 229/126); (6) engine rows V2-29..V2-35 stay NOT_RUN — no UE/MSVC on this Linux sandbox, never faked. No code, engine package, or binding changed this round — a truth gate only.)**
**v9.1 (SCI PHASE-2 AMENDMENT — runtime material/theme switch wired for real: the FINAL EXECUTION audit found the M_SciFi_* theme masters were authored by import_echo_bases.py but had NO runtime consumer (data-only material language). Closed with smallest-compatible implementation: FAstrawildEchoMutator::BuildThemeMaterialPath (8 material languages → 8 M_SciFi_* masters, 1:1 distinct) + ApplyThemeMaterial (dynamic material instance per slot on the skinned body, parameterized Tint/PatternTint/GlowIntensity per species, called from TryActivateSkeletalBody, opt-in fail-closed — before import the GLB's own materials stay); import_echo_bases.py now authors 8 masters (was 6 — added M_SciFi_OrganicHide/M_SciFi_FocusCrystal so ALL 8 languages resolve) AND counts material coverage in echo_base_report.json total_missing (a failed master is an ERROR — kills the false-clean-report risk); test 126 extended (section 8b: 8 distinct master paths); themes doc regenerated with the master mapping table. Everything else from v9.0 stands: 204-spec mutation table + mutator + 16 SK_Base_* GLBs + opt-in binding; census UNCHANGED (229); ENGINE-UNVERIFIED until V2-35.)
**v9.0 (SCI-FANTASY MONSTER DIRECTIVE EXECUTED — content development re-opened by explicit user directive AFTER the CURRENT-HEAD ASSET TRUTH AUDIT (Docs/ASTRAWILD_CURRENT_ASSET_TRUTH.md, HEAD 68c2b07, LFS 459/459, 0 true-missing): the 204-species roster regrouped into 8 Sci-Fantasy themes with a runtime mutation system — FEchoMutationSpec table (204 generated rows, Scripts/generate_echo_mutations.py) + FAstrawildEchoMutator (per-part scale / 7 attachment types / 8 material languages / independent pattern tint / 8 persistent element VFX types, deterministic instance jitter) + 16 baked SK_Base_* archetype GLBs (Tools/ArtSourceGen/gen_sci_fantasy_bases.py, rigged + Idle/Move/Hit clips) + opt-in base binding for every bestiary species in ProductionContent + theme sound sets staged (16 CC0 cues) + UE import pipeline import_echo_bases.py (7 element Niagara templates). +test 126 SCI_FANTASY.MutationSystem; census UNCHANGED (229 species — no new species, the directive re-skins the existing 204 into Pokémon-style visual identities via base+mutation); all opt-in fail-closed (PMC mutated body stays until import). ENGINE-UNVERIFIED: base import + VFX + materials + runtime mutation visuals = V2-35 queue row. Post-FPP-3 freeze was superseded BY THIS USER DIRECTIVE only; engine gates unchanged.)
**Custodian**: GLM 5.3 — Lead Programmer / Game Architect
**Runtime verification authority**: Antigravity (exclusive — GLM never claims runtime PASS)
**Canonical LIVE EXECUTION STATE**: `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` (directive MASTER SYNCHRONIZATION 2026-09-06) — the ONLY active task-state document; this rulebook stays canonical for game/product rules. When any status block elsewhere disagrees, LIVE_EXECUTION_STATE wins for execution state.
**Baseline chain**: `main` (94a398c) ⊂ `agent/antigravity-ue5-v2` (f31f5e1 — PR #4 head) ⊂ `final-completion` (THE integration branch: ALL batches + FINAL-AUDIT A/B/C/D + GDP + SCP + FCR + ASSET ACQUISITION (26a7c7b, a09e566) + DEPTH PASSES DP-1..DP-10 (981250d → 00354da) + LAN CO-OP PACK LCP-1..LCP-8 + PRODUCT COMPLETION RUN PCR-0..PCR-6 + PRESENTATION PASS FPP-1 (this commit onward) — all pushed)
**Last Updated**: 2026-09-06 (v9.6 FMP — fresh-machine onboarding pack shipped at the FMP tip; ENGINE-RUN-1 (V2-29..V2-36) remains the only product-critical work, blocked on the Windows UE 5.8.2 machine — see LIVE_EXECUTION_STATE)

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
| **Antigravity** | Integration & QA | Windows UE 5.8.2 build, 133-test automation run, playtest, package, push |
| **Sonnet/Reviewers** | Auditors | Findings are inputs; only REAL BUG / STALE DOC / UNPROVEN CLAIM classes act on |

## 1b. Supported multiplayer target (v6.0 — user product decision, binding)

ASTRAWILD is a **private personal game for the user and 3 friends**.

SUPPORTED (current release scope):
- Single Player (standalone — original first-class path, unchanged)
- LAN Listen Server: 1 host plays + serves, up to 3 LAN clients join
- **4 players total**, one shared authoritative (host) world, UE server-authoritative replication

NOT REQUIRED for the current release (deferred/potential future scope ONLY — do not build):
- Dedicated server · public matchmaking · online account system · cloud backend · MMO-scale networking

Full spec, audit and party rules: `Docs/ASTRAWILD_LAN_COOP_SPEC.md`.
Multiplayer expansion must never explode the project (scope guard, §1b).

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
  All zones have ≥1 POI (Azure Shallows got POI_ShallowsSextant in the Final Run), ≥1 anchored world
  event and an explicit hazard identity (DP-7 — 16 events, 12 hazard rows; Dawn Fields + Glimmerwood
  stay hazard-free by design).
- **229 Echo species** (19 authored + 6 evolution targets + 204 bestiary-generated rows).
  The historical "214" was a documentation error — never use it.
- **Element system**: 6 elements, weakness chain **Flora→Ember, Ember→Frost, Frost→Pulse, Pulse→Light;
  Light and Ash have no weakness** (weakness ×1.5, same-element resist ×0.80 — unified across the wild-Echo
  and boss pipelines in the final audit). All 19 authored species, all 204 bestiary-generated rows and all
  4 bosses obey this matrix (FR-3 + final audit H-1/H-2/H-4). The Glass Tyrant's Light weakness is the one
  documented encounter DESIGN exception (Ash otherwise has no weakness — see ProductionContent.cpp).
- **2 villages** (Dawnstead 10 NPCs — 8 + the DCP-4 Act 3 pair Vess/Ione — + Driftwood Landing 3), all 13 NPCs conversational.
- **3 dungeons**: Hollow Underlight (5 rooms, Warden), Sunken Vault (4 rooms, Colossus),
  Eye of the Maelstrom (5 rooms, Drowned Sovereign — Final Run). Each now reads distinctly IN-ROOM
  (DP-9): per-dungeon themed shells (tint/proportions/side walls), deterministic ArtPack dressing,
  room-level hazards while uncleared (ash lung / waterlogged slow / energy-pulse tiles) and
  resonance-pillar puzzle rooms; boss specials stay DP-5's per-boss sets.
- **Content totals** (machine-checked census — the single authoritative set, enforced by
  `Scripts/validate_final_run.py` §11 equality gates and re-derived live by the
  engine-side "live census" registry log): **78 items, 58 recipes, 17 techs, 22 quests (17 MQ + 5 post-game DCP-1),
  229 Echo species, 26 buildings, 11 loot tables, 17 POIs, 16 world events, 13 NPCs,
  13 dialogue trees, 8 weapon profiles, 10 resource nodes, 8 work sites, 3 robots** (NPCs/trees 11→13 = DCP-4 Vess/Ione).
  Historical doc counts (67 items / 49 recipes / 12 POIs / 17 buildings) were stale —
  superseded. The automation suite holds **126 world-free contract tests** (109 through DP-9 + 2 LCP-2 + 2 LCP-3 + 2 LCP-4 + 2 LCP-5 + 2 LCP-6 LAN co-op + 1 PCR-1 Field Journal + 1 PCR-2 Echo Roster + 1 PCR-3 World Map + 1 PCR-4 Tier-B library + 1 PCR-5 hunt system + 1 FPP-1 presentation contract + 1 SCI-FANTASY mutation contract). The player-facing rulebook is `Docs/ASTRAWILD_PLAYER_RULES.md` (every number sourced from live code at the freeze SHA).

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
| Automation 57/57 @ c65d734 | DECLARED PASS (raw log) | 109 tests now (99 SCP-era + 3 FCR regressions + 1 DP-3 resonance + 1 DP-4 skill loadout + 1 DP-5 boss special sets + 1 DP-6 base depth + 1 DP-7 world depth + 1 DP-8 affinity dialogue + 1 DP-9 dungeon identity) — re-run required |
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
| W-16 | NPC/villages/dialogue | LIVE + 5 trees + 2 DCP-4 trees | all 13 NPCs conversational (11 + Vess/Ione) |
| W-17 | Skiff | LIVE + Stratos Coil + mesh binding | ceiling gate = Act 3 |
| W-18 | Save/Load | LIVE + schema V5 + FR-0004..10 | day cap, slot fallback, robot chassis |
| W-19 | HUD/UI | LIVE + ending banner + boss labels | |
| W-20 | **Ending + post-game** | **LIVE (FR-5/FR-6 + audit G-2 gate fix)** | ending gated on MQ-17 per canon |
| W-21 | Content pipeline (ArtSource/LFS/import) | LIVE | 586 LFS pointers verified (v9.4 re-derivation: 491 pre-v9.3 + 95 v9.3 real-mesh source files; all objects on-disk OID-resolvable, 0 pending push; fsck raw-pack caveat in LIVE_EXECUTION_STATE §8) |
| W-22 | Tests | 126 world-free contracts | ENGINE-UNVERIFIED until run |
| W-23 | **Sci-Fantasy mutation system** (directive SCI) | **LIVE source-side** — 204-spec table + mutator + 16 base GLBs + opt-in binding + runtime material/theme swap (8 masters, dynamic instances on the skinned path); runtime visuals | ENGINE-UNVERIFIED until V2-35 |
| W-24 | **Real-mesh catalog (directive AO v9.3)** | **LIVE source-side** — 109 real unique CC0 meshes (3 armor tiers / 42+16+14 Echo/boss / 5 weapons / 4 vehicles / 4 nodes / 21 env), manifest 189/189 present / 0 pending, 1:1 source uniqueness, Setup_And_Play.bat one-click | ENGINE-UNVERIFIED until V2-36 |

## 8. Verification queue for Antigravity (one-time final integration)

1. `git fetch && git checkout final-completion` (or merge into main — subsumes PR #4)
2. Build: `Engine\Build\BatchFiles\Build.bat AstrawildEditor Win64 Development -project=<repo>\ASTRAWILD.uproject`
3. Run automation: 126/126 expected (incl. `ASTRAWILD.Quest.FinalRunChain`, `ASTRAWILD.Dialogue.EndingChoice`, `ASTRAWILD.Inventory.TransactionSafety`, `ASTRAWILD.Save.SchemaV5Ending`, `ASTRAWILD.Quest.OneShotBackFill`, the 12 GDP contracts: `ASTRAWILD.Ability.*` x5, `ASTRAWILD.Locomotion.Derivation`, `ASTRAWILD.Attributes.*` x4, `ASTRAWILD.NPC.Affinity*` x2, the 15 SCP contracts: `ASTRAWILD.SCP.*`, the 7 depth-pass contracts `ASTRAWILD.DP3..DP9.*`, the 10 LAN co-op contracts `ASTRAWILD.LCP2..LCP6.*`, the PCR contracts `ASTRAWILD.PCR1..PCR5.*`, `ASTRAWILD.FPP1.PresentationContract`, and `ASTRAWILD.SCI_FANTASY.MutationSystem` — full list in `ASTRAWILD_TEST_INVENTORY.md`; the count is read from the repo, never from memory)
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
| Content asset paths | YES — 491 LFS objects + procedural fallbacks + import pipeline |
| Story reaches defined ending | YES — two endings + post-game state |
| Campaign + endgame dungeons | YES — Underlight/Vault/Eye |
| Bosses with encounter logic + quest integration | YES — 4 bosses incl. final |
| Save persistence model | YES — schema V5, every major system persisted |
| UI player-accessible | YES — HUD/screens incl. ending banner |
| AI complete source paths | YES (echo/hostile/boss) |
| Automation deterministic | YES — scripts + 125 contracts + this document |
| Documentation single control | YES — this file |
| Task registry | YES — ASTRAWILD_MASTER_TASK_REGISTRY.md |
| P0 source blockers | NONE KNOWN (static level) |
| **LAN 4-player source support** | **YES (source-side)** — LCP-1..8 complete: client world, interaction/trade routing, per-player persistence + reconnect, client state sync, LAN session flow; ENGINE-UNVERIFIED until §22 |
| **Free-asset ledger (verified licenses)** | YES — `Docs/ASTRAWILD_FREE_ASSET_LEDGER.md` (15 CC0 Kenney + 6 CC0 Quaternius packs, 3,942 files total, per-file SHA-256 manifests, LICENSE_UNCLEAR never enters) |
| **Player-facing UI surfaces (PCR)** | YES — Field Journal [P] / Echo Roster [L] / World Map [M] / Hunt Board [U] + pause-menu paths (gamepad-reachable) |
| **Tier-B creature visual identity (PCR)** | YES (source-side) — 36 species with UNIQUE REAL CC0 meshes (33 Tier-B + 3 production bosses) + definition-driven opt-in binding; the rest render via the real 16-base geometry + unique mutation spec (never a palette swap) |
| **Real-mesh asset catalog (ASSET OVERHAUL)** | YES — 109 unique CC0 models, manifest 189/189 present / 0 pending, per-asset license provenance (Docs/ASTRAWILD_REAL_ASSET_CREDITS.json), live-verified remote fetch path (Kenney) |
| **Post-game hunts (PCR)** | YES — 8 repeatable cull contracts + Hunt Board (the post-game claim is now TRUE) |

**Overall status (v9.8): SOURCE_PRODUCT_FROZEN — VIS-001 complete (charm spectrum + personality + zone fauna identity + review findings applied; READY_FOR_REVIEW → review closed → frozen at 26b7600). The only remaining work is the one-time Antigravity engine integration: §20 build/automation/PIE/package + §20b-e imports + §22 LAN acceptance + V2-36 one-click. Post-freeze = evidence-driven fixes only.**
Historical: SOURCE_PRODUCT_FROZEN (v8.0-era, superseded).
Declared at DP-10, suspended twice (LAN CO-OP scope; PCR product audit),
re-declared at the PCR-6 gate and now **FROZEN at the FPP-1 gate**: the six
closed product gaps (PG-1..PG-6), 125 contracts, census gates unchanged, and
the presentation dead ends closed by FPP-1 — the engine conversion queue is
§20 (build/automation/PIE + the 36 real-mesh Tier-B imports riding the §20b baseline
pass) + §22 (LAN acceptance) + V2-36 (Setup_And_Play.bat one-click engine import
+ showcase-map PIE). Nothing in this document claims engine
verification.

## 11. Known engine-unverified items (honest ledger)

- LAN co-op: all LCP source work is ENGINE-UNVERIFIED until the Antigravity run executes the §22 LAN acceptance test (4 players, host + 3 clients).
- Door visual state on pure clients (bIsSwitchedOn has no OnRep) — single-player/listen-server correct.
- Imported skiff mesh orientation (glTF Y-up→Z-up assumption) — cosmetic; collision hull unaffected.
- 126 automation tests never executed in a real engine.
- Package/cook success at the final SHA (FZ-A1 failure was at 8313c61).
- Dungeon generator float-precision at 400 m altitude (Eye) — probes use world height; watch PIE log.
- Dedicated-server paths remain out of scope by design (§1b — LAN listen server is the target).

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
| 2026 | **v5.0 (GLM DP-10 FINAL GATE — DEPTH PASSES COMPLETE)**: full source audit at tip 018a95a — both validators PASS (validate_repository.sh v2 + validate_final_run.py 61 checks incl. the 109-test gate and the 15 census equality gates); doc-consistency sweep executed (stale current-state counts corrected across the matrix/manifest/registry/readiness/handoff — historical/dated rows untouched); content readiness matrix re-verified at 14 categories (Tier-A meshes IMPORT_READY, skill loadout live, affinity-gated dialogue, themed dungeons — statuses stay honest, nothing BOUND); readiness report re-affirms **READY_FOR_FINAL_BUILD (source-side)** with the residual ledger (Tier-B rig library P1 / engine import+binding queue §20b+§20c / tone-weapon-particle-ACS decisions awaiting engine evidence); HANDOFF coherence pass (109 tests as repo truth, §20b+§20c referenced from the §20 sequence, 12 golden-path verify items, 12-point stop-condition list from the user directive); registry §I closed — DP-1..DP-10 all COMPLETE, no orphans; branch frozen for the Antigravity one-time integration run |
| 2026 | **v6.1 (GLM LCP-8 — LAN CO-OP PACK COMPLETE, READY_FOR_FINAL_BUILD re-declared)**: LCP-2 client world (deterministic cosmetic build from the replicated seed + replicated gameplay actors incl. the DP-9 client-shell fix) · LCP-3 interaction/trade routing (ServerInteract choke point, first Client RPCs, fail-closed dialogue/trade validation, mount/pilot input relays, cheat client gate) · LCP-4 per-player persistence (coop save blocks, roster owner partition, stable player keys, late-join/reconnect) · LCP-5 client state sync (quest replication, research mirror, unlock/completion feedback everywhere) · LCP-6 LAN session flow (UDP beacon, pause panel, HUD mode line, travel-autoload) · LCP-7 free-asset ledger + 6 CC0 Quaternius Ultimate packs (264 files, dual license gates, Drive-crawl downloader) · LCP-8 this gate — suite 109→119, validators PASS ×2, HANDOFF §21b+§22, READINESS §O, registry §J closed; engine queue = AG-2..5 + §22 |
| 2026 | **v6.0 (GLM LCP-1 — LAN CO-OP PACK OPENED)**: user-issued product decision reopens scope: ASTRAWILD is a private 4-player LAN co-op game (listen server, host-authoritative, free-asset production mode) — §1b added, `ASTRAWILD_LAN_COOP_SPEC.md` issued (full PART-3 source audit: 43 replicated props / 14 classes, 8 Server RPCs, 0 Client RPCs; client-visible world BROKEN-for-clients, co-op save MISSING, session flow MISSING, quest/research client sync PARTIAL — every verdict source-grounded), MULTIPLAYER.md refreshed to the audit truth, registry §I reopened with the LCP-1..LCP-8 ledger; DP-era freeze lifted for LCP batches only; depth-pass canon UNCHANGED; engine gates unchanged (AG-2..5 + new §22 LAN acceptance) |
| 2026-09-06 | **v9.5 (GLM DEFERRED COMPLETION PACK — user directive: re-open ALL deferred work, playable-first)**: DCP-1 SQ-23 post-game quest batch (5 standalone quests + 5 gated one-time NPC offers; quests 17→22 census-synced with docs/validator) · DCP-2 NG+ (StartNewGamePlus — carries attributes/journal/affinities/defeat-counts/top-3-bond echoes, resets story incl. crown choice + research + inventory veteran kit + dungeon regeneration; hostile scale + research multiplier world-free statics, additive v5 save field NGPlusCycle; pause button visible only post-game — the first real bPostGameActive consumer) · DCP-3 ending cinematics (UAstrawildEndingCinematicComponent + UAstrawildEndingLetterboxWidget — 3 transient camera shots + first SetViewTargetWithBlend in module + letterbox/fade/title cards, 18.7s skippable, host OnEndingTriggered broadcast + 0.5s client poll, one guarded entry, presentation-only) · DCP-4 Vess/Ione (NPC_Vess storm-scholar + NPC_Ione relic trader in Dawnstead roster 8→10, real survivor soft-path bodies fail-closed to procedural look, Dialogue_Vess 7-node progressive Act 3 lore + Dialogue_Ione one-time glass gift; NPCs/trees 11→13 everywhere) · DCP-5 (toast A_UI_Confirm PlaySound2D fail-closed + journal clickable rows + BuildSpeciesDetailText knowledge-gated per-species detail view) · DCP-6 (LB+X chord via UInputModifierChordAction — no committed binding changed, INPUT_REFERENCE updated) · DCP-7 (GetEchoArt 6→15: 9 priority species ← boss spare pool, Printf-convention fail-closed paths; direct-mesh 42→51/229; PCR4 latent 39-assert fixed — would have failed the first real engine run) · tests 126→133 (7 new DCP contracts), validators ALL PASS at every commit e90a773..1c1563c; everything ENGINE-UNVERIFIED until ENGINE-RUN-1 |
| 2026 | **v9.1 (GLM SCI-AMEND — FINAL EXECUTION audit round: runtime material swap wired)**: user FINAL EXECUTION directive (Phases 1-10) found the one real dead-end left in the Sci-Fantasy implementation — the 6 M_SciFi_* theme masters existed only as import-script authoring with NO runtime consumer (EAstrawildMutationMaterialTheme was data-only on the skinned path; the PMC path's vertex-color language was the only live material expression). Closed source-side: `BuildThemeMaterialPath` + `ApplyThemeMaterial` (8 languages → 8 distinct masters 1:1; dynamic material instances over every skinned slot, Tint = species primary pushed toward theme tint / PatternTint = spec's independent tint / GlowIntensity = language default; called from TryActivateSkeletalBody; opt-in fail-closed) · import_echo_bases.py 6→8 masters (M_SciFi_OrganicHide + M_SciFi_FocusCrystal close the Organic/Crystalline gap) + material coverage now REQUIRED in echo_base_report.json (missing master = ERROR + total_missing counts it — the report can never look clean while a language is unresolvable) · test 126 §8b (8 distinct master paths, full-package form) · themes doc regenerated with the master mapping table (mutation table byte-identical — generator determinism re-proven) · V2-35 acceptance extended (theme material swap visible in PIE) · dashboard-truth sweep (PROJECT_STATUS_SUMMARY historical banner, README/BUILD_STATUS/PLAYABLE_BUILD_STATUS stale pointers fixed to v9.1 truth) · validators ALL PASS at the new tip; census/test-count gates unchanged (229/126) |
| 2026 | **v9.3 (GLM ASSET OVERHAUL & PRODUCTION PIPELINE — NO PLACEHOLDERS / NO PALETTE SWAPS, per user executive directive)**: the interim procedural-recolor mesh era is closed — `Scripts/fetch_free_assets.py` replaces EVERY staged ArtSource mesh with a REAL uniquely-shaped CC0 model (1:1 source uniqueness enforced: 109 assets ← 109 distinct Quaternius/Kenney models, 0 reuses; live-verified Kenney remote-download pattern; glTF→self-contained-GLB conversion with embedded images; deterministic theme-aware Tier-B draw; manifest regenerated → **189 entries / 189 present / 0 pending**, stale absolute paths normalized; per-asset license provenance in ASTRAWILD_REAL_ASSET_CREDITS.json) · player armor tiers T1 Scavenger (Adventurer) / T2 Astraite (Spacesuit) / T3 Singularity Exosuit (Swat) — rigged 62-bone humanoids, clip_map Idle/Walk/Run/Roll→Jump/Idle_Gun_Pointing→Aim/Gun_Shoot→Fire/Interact→Gather · 16 real base archetypes + 6 heroes + 36 Tier-B (incl. GlassTyrant←Frog, EyeSentinel←Astronaut_Flamingo, DrownedSovereign←BlobYeti) + 14 showcase bosses · 5 geometry-distinct weapons + Dawn Skiff/Ground Rover/support vehicles + 4 ore nodes (crystal clusters) + 21 env props (Kenney nature + Quaternius ruins FBX) · `import_all.py` upgraded (rename_clips_by_map clip-map AM_ renames, apply_node_emissive PBR glow per ore type, weapon Muzzle + survivor Weapon_R sockets, coverage = meshes + clips direct-binding evidence) · NEW `build_showcase_map.py` (PlayerStart + armor podium + hero row + base grid + Tier-B grid + boss arena + weapon rack + vehicle pad + node garden, idempotent) + `run_overhaul.py` + **`Setup_And_Play.bat`** (Windows one-click: locate UE 5.x → verify catalog → run pipeline → editor opens showcase map) · 11 superseded procedural GLBs purged (species render via real base + unique mutation spec) · Tier-B code table + PCR-4 test + validator gate 39→36 (+prod-boss membership asserts + 8 new ASSET-OVERHAUL gates: manifest 100% present, no-dup sources, files-on-disk, CC0 fields, 14 bosses, 3 tiers, 5 distinct weapons) · validators ALL PASS at the new tip; census/test-count gates UNCHANGED (229/126) · engine import stays NOT_RUN here (no UE on the Linux sandbox) — **V2-36** added: run Setup_And_Play.bat on the Windows machine, evidence = import_report.json (total_missing 0 incl. clips) + showcase PIE |
| 2026 | **v9.2 (GLM FINAL-EXECUTION TRUTH RE-VERIFICATION — stale-count + false-100% closure)**: independent re-verification of every v9.1 claim with actual commands at tip 4daa113 (docs are not evidence — filesystem/git/validators are): (1) **stale LFS count found & fixed** — the SCI commit's 32 LFS-tracked source files (16 SK_Base_*.glb + 16 SFXSet_*.wav) made the live count 491, but v9.1 surfaces still cited the pre-SCI 459/459 · 233.0 MB; re-derived 491/491 objects on disk, sha256 OID-match verified (all 32 SCI + 6 random), 236.5 MB payload, git lfs fsck exit 0; Content binaries re-walked 416/416 genuine UE package magic (unchanged — GLB/WAV are raw sources); all current-facing 459→491 (README, W-21, §N, readiness, HANDOFF §3 pre-flight `expect 491`, manifest v1.6, asset-truth final block); version-history rows keep era-correct 459 as dated history · (2) **stale manifest counts fixed** — ArtSource/manifest.json is now 175 entries (159 + 16 SK_Base rows) with 112/175 ue_paths present and 63 pending (= 47 GLB-backed Echo IDs + 16 SK_Base base meshes, both import at the §20d/V2-35 + V2-29 engine passes, opt-in by design, TRUE_MISSING 0) · (3) **5 more unbannered false-100% surfaces closed** — ANTIGRAVITY_TO_GLM_HANDOFF (“100% VERIFIED” status line), PRODUCTION_V2_WORKLOG (48-test era), PROJECT_MASTER_STATUS_AND_GLM_HANDOFF (53-test era), ANTIGRAVITY_RUNTIME_FAILURES (“100% OPERATIONAL”), CONTENT_PACK/CP-00_INDEX (inline 48/48) — all now carry HISTORICAL/SUPERSEDED banners pointing at this doc; every remaining 100% in the repo is either banner-qualified history or a factually-true fraction (LFS 491/491, gameplay refund rates, census fractions) · (4) readiness §14 test count 125→126 + §17 contradiction row re-derived · (5) mutation wiring re-verified end-to-end (21/21 mutator methods defined; FindSpec@PMC/skinned, ComputePartScales, AppendMutationParts, ApplyThemeToBodyColors, ApplyElementVfx, ComputeRootScaleJitter, ApplyThemeMaterial@TryActivateSkeletalBody, BuildSoundSetCuePath@vocal-cue — all genuine call sites); validators re-run ALL PASS (126-test gate, 15 census gates 229/126, 204-row table, 16 bases, 16 cues on disk) · (6) engine rows V2-29..V2-35 stay NOT_RUN (no UE/MSVC on this sandbox — re-verified; never faked) |
| 2026 | **v9.0 (GLM SCI — SCI-FANTASY MONSTER DIRECTIVE, post-audit content re-open)**: user directive ordering 204 Sci-Fantasy Echo identities + runtime mutation AFTER the CURRENT-HEAD ASSET TRUTH AUDIT (ASTRAWILD_CURRENT_ASSET_TRUTH.md: remote HEAD 68c2b07 not 4bb7be5, LFS 459/459 OID-verified, 416/416 genuine UE packages, 47 GLB ue_paths pending import, TRUE_MISSING=0) → Phase 1: 8-theme regroup (Ancient Constructs 24, Elemental Beasts 50, Mutated Fauna 16, Armored Organics 11, Ethereal Spirits 26, Mechanical Hybrids 8, Plant Monsters 40, Void Abominations 29) + Docs/ASTRAWILD_SCI_FANTASY_THEMES.md; Phase 2: FEchoMutationSpec (204-row generated table via Scripts/generate_echo_mutations.py, parsed from the ACTUAL bestiary) + FAstrawildEchoMutator (AstrawildEchoMutator.h/.cpp — theme/attachment/material/VFX vocabularies, per-part scale + instance jitter, theme color math, derived opt-in paths, persistent element VFX) + EchoCharacter integration (PMC per-part scaling + AppendMutationParts attachments + theme palette; skinned root jitter; weakness-hit sound-set cue hook); Phase 3: 16 SK_Base_* GLBs baked (gen_sci_fantasy_bases.py on the Tier-B rig conventions, theme identity geometry, manifest-recorded) + tools/download_scifi_fantasy_assets.py staging (16 CC0 theme cues from local Kenney packs + curated-source ledger incl. Poly Haven REJECTED-with-reason and Sketchfab APPROVED-NOT-REQUIRED) + ASTRAWILD_SCI_FANTASY_ACQUISITION.json; Phase 4: Content/Python/AwPipeline/import_echo_bases.py (REQUIRES LOCAL EXECUTION — 16 skeletal imports + clip normalization + 16 SoundWave cues + 7 NS_AW_Elem_* templates + theme master materials built on MaterialEditingLibrary); Phase 5: opt-in base binding in ProductionContent for every bestiary species (Tier-A > Tier-B > base precedence, never overwrite) + test 126 SCI_FANTASY.MutationSystem + validator 9c (table==bestiary 204, 8 themes, 16 bases on disk, consumption wiring, 16 cues staged) + FREE_ASSET_LEDGER §4; census UNCHANGED — the directive re-identifies the existing roster, no species inflation; runtime visuals ENGINE-UNVERIFIED (V2-35) |
| 2026 | **v8.0 (GLM FPP-1 — FINAL PLAYER-FACING PRESENTATION PASS, SOURCE_PRODUCT_FROZEN)**: 5-way presentation audit (skills/echo/bosses/progression+feedback/rulebook-extraction) → real dead ends fixed with smallest-compatible implementations, NO new systems and census UNCHANGED: (1) **P0 crafting screen** — UCLASS(Abstract)+no WBP meant CreateWidget returned nullptr and the crafting UI never opened; now a concrete native pure-C++ screen (rows: inputs have/need, station range, [Craft] with reason-labeled disabled states, live craft status) keeping the BP_* subclass contract; (2) player skills: GetSkillDescription table + pause-slot name/description/READY-recharging + GROWTH attributes+XP readout + level-up toast with new-skill milestone + Y no-ready/cast toasts; (3) journal+roster: ability kit lines (name+unlock Lv), passive aura, rideable markers, weakness flag names the ELEMENT, boss species gain bestiary entries; (4) HUD own-echo prompt truth (ride/evolve not "capture") + boss bar engages only within 4000cm of the NEAREST alive boss; (5) bosses: weakness/weak-point HIT toast+SFX, 0.5s melee windup telegraph + whiff window, element-tinted blast discs, weak-point pulse, phase/enrage/defeat toasts + defeat sound + loot toast; (6) feedback: craft refusal reasons + success toasts, kill-loot/supply-drop/brownout/quest-title/bond-25/40 toasts, WorkSite display names; +test 125 `FPP1.PresentationContract` + validator gate; `ASTRAWILD_PLAYER_RULES.md` issued (the concise player-facing rulebook). Post-freeze work is evidence-driven only: actual UE5 integration failures, runtime bugs, or engine-run visual defects |
| 2026 | **v7.3 (GLM PCR-6 — FINAL GATE, READY_FOR_FINAL_BUILD re-declared)**: gap-matrix closure ledger (PG-1..PG-6 all delivered with commit SHAs) + READINESS §P re-declaration + residual ledger rewrite (§M-a Tier-B = EXECUTED) + registry §K closed + this document's status/rows updated; both validators PASS at tip (124-test gate, census unchanged, Tier-B coherence gate); the branch freezes for the one-time Antigravity engine integration (AG-2..AG-5 + §22) |
| 2026 | **v7.2 (GLM PCR-5 — POST-GAME HUNT SYSTEM, PG-5 closed)**: `UAstrawildHuntSubsystem` (world subsystem; 8 repeatable cull contracts — every row reuses an EXISTING Tier-B species + EXISTING reward item, census UNCHANGED by design) + progress observes the SAME defeat events the quest counters count (Event.HostileDefeated/EchoDefeated via the event bus, server-side; world-shared counting = the documented co-op v1 exception class) + `ClaimHunt` (authority + completion validated; AddItemSilent rewards — no false quest credit; round resets → repeatable forever) + additive `FAstrawildHuntSaveRow` rows on the world save (no schema bump; fail-closed import drops unknown ids) + `UAstrawildHuntScreenWidget` + key **U** + pause-menu button + controller Request/ServerClaimHunt routing + test 124; post-game claim now TRUE: "world events, hunts, economy and dungeons keep running" has a hunts system behind it |
| 2026 | **v7.1 (GLM PCR-4 — TIER-B ARCHETYPE MESH LIBRARY, the §M-a MISSING residual closed)**: `Tools/ArtSourceGen/aw_archetypes.py` (8 parameterized body-plan builders on the Tier-A rig conventions — quadruped/biped/avian/serpent/insectoid/amorphous/floating/crystalline, per-species proportions/features/palette from the bestiary rows + deterministic name-hash jitter, 3 clips per species) + `gen_tier_b.py` (species data parsed from the ACTUAL source tables — bestiary 204 rows + appearance retrofit + zone-wildlife + dungeon pools + event boosts; Tier-B rule = spawn tables ∪ Huge ∪ monolith/colossus family minus the 14 Tier-A species) → **39 unique GLBs (4.8 MB, validate_glb PASS ×39, LFS-committed, manifest-recorded)**: every zone signature species, both dungeon pools, the monolith/colossus line + binding: `GetTierBSpeciesIds()` + `BuildTierBMechPath/BuildTierBAnimPath` + **definition-driven opt-in binding in ProductionContent (derived convention paths — ZERO engine-side code patch; before import the PMC body stays, the same opt-in contract the bosses use; explicit art rows always win)** + validator §9b (code list == 39 == baked GLBs) + test 123 (ASTRAWILD.PCR4.TierBLibrary — size pin + path derivation + on-disk GLB existence + registry binding round-trip); input/census unchanged |
| 2026 | **v7.0 (GLM PCR-0 — PRODUCT COMPLETION RUN opened)**: user directive "do not stop at source-complete" — full product audit at 4e52548 (Source/Content/ArtSource/Docs + widget census + input map + Journal/Roster/POI/Zone APIs + binding model + validator gates) produced `ASTRAWILD_PRODUCT_GAP_MATRIX.md`: PG-1 Field Journal UI (JournalSubsystem has data+save, zero UI consumers), PG-2 Echo Roster UI (roster has no screen; LCP-5 already noted "no client roster UI exists"), PG-3 World Map UI (12 zones/17 POIs/villages/dungeons/events, no map), PG-4 Tier-B archetype mesh library (the explicit READINESS §M-a MISSING residual — ~55 species), PG-5 post-game hunt system (active docs claim "hunts" continue post-game; zero hunt code exists — honesty gap), PG-6 doc-claim sync. Everything else verified as a non-gap (audio/VFX hooks live, affinity/farming/genetics/mounts live, save V5 complete, §20c binding queue correct, LAN closed). Registry §K opened (PCR-0..PCR-6); verdict suspended → IN PROGRESS-PCR; canon/census/engine gates unchanged |


### §5c SCP — Systems Completion Pack (v3.5, session 2026)

Plan-vs-repo audit of the vULTIMATE 14-phase directive found 17 missing systems;
six SCP batches closed 15 of them (source-complete, additive; 99 tests at SCP time — 102 at FCR, 103 after DP-3, 104 at DP-4, 105 at DP-5, 106 at DP-6, 107 at DP-7, 108 at DP-8, 109 at DP-9):
SCP-1 DataValidator/AssetFallback/ErrorReporter + Durability/Spoilage (a7a827f) ·
SCP-2 Base Terminal + Creature Sanity (394ac81) · SCP-3 Mounting (edc6b08) ·
SCP-4 Dual-Tech Combos + DDA (6cd29e4) · SCP-5 NPC Schedules + Crops + Offline
Production + Turrets (bbe2e3c) · SCP-6 Genetics + Performance Manager (9864cce).
Deferred with reasons: object pooling (engine-verify destroy path first),
IGenericTeamAgentInterface (co-op perception layer). Full matrix:
Docs/ASTRAWILD_SYSTEMS_COMPLETION_PACK.md.

### §5d DEPTH PASSES (v4.1–v5.0, 2026 session — user directive "MAKE IT A REAL GAME", closed at DP-10)

DP-1 creature visual strategy + 14 bespoke Tier-A echo meshes (981250d, c4012a0,
d9ebf86) + boss opt-in skeletal path with cone fallback (675e5b4, binding patch in
HANDOFF §20c) · DP-2 content integration matrix 14 categories (a2e7783) ·
DP-3 echo depth: locomotion signature abilities + 15-pair party element resonance +
water mounts (ffc7eca, +test 103) · DP-4 player 3-slot skill loadout (e6607b6, +test 104) ·
DP-5 combat depth: weak-point windows + weakness-hit feedback + per-boss special sets
(8771519, +test 105) · DP-6 base depth: 4 work sites + research-branch pin + field
consumables (89bd714, +test 106; census items 78 / recipes 58 / sites 8) · DP-7 world
depth: 7 zone events + per-zone hazard identity + 4 scanner-gated secrets (0087047,
+test 107; census events 16 / POIs 17) · DP-8 NPC depth: affinity-gated dialogue
evolution + regional knowledge (0710dd0, +test 108) · DP-9 dungeon depth: themed rooms +
resonance-pillar puzzles + room hazards (018a95a, +test 109) · DP-10 final gate (this
commit): both validators PASS, doc-consistency sweep, matrix/readiness/handoff
re-verified, registry §I closed. Residual ledger: Tier-B archetype rig library (~55
species, P1 art backlog with procedural material-identity degradation), engine-side
import/binding queue (§20b/§20c), and the tone/weapon/particle/ACS decisions that
await engine evidence. NOTHING here is engine-verified — AG-2..AG-5 (§8) remain the
exclusive conversion gates.
