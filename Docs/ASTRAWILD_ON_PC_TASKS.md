# ASTRAWILD — ON-PC TASKS: The First Day, Hour by Hour

> **Master Directive v1 / roadmap P4 — the MACHINE-READY runbook.**
> Audience: you, at the Windows machine, starting from a fresh clone.
> Engine authority: **UE 5.8.2 at `E:\Epic Games\UnrealEngine`** · Project
> home assumed `E:\AstrawildGame` (every repo script defaults to these two
> paths — override with `UE_ROOT` / `ASTRAWILD_UPROJECT` if different).
> Success bar for the day: **a walkable, lit runtime prototype + the 134
> automation contracts green + (stretch) a Shipping package.**
> Every step has an expected result; every failure routes to a fix entry.
> Written 2026-09-18 at HEAD `0f2617d`. Statuses here are UNVERIFIED —
> NEEDS MACHINE until YOU run them; this file is your checklist, not a
> claim.

---

## Hour 0 — Ground truth (15-20 min)

### 0.1 Prerequisites (all must be YES before anything else)

| # | Check | How | Expected |
|---|---|---|---|
| 1 | UE 5.8.2 exists | Explorer → `E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor.exe` | file present |
| 2 | Visual Studio 2022 + **Game Development with C++** workload | VS Installer | workload ticked |
| 3 | Git + **Git LFS** | `git lfs version` in cmd | prints a version |
| 4 | ~10 GB free on E: | Explorer | yes |

### 0.2 Fresh clone + LFS

```bat
cd /d E:\
git clone https://github.com/banksaisuoy/astrawild-game.git AstrawildGame
cd AstrawildGame
git checkout final-completion
git lfs install
git lfs pull
```

**Expected**: `git lfs pull` ends without `pointer without content` errors
(586 objects — be patient on first pull).

**Fail route**: pointer errors → `git lfs pull --include="ArtSource/**"`
then re-run; persisting errors → R-C5 in
`Docs/ASTRAWILD_COMPILE_RISK.md` §2.

### 0.3 Set the environment once (this cmd window)

```bat
set UE_ROOT=E:\Epic Games\UnrealEngine
set ASTRAWILD_UPROJECT=E:\AstrawildGame\ASTRAWILD.uproject
```

`Setup_And_Play.bat` and `Test.ps1` honor these (and default to the same
values if you skip this).

---

## Hour 0-1 — THE BUILD (the compile gate; 20-40 min)

This is the moment the 164-file / +33 K-line delta meets MSVC for the
first time (see `Docs/ASTRAWILD_COMPILE_RISK.md` §1).

```bat
cd /d E:\AstrawildGame
Build.bat
```

(= `Build.ps1` → UnrealBuildTool, Development Editor target.)

**Expected**: ends with `Total execution time` and **0 errors**; produces
`Binaries\Win64\UnrealEditor-AstrawildCore.dll`.

