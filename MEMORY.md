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


## Session 2026-10-12 — MRC-1 completion-stage merge (SUPER Z / GLM)

- User: "ทำต่อให้เสร็จแล้วเอาขึ้น git ด้วย" — continue to done + push.
- Found: local-main `af11ab4` (title/victory/death bookends + .bat tooling
  + WHATS_MISSING audit, 2026-09-17) was NEVER pushed and diverged from
  final-completion at `94a398c` (120 commits apart). Remote WF commits
  019a6eb/19da60c were NOT lost — they are ancestors of remote tip d326eee.
- Diagnosed the 440-file "modified" worktree as pure noise (file-mode bits
  + LFS pointer smudge; every oid identical) — safe force-checkout.
- Merged af11ab4 into final-completion -> merge commit `1b03a39`.
  4 conflict files resolved: PlayerController.h/.cpp (union: PCR 4 screens
  + completion 3 screens; IsAnyScreenOpen covers 12), AutomationTests.cpp
  (deterministic stitch: full 134 + VictoryChain -> 135 tests), README
  (both sections kept).
- Test-count gates synced 134->135 (validate_final_run, design_data
  recount + JSON envelope, orchestrator log, ON_PC_TASKS, LIVE_STATE
  current fields, REVIVAL_PLAN, README). Historical rows untouched.
- Validators at 1b03a39: lint 0 errors · final_run ALL PASS · design 78/78
  · repository PASS · dryrun 49/49.
- Remaining: ENGINE-RUN-1 machine day (now includes FIRST compile of the
  completion stage — #1 action per WHATS_MISSING §4) + push to origin.
- Side-track answered earlier this session: UE5 cannot run/compile on
  Google Colab via colab-mcp (Linux VM, no UE Linux binary, RAM/disk
  limits, headless) — machine day stays on Windows UE 5.8.2.


## Session 2026-10-15 — TOK-2 blocked-push release (SUPER Z / GLM)

- User supplied a fresh short-lived fine-grained PAT (~30-minute expiry)
  after TOK-1 proved every earlier token dead (live HTTP 401).
- Executed the prepared `scripts/push_all_branches.sh` (env-var only,
  masked output): origin `final-completion` fast-forwarded
  `d326eee -> 7394520` (MRC-1 merge + docs sync; LFS 2/2, 5.7 MB);
  NEW remote branch `playable-delivery` @ `8c6515c` (LFS 602/602, 300 MB);
  NEW remote branch `agent/game-shape-v1` @ `2d39f66` (LFS 604/604, 306 MB).
- Post-push anonymous `git ls-remote` verified all three tips; push exit
  codes 0/0/0. All surviving sandbox work is now ON GitHub — no
  local-only lines remain (backup bundles in download/ kept as archives).
- `main` left untouched on purpose (frozen baseline mirror per
  LIVE_STATE §1; its ahead-commit af11ab4 is already inside
  final-completion).
- Honest remaining: (1) ENGINE-RUN-1 machine day — unchanged, Windows
  UE 5.8.2 per ON_PC_TASKS hour 0->8; (2) canonical-branch decision:
  final-completion vs playable-delivery carry equivalent-but-different
  syncs of the same content — both are now on origin side-by-side for
  comparison before any reconciliation merge.
- Security: the PAT was passed via environment variable only, never
  written to any file/log/commit; it self-expires ~30 min after creation
  (no manual revoke needed, though revoking early never hurts).

## Session 2026-10-19 — VO-1R visual-overhaul rebuild (SUPER Z / GLM)

- Context: the prior session (visual overhaul + EchoDex design) ran out of
  context AFTER implementing+pushing EchoDex but BEFORE pushing the visual
  overhaul; its sandbox was then destroyed by the quota limit — the VO branch
  (c59613e) and its bundle are gone from every machine. Confirmed on origin:
  `agent/echodex-v1` @ 33b64f9 IS live (1,381 LOC, Paldeck/Aniimo-parity
  search — that deliverable survived); `agent/visual-overhaul-v1` was NOT.
- User directive: "rebuild ให้ทั้งหมด ตรวจสอบให้เรียบร้อยนะขาดอะไรตรงไหนหรือ
  บั๊กอะไรมั้ยด้วย" + confirmed the fine-grained PAT now lives 1 day
  (previously ~30 min).
- Rebuilt the whole VO layer ZERO-ASSET on new branch `agent/visual-overhaul-v1`
  (base 5f86a47 = TOK-2 tip): AstrawildVisualOverhaul.h/.cpp (pure contracts +
  director), bootstrapper toggle + atmosphere bias, 12-zone flower budgets +
  petal geometry + deterministic palettes, kawaii cute-band amplification
  (head/eyes/blush), ambient mote layer (day pollen / night fireflies).
- Contracts 135→140; gates synced across validate_final_run,
  validate_design_data, design_data.json envelope, first_day_orchestrator,
  ON_PC_TASKS, REVIVAL_PLAN, README, LIVE_STATE current fields.
- Honest scope note: the lost session's ~304MB LFS payload (AI-generated
  textures) was NOT recreated byte-for-byte — the rebuild covers the same five
  pillars procedurally (PP grade / light bias / flowers / kawaii / motes),
  which is the part that survives compile and is machine-checkable. A real
  art pass can layer on top via the existing ArtSource pipeline.
