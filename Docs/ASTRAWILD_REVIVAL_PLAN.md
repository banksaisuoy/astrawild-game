# ASTRAWILD — Revival Plan (start from any state, zero context)

> **Purpose**: you (or any agent) just opened this repo cold. Something may
> be half-done, half-built, or broken. This page is the triage path that
> works from ANY state — it never assumes yesterday's context.
> Companion docs: `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` (canonical state),
> `Docs/ASTRAWILD_ON_PC_TASKS.md` (the healthy-machine day plan),
> `Docs/ASTRAWILD_COMPILE_RISK.md` (risk + fix-forward protocol).

---

## Step 0 — Read the three truth files (5 min, in this order)

1. `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` §1 — the LIVE snapshot: HEAD,
   phase, in-flight task, blocked tasks. **When any other doc disagrees
   with this file on status, this file wins.**
2. `MEMORY.md` (bottom = most recent session) — what the last session did
   and meant to do next.
3. `git log --oneline -15` — what actually landed last.

## Step 1 — Establish ground truth (10 min)

```bash
git status                     # clean? committed? on final-completion?
git branch --show-current      # must be final-completion (main = frozen mirror)
python Scripts/validate_final_run.py      # ALL PASS = source-side coherent
python Scripts/validate_design_data.py    # ALL PASS = design data in sync
```

- Both validators PASS → the repo is self-consistent at this HEAD; trust
  the docs' claims about STATIC state.
- Any FAIL → something mutated Source/ without re-running the design
  extractor or a validator regression exists: fix THAT first (usually just
  `python Scripts/extract_design_data.py`), commit, push.

## Step 2 — Where are you? (pick the row, do the action)

| Situation | Meaning | Action |
|---|---|---|
| Fresh machine, nothing built | the default case | follow `Docs/ASTRAWILD_ON_PC_TASKS.md` from hour 0 |
| Built before, now broken build | a C++ change landed since | `Build.bat`, first-error-only, fix-forward per COMPILE_RISK §5 |
| Build OK, tests fail | logic regression (rare — contracts pin logic) | `Test.bat`, grep failing test name in AstrawildAutomationTests.cpp, fix-forward |
| Tests OK, game feels wrong | design drift, not bugs | grade probes in CINEMATIC_IDENTITY §2; change values IN SOURCE + re-extract |
| Docs/status disagree with git | stale docs | LIVE_EXECUTION_STATE wins; reconcile others to it, commit `docs(sync)` |
| LFS pointer errors | assets not pulled | `git lfs pull`; see R-C5 |
| Editor Python tools misbehave | first-execution API drift | read `[AWENV]` / `[AWFIRST]` FAIL lines, fix in Tools/Python, re-run (idempotent) |
| No idea at all | worst case | do Steps 0-1, then hour 0-1 of ON_PC_TASKS — the compile gate tells you everything |

## Step 3 — The invariant that makes revival cheap

The repo is engineered so NO failure state is deep:

- **Zero-asset-first**: every creature/prop renders engine-basic-shape
  placeholders — the game is walkable even with zero imported art.
- **134 world-free contracts**: logic truth is machine-checkable in
  minutes (`Test.bat`), independent of maps, art, or saves.
- **Traced design data**: every number re-derivable from Source/
  (`Scripts/extract_design_data.py` + validator) — no spreadsheet drift.
- **Idempotent tooling**: map builder purges + rebuilds its own actors;
  input setup recreates its assets; the orchestrator is safe to re-run.
- **Evidence discipline**: engine runs leave logs in
  `Docs/ENGINE_LOGS/` — the revival question "was this ever verified?" is
  always answerable from files, never from memory.

## Step 4 — After recovery: leave the trail warm

Every session ends: commit (Conventional) → push → update
LIVE_EXECUTION_STATE §1/§5 → append one block to `MEMORY.md`. A session
that doesn't do this is a session that creates the next revival scenario.

---

## Quick reference — who owns what

| Layer | Owner file |
|---|---|
| Execution state (live) | `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` |
| Game/product rules | `Docs/ASTRAWILD_MASTER_CONTROL.md` |
| Machine first-day plan | `Docs/ASTRAWILD_ON_PC_TASKS.md` |
| Compile risks + fix protocol | `Docs/ASTRAWILD_COMPILE_RISK.md` |
| Cinematic identity + feel | `Docs/ASTRAWILD_CINEMATIC_IDENTITY.md` |
| Design data + schema | `Design/` (+ `Scripts/extract_design_data.py`) |
| Task registry (history) | `Docs/ASTRAWILD_MASTER_TASK_REGISTRY.md` |
| Engine evidence (logs) | `Docs/ENGINE_LOGS/` |
