# ASTRAWILD Working Memory

> **[HISTORICAL — early-session memory, frozen to its date]** The "Verified facts"
> below describe the pre-Final-Run repository (main branch, no binary assets) and
> are superseded: the live branch is `final-completion` — source-complete, 126
> automation contracts, 229 species, 416 genuine UE packages, LFS 459/459
> (ENGINE-UNVERIFIED at tip). Canonical control: `Docs/ASTRAWILD_MASTER_CONTROL.md`
> (v9.1); asset truth: `Docs/ASTRAWILD_CURRENT_ASSET_TRUTH.md`.

## Verified facts

- GitHub repository is private and currently has `main` as the verified branch.
- The reported `release/vertical-slice-v1` and commit `f8cf5f1` are not present on GitHub at the time of audit.
- Current GitHub `main` has C++ source and docs but no `.uasset` or `.umap` under `Content/ASTRAWILD`.
- The C++ core is a prototype contract; Unreal Compile and Playtest are still required on the target machine.
- Primitive mesh placeholders are allowed for the first playable risk slice.
- `Docs/visual_target_astrawild.png` is concept art used to guide composition, not a final imported asset.

## Verified facts (2026-09-18 completion-stage audit)

- **LFS objects ARE on GitHub's LFS storage** (60 MB, 46 files): all textures, both boot maps, the master material. A clone without git-lfs yields 130-byte pointer files and cannot boot — `Setup_FirstRun.bat` now detects and fixes this.
- Both `Content/ThirdPerson/Lvl_ThirdPerson.umap` and `Content/ASTRAWILD/Maps/MainMap.umap` are genuine UE packages (magic `0x9E2A83C1`).
- Post-`03c2fe6` source drift (5 files) was statically reviewed — no compile blockers found.
- The completion stage (title screen / victory / death overlay) is source-complete but NOT yet engine-compiled: first compile on the UE 5.8 machine is the #1 pending action. Tests now 55 (added `ASTRAWILD.Quest.VictoryChainTermination`).
- `/Game/Characters/Mannequins/...` (referenced by `94a398c`) is local-machine-only content — soft fallbacks to exosuit/procedural body keep the repo self-contained.

## Stable naming

- Module: `AstrawildCore`
- GameMode: `AAstrawildGameMode`
- Player: `AAstrawildPlayerCharacter`
- Echo: `AAstrawildEchoCharacter`
- Resource: `AAstrawildResourceNode`
- Rest Point: `AAstrawildRestPoint`
- Damage target: `AAstrawildDamageTarget`
- Save: `UAstrawildSaveGame` and `UAstrawildSaveSubsystem`
- Title menu: `UAstrawildTitleMenuWidget` (shown on standalone boot; PIE skips)
- Victory: `UAstrawildVictoryScreenWidget` (raised on `Quest_VanguardProtocol`)
- Death overlay: `UAstrawildDeathOverlayWidget` (respawn countdown)

## Decisions

- Build a compact Vertical Slice before open-world expansion.
- Target future co-op for 1–4 players; do not claim multiplayer complete until authority and two-client tests pass.
- Use original ASTRAWILD identity inspired by gameplay categories, not copied characters, names, sounds, maps or assets.
- Keep C++ authoritative for transaction rules and stable save data; use Blueprint/Content for composition and presentation.
- Record every third-party asset license.
- Completion stage bookends stay pure-C++ UMG (no asset dependency) so the zero-asset rule holds; each screen has a `*Class` override point for future UMG/Designer variants.

## Handoff truth

Antigravity must update `Docs/BUILD_STATUS.md`, push its branch/commit to GitHub, and include evidence of Compile and Playtest. A text report alone is not proof that files exist in the repository.

---

## Session 2026-09-18 — Master Directive v1 MACHINE-READY pack (GLM, Unreal Division)

> [LIVE] This section supersedes everything above it for current-session
> context. Canonical live state remains `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md`.

- Directive accepted: autonomous engineering lead, cinematic branch, R1-R10,
  UNVERIFIED — NEEDS MACHINE discipline, one task = one commit = one push.
- User approved FULL autonomous execution ("ทำต่อให้เสร็จตามแผนทั้งหมด") +
  installed the wayfinder skill; map charted at `.wayfinder/` in the sandbox
  (8 tickets, all closed this session).
- Landed (all pushed to origin/final-completion):
  - `118c9a1` feat(design): Design/design_data.json (574 tunables / 204
    species / 49 items / 32 recipes / 134 tests — every value file:line
    traced) + Scripts/extract_design_data.py + Scripts/validate_design_data.py
    (sandbox run: 8/8 PASS, 0 drift).
  - `6157a1e` feat(tools): Tools/Python/verify_environment.py +
    first_day_orchestrator.py (AWENV/AWFIRST verdict contract; delegation to
    existing proven scripts; ast-verified only — first run is on machine).
  - `138378f` docs(risk): Docs/ASTRAWILD_COMPILE_RISK.md — headline: last
    green compile 8313c61; delta 164 files / +33,343 lines / 95 commits
    never machine-compiled; R-C1..R-C5 ranked; binding fix-forward protocol.
  - `0f2617d` docs(identity): Docs/ASTRAWILD_CINEMATIC_IDENTITY.md — charter
    reconciliation (nothing pruned, R1) + traced feel targets + HIT /
    MISS(number) / MISS(feel) grading semantics.
  - `5f1d702` docs(runbook): Docs/ASTRAWILD_ON_PC_TASKS.md — hour 0 → 8
    machine manual with exact commands, pass signals, fail routes.