**Fail route (BINDING — follow exactly)**:
1. Copy the **first** error only (MSVC/UHT cascade noise starts at #2).
2. Save the full log as `Docs\ENGINE_LOGS\BUILD_FAIL_<short-sha>_<date>.log`.
3. Apply the smallest positive diff (the repo's repair precedent: 6
   blockers = 5 includes + 1 forward decl, commit `746c59f`).
4. If you want the sandbox to re-check the patched tree: run
   `python Scripts/validate_final_run.py` + `python Scripts/validate_design_data.py`
   on it before committing — both are pure Python, run anywhere.
5. Commit as `fix(core): <error-id> <symbol> in <file> — BUILD_FAIL_<sha>`
   and push. Never defer silently.

**Time-box honesty**: with 1-5 residual one-liner errors, hour 0-1 may
stretch to hour 2. That is INSIDE the "one work day" budget — do not
panic, do not skip the protocol.

---

## Hour 1 — Orchestrated source-side flow (10-15 min)

The new single-command driver (read-only verify → prototype map → input
assets — every step idempotent):

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
  "%ASTRAWILD_UPROJECT%" ^
  -run=pythonscript -script="E:\AstrawildGame\Tools\Python\first_day_orchestrator.py" ^
  -stdout -unattended -nopause -nosplash
```

**Expected** (log lines with the `[AWFIRST]` prefix):
```
[AWFIRST] --- 1/3 environment check ---
[AWENV] PASS  engine-version        running 5.8.2-... (need 5.8+)
[AWENV] PASS  astrawild-classes     AstrawildGameMode=ok, ... =ok
...
[AWFIRST] --- 2/3 prototype map ---
[L_Proto_01] DONE ...
[AWFIRST] --- 3/3 input assets ---
[InputSetup] DONE — ...
[AWFIRST] VERDICT: PASS (map + input assets ready)
```

**Fail route**: `VERDICT: FAIL` names the step + its own error line:
- environment step → check the `[AWENV] FAIL` group (engine version vs
  AstrawildCore classes missing = the module didn't build → back to
  Hour 0-1);
- map/input steps → re-run once (idempotent by design), then file the
  exact exception — both scripts are first-execution code (R-C3).

`VERDICT: SOFT-PASS` = no hard failure but some groups printed `UNK` —
readable but not fatal; continue, note the groups for the report.

---

## Hour 2 — Real-asset import + showcase map (20-30 min)

```bat
cd /d E:\AstrawildGame
Setup_And_Play.bat
```

Imports the 109 real CC0 meshes + clips, builds materials, adds sockets,
dresses `/Game/Maps/L_Showcase_ArtOverhaul`, leaves the editor open.

**Expected**: editor opens ON the showcase map, meshes visible on the
ground, PlayerStart placed; report at
`Saved\AwPipelineReport\import_report.json` shows `"total_missing": 0`
(109/109 incl. AM_ clips).

**Fail route**: missing meshes in report → R-C4 in COMPILE_RISK §2
(non-fatal to the day's goal — the game is zero-asset-first and renders
placeholders); catalog errors → the bat prints the manifest path to check.

**Milestone check (the DAY's minimum bar)**: in the open editor, press
**Play (PIE)** on the showcase map → you are WALKING in a lit world.
If only this much works by end of day, the MACHINE-READY promise held.

---

## Hour 3 — Automation gate (15-25 min)

```bat
Test.bat
```

Runs the **134 world-free contracts** via
`-ExecCmds="Automation RunTests Astrawild; Quit" -nullrhi`.

**Expected** console summary:
```
>>> Test Execution Completed in <N>s <<<
PASS  (134 successes, 0 failures)   [exact phrasing: Result={Success} x134]
```
Output file: `Automation_Output.txt` beside the project.

**Fail route**: any `Result={Fail}` line carries the failing test name —
grep it in `Source/AstrawildCore/Private/AstrawildAutomationTests.cpp`
(the name encodes the contract); a logic failure here is a REAL BUG
(numbered and pinned), fix-forward like Hour 0-1.

---

## Hour 4-6 — PIE golden path + FEEL grading (the judge hours)

Play the game. Use `Launch_ASTRAWILD.bat` (standalone) or PIE in editor.
Grade against `Docs/ASTRAWILD_CINEMATIC_IDENTITY.md` §2 — every rule
below cites its traced number:

| # | Probe | Expected (traced) | Grade |
|---|---|---|---|
| 1 | Walk a straight line 30 s | steady 450 uu/s cadence, "trek" not "dash" | HIT / MISS |
| 2 | Hold sprint until exhausted | ≈ **14 s**, then forced drop to walk | HIT / MISS |
| 3 | Light attack x10 | ≈ 2.2 swings/s (0.45 s cd) | HIT / MISS |
| 4 | Heavy attack | feels committed (1.3 s cd), 60 dmg on hit | HIT / MISS |
| 5 | Dodge through a boss telegraph disc | i-frames carry (0.4 s window) | HIT / MISS |
| 6 | Stand in a hazard 10 min | vitals bars move, never a forced menu trip | HIT / MISS |
| 7 | Wait for dawn + dusk | full day = 24 min; both times produce ≥ 1 screenshot composition | HIT / MISS |
| 8 | Run the ending sequence (if reachable) | 18.7 s, 6 shots, 1.4 s blends, skippable | HIT / MISS |

**Grading semantics** (from CINEMATIC_IDENTITY §3):
- **MISS (number)** — traced value didn't manifest = bug → fix-forward,
  file it like a compile error.
- **MISS (feel)** — number hit but sensation wrong = DESIGN change → edit
  the value IN SOURCE, re-run `Scripts/extract_design_data.py`, commit.
  Never hand-tune docs around code.

**Bonus probes** (systems breadth — nice if green, not the day's bar):
capture a creature (feed → weaken → capture), craft a Bandage, open the
journal/map screens, mount/skiff if reachable.

---

## Hour 7-8 — Package + report (stretch goal)

```bat
cd /d E:\AstrawildGame
Tools\package_windows.bat
```

Builds Win64 **Shipping** into `Build\Shipping\` (one-click; optionally
attaches a GitHub Release via `.github/workflows/release.yml` on tag).

**Expected**: `Build\Shipping\Windows\ASTRAWILD.exe` launches standalone.

**Fail route**: packaging errors are usually missing-plugin/stripping
issues — capture the first error, same protocol as Hour 0-1.

---

## End-of-day report (10 min — makes the next session start warm)

1. Paste outcomes into `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` §1/§5
   (it is the canonical state file; statuses from its §2 vocabulary only).
2. Append the day to `MEMORY.md` (one line per hour outcome).
3. Screenshot the dawn + dusk compositions into
   `Docs/ENGINE_LOGS/` — they are the cinematic charter's evidence.
4. Commit `docs(state): ENGINE-RUN-1 machine results <date>` + push.

---

## The day at a glance

| Hour | Gate | Command | Pass signal |
|---|---|---|---|
| 0 | ground truth | clone + `git lfs pull` | no pointer errors |
| 0-1 | **compile** | `Build.bat` | 0 errors, DLL produced |
| 1 | source-side flow | orchestrator cmdlet | `[AWFIRST] VERDICT: PASS` |
| 2 | assets + map | `Setup_And_Play.bat` | `total_missing: 0`, PIE walks |
| 3 | logic gate | `Test.bat` | 134/134 `Result={Success}` |
| 4-6 | feel grading | play | ≥ 6/8 HIT on the probe table |
| 7-8 | package | `Tools\package_windows.bat` | exe launches |
| +10m | report | docs edits + push | state file truthful |

If any gate fails: STOP at that gate, run its fail route, and the day's
remaining budget goes to fix-forward — that is the plan working, not the
plan failing.