- Security: PAT used via env-var + remote URL only, never written to any
  file/log/commit; user should revoke it after the session window closes.

## Session 2026-10-20 — VOQA-1 static-QA closure (SUPER Z / GLM)

- Context: this sandbox came back RESTORED to the TOK-2 state (2026-10-15) —
  the VO-1R sandbox was destroyed by quota AFTER its push, so the local VO
  branch was gone while origin carried the truth. Re-fetched from origin:
  `agent/visual-overhaul-v1` @ `1d22925` (VO-1R tip — 6 commits on
  `5f86a47`, not the lost c59613e line) and `agent/echodex-v1` @ `4695e2c`
  (the lost session's other deliverable, already live). Checked the VO tip
  out locally with LFS smudge skipped (QA needs pointers only).
- User directive: "ทำไรก็เช็คให้ละเอียดเเล้วรีวิวด้วย ดูว่าทำไรยังไม่เสร็จ เเล้วทำต่อให้เสร็จ
  พร้อมกับดูในกิตว่าเกม v1 กับ 2 ต่างกันยังไงบ้าง".
- Full static QA re-run at the tip: lint_unreal_cpp PASS (0 errors, 1 known
  advisory dead-decl unchanged since L4); validate_final_run ALL PASS (census
  15/15: species 229 / quests 22 / items 78 / recipes 58 / buildings 26 /
  techs 17 / npcs 13 / loot 11 / weapons 8 / nodes 10 / sites 8 / events 16 /
  pois 17 / trees 13 / robots 3); validate_repository PASS;
  dryrun_unreal_tools 49/49 PASS.
- ONE genuine leftover found & FIXED: validate_design_data 77/78 — the VO-1R
  session ran the extractor BEFORE its commit batch, so design_data.json
  still stamped repo_head `5f86a47` while HEAD had moved to `1d22925` with
  Source/ changes (the envelope check correctly refused). Regenerated at the
  tip: 3-file diff (envelope 2 lines + bosses/dungeons DataTable trace
  line-shifts +17 from the VO bootstrapper growth); tunables 577 / tests 140
  / source_files 208 / census all identical; validator now 78/78 ALL PASS.
  Commit `c1bc8b1`.
- v1→v2 git comparison delivered to the owner (final-completion `5f86a47`
  vs visual-overhaul `1d22925`: 6 commits / 18 files / five pillars).
- Honest remaining (UNCHANGED): ENGINE-RUN-1 machine day on Windows UE 5.8.2
  (ON_PC_TASKS hour 0→8, now including the VO §7 PIE grading addendum) is
  the only product-critical item; optional: co-op client-parity batch
  (WHATS_MISSING §3 row 1), canonical-branch decision final-completion vs
  playable-delivery, real art pass on top of the zero-asset VO layer.
- Security: the user's 1-day PAT (set during the VO-1R session) reused for
  the push via env-var only; never written to any file/log/commit.