- Validators at this tip: validate_final_run.py ALL PASS ·
  validate_design_data.py 8/8 PASS.
- Next session pointer: the machine day itself (ON_PC_TASKS) — everything
  source-side that the directive's DoD requires is now in place; the only
  open items are machine-execution rows in COMPILE_RISK §6.

## 2026-09-18 — LONG-RUN DIRECTIVE L1..L10 (autonomous day run, 11 commits 4520db6..0c69866)

- L1 TRUTH: Docs/ASTRAWILD_COVERAGE_REPORT.md — docs census re-proved 15/15
  (0 overclaim); MY prior extractor was the incomplete side (missed
  ProductionContent.cpp). L2 fixed it: 26 traced domains, census 15/15,
  validate_design_data.py 78/78 ALL PASS.
- L3+L5 TOOLING: Tools/Python/_mock_unreal/ recording mock +
  Scripts/dryrun_unreal_tools.py — 7 tools, 49/49 proofs (idempotence,
  convergence, no-missing-symbols); symbol table 55 VERIFIED / 0 UNVERIFIED.
  New tools: generate_datatables (3 real DataTables + 20 staged owner-
  decision JSON in Design/DataTable/), wire_gamemode, build_showcase_map
  (cinematic rig), orchestrator now 6 steps.
- L4 LINT: Scripts/lint_unreal_cpp.py (9 families, 0 errors/1 warning) —
  found & FIXED 2 real compile blockers: phantom UAstrawildPlayerController
  (EndingCinematic) + SaveSubsystem missing PlayerController include
  (commit f9a6dcc; 4 changed lines). COMPILE_RISK.md rewritten with
  before/after + open O-1..O-4.
- L6-L9 DOCS: SOURCE_INVENTORY (183/17/0, 20 REPLACE ledger) · ART_ASSET_PLAN
  (12 targets → staged CC0) · PACKAGING_PLAN (workflow audit clean) ·
  ON_PC_TASKS v2 + Tools/preflight_check.bat (GO/NO-GO gate).
- L10 REPORT: Docs/EVENING_REPORT.md (self-critical, 78% honest readiness).
- Validators at this tip: validate_final_run ALL PASS ·
  validate_design_data 78/78 · dryrun 49/49 · lint 0 errors.
- DECISION_LOG.md (10 autonomous decisions) + BLOCKER.md (0 entries).
- Next session pointer: THE MACHINE DAY (preflight → Build → 6-step
  orchestrator → Setup_And_Play → Test → feel grading → package). The
  first MSVC compile is the only closer for everything still UNVERIFIED.

---

## Session 2026-10-09 — GAME-SHAPE PASS (GLM, evidence-first takeover, branch agent/game-shape-v1)

> [LIVE] Baseline d326eee (verified tip of origin/final-completion, 0/0 divergence;
> working tree's 1152 "modified" files proven byte-identical LFS filter artifacts).

- GS-1 ART-PATH HARDENING: `AstrawildArtPack::CanonicalAssetPath()` — package-style
  soft paths ("Path/SM_A") canonicalized to object-path form ("Path/SM_A.SM_A") at
  every apply site (ProductionContent 21, PlayerCharacter 8, ContentLibrary NPC 4,
  dungeon tiles + 5 sound-cue sites). Rationale: FSoftObjectPath's suffix-less form
  is engine-version-dependent; worst case = silent never-resolving bindings and
  permanent primitive fallbacks. Canonical form works in EVERY version. Pinned by 4
  unit assertions + round-trip contract in ASTRAWILD.ArtPack.BindingContract.
  COMPILE_RISK O-6 (evidence F→B). Commit d410ac7.
- P-5/P-6 LANDMARK BINDINGS: `GetLandmarkArt()` ruin pillar bound to
  ResonancePillar (240cm) + POIMarker (260cm) via runtime bounds-fit (any pivot
  convention, fail-closed to engine primitives), warmed in WarmArtPackBindings.
  Stale REPLACE_BEFORE_RELEASE comments corrected where bindings are LIVE
  (ResourceNode Batch-4, Projectile Niagara CP-05).
- GS-2 STRANDED-COMMIT RESCUE: local af11ab4 ("game-loop bookends" — TITLE /
  VICTORY / DEATH screens + `_FindEngine.bat` registry-based engine detection +
  relative-path launchers + Setup_FirstRun) was NEVER in final-completion's
  history (authored 2026-09-17 in the repo-astrawild working copy, unpushed).
  Cherry-picked with conflict resolution onto the evolved controller (union:
  PCR screens + completion bookends). Commit 833bcf4. Tests 134→135
  (VictoryChainTermination); validator gates + live docs updated together.
- Dead decl BuildContentDefaults() removed (lint 0 errors / 0 warnings, 203 files).
- Validators at tip: final_run ALL PASS · design 78/78 · lint 0/0 · dryrun 49/49
  (55 VERIFIED / 0 UNVERIFIED) · census 15/15 · 135 automation contracts.
- The game now has its full loop shape statically: Title (New Journey / Continue)
  → 12-zone bootstrapped world (real meshes where imported; fail-closed stand-ins
  elsewhere) → survive/craft/capture/quest → DEATH overlay + respawn → VICTORY
  screen + free hunt → save/continue. Remaining UNKNOWN = the one Windows
  engine day (compile + PIE + package) — unchanged, honestly parked at
  ENGINE-RUN-1 in Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md.
