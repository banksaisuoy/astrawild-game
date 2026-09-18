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

## Stable naming

- Module: `AstrawildCore`
- GameMode: `AAstrawildGameMode`
- Player: `AAstrawildPlayerCharacter`
- Echo: `AAstrawildEchoCharacter`
- Resource: `AAstrawildResourceNode`
- Rest Point: `AAstrawildRestPoint`
- Damage target: `AAstrawildDamageTarget`
- Save: `UAstrawildSaveGame` and `UAstrawildSaveSubsystem`

## Decisions

- Build a compact Vertical Slice before open-world expansion.
- Target future co-op for 1–4 players; do not claim multiplayer complete until authority and two-client tests pass.
- Use original ASTRAWILD identity inspired by gameplay categories, not copied characters, names, sounds, maps or assets.
- Keep C++ authoritative for transaction rules and stable save data; use Blueprint/Content for composition and presentation.
- Record every third-party asset license.

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
