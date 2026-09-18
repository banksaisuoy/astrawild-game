# ASTRAWILD — ON-PC TASKS v2: The First Day, Hour by Hour

> **LONG-RUN DIRECTIVE L9 — the MACHINE-READY runbook, rewritten to absorb
> everything L1..L8 produced.** Audience: you, at the Windows machine,
> **having never opened Unreal Editor before** — every command is
> copy-paste, every expected output is written out, every failure has a
> route. Engine authority: **UE 5.8.2 at `E:\Epic Games\UnrealEngine`** ·
> Project home assumed `E:\AstrawildGame`. Success bar for the day:
> **a walkable, lit runtime prototype + the 134 automation contracts green
> + (stretch) a Shipping package.**
> Written at HEAD after the L1-L8 run. Statuses here are UNVERIFIED — NEEDS
> MACHINE until YOU run them; this file is your checklist, not a claim.

**What changed vs v1** (context, 30 seconds): the sandbox run re-proved the
repo's content census to the number (`Docs/ASTRAWILD_COVERAGE_REPORT.md`
— 15/15), finished the traced design layer (`Design/design_data.json`, 26
domains, validator 78/78), executed every editor tool's logic against a
recording mock (`Scripts/dryrun_unreal_tools.py` — 49/49 proofs), statically
linted all 197 Source files and fixed the two real compile blockers it
found (`Docs/ASTRAWILD_COMPILE_RISK.md` §2 — read it before Hour 0-1),
built the full first-day tooling chain (6 steps), and wrote the art /
packaging plans. The machine day below is where all of that meets MSVC.

---

## Hour 0 — Ground truth (10-20 min)

### 0.1 ONE command first (the new preflight)

```bat
cd /d E:\AstrawildGame
Tools\preflight_check.bat
```

Checks engine path, VS2022 "Game development with C++" workload, Python
Editor Script plugin, git + LFS, branch/tree state, and ~10 GB disk.

**Expected** (last lines):
```
 GO - all hard checks passed. Start at Hour 0-1 of
      Docs\ASTRAWILD_ON_PC_TASKS.md (the compile gate).
 PASS=10  FAIL=0
```

**Pass signal**: `GO` + exit code 0.
**Fail route**: the script names the first failing check and its fix
(e.g. `vs-workload-missing` → open VS Installer, tick the workload). Fix,
re-run. **Do not proceed on NO-GO.**

### 0.2 Fresh clone + LFS (only if you have not cloned yet)

```bat
cd /d E:\
git clone https://github.com/banksaisuoy/astrawild-game.git AstrawildGame
cd AstrawildGame
git checkout final-completion
git lfs install
git lfs pull
```

**Expected**: `git lfs pull` ends without `pointer without content` errors
(586 objects — first pull takes a while).
**Fail route**: pointer errors → `git lfs pull --include="ArtSource/**"`,
re-run; persisting → `Docs/ASTRAWILD_COMPILE_RISK.md` §4 O-items.

### 0.3 Set the environment once (this cmd window)

```bat
set UE_ROOT=E:\Epic Games\UnrealEngine
set ASTRAWILD_UPROJECT=E:\AstrawildGame\ASTRAWILD.uproject
```

---

## Hour 0-1 — THE BUILD (the compile gate; 20-40 min)

This is the moment the 197-file / +33 K-line delta meets MSVC for the
first time. The L4 static linter already removed the two blockers it could
prove (`Docs/ASTRAWILD_COMPILE_RISK.md` §2: the phantom
`UAstrawildPlayerController` type and the missing PlayerController include
— both FIXED in this run), and left 1 dead declaration + engine-side
unknowns (§4 O-1..O-4).

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
   blockers = 5 includes + 1 forward decl, commit `746c59f`; plus this
   run's 2 linter-proven fixes).
4. Re-check the patched tree anywhere: `python Scripts/validate_final_run.py`
   + `python Scripts/validate_design_data.py` + `python Scripts/lint_unreal_cpp.py`
   (all pure Python, no engine).
5. Commit as `fix(core): <error-id> <symbol> in <file> — BUILD_FAIL_<sha>`
   and push. Never defer silently.

**Time-box honesty**: with 1-5 residual one-liner errors, hour 0-1 may
stretch to hour 2. That is INSIDE the "one work day" budget.

---

## Hour 1 — Orchestrated first-day flow, now SIX steps (10-15 min)

The single-command driver runs the whole chain in order (each step prints
its expected result before running; every step idempotent):

```bat
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
  "%ASTRAWILD_UPROJECT%" ^
  -run=pythonscript -script="E:\AstrawildGame\Tools\Python\first_day_orchestrator.py" ^
  -stdout -unattended -nopause -nosplash
```

**Expected** (`[AWFIRST]` prefix; steps 1-3 existed in v1, 4-6 are new):

```
[AWFIRST] --- 1/6 environment check ---
expect: VERDICT line with engine 5.8+, 4 class groups ok
[AWENV] VERDICT: PASS ...
[AWFIRST] --- 2/6 prototype map ---
expect: L_Proto_01 saved; 8 LPROTO_* actors; DONE line
[L_Proto_01] DONE — open L_Proto_01 and press Play.
[AWFIRST] --- 3/6 input assets ---
expect: 32 IA_* + IMC_Player(35) + IMC_Gamepad(19) + 2 BPs
[InputSetup] DONE — 32 actions, IMC_Player (35 maps), ...
[AWFIRST] --- 4/6 DataTable generation ---
expect: 3 DataTables (Abilities 53 / Weather 8 / Zones 12)
[DTGen] DONE — DT_AstrawildAbilities=53, DT_AstrawildWeather=8, DT_AstrawildZones=12. ...
[AWFIRST] --- 5/6 showcase map ---
expect: L_Showcase_ArtOverhaul saved; cinematic rig + vista
[L_Showcase] DONE — open L_Showcase_ArtOverhaul and press Play: ...
[AWFIRST] --- 6/6 GameMode wiring ---
expect: both levels: GameMode override asserted, surfaces ok
[WireGM] VERDICT: PASS — both levels wired ...
[AWFIRST] VERDICT: PASS (map + input assets ready)
```

**Fail route**: `VERDICT: FAIL/PARTIAL` names the step + its own error
line. Environment step failing on `astrawild-classes` = module didn't
build → Hour 0-1. Steps 4-6 are first-execution code: re-run once
(idempotent), then file the exact exception line. Step 4 failing on a
row-struct lookup means the module compiled but the Python class name
differs (R2 conformance item — see the `[DTGen]` error text, adjust the
`row_struct` spelling in `Tools/Python/generate_datatables.py`).

**Why you can trust the chain's logic**: all six scripts executed
end-to-end in the sandbox against a recording mock of the editor API —
49/49 proofs including run-twice idempotence (`Scripts/dryrun_unreal_tools.py`).
What the mock cannot prove is the exact 5.8 API spellings; that is what
this hour is quietly testing.

---

## Hour 2 — Real-asset import + showcase map (20-30 min)

```bat
cd /d E:\AstrawildGame
Setup_And_Play.bat
```

Imports the 109 real CC0 meshes + clips, builds materials, adds sockets,
dresses `/Game/Maps/L_Showcase_ArtOverhaul`, leaves the editor open.

**Expected**: editor opens ON the showcase map, meshes visible, PlayerStart
placed; `Saved\AwPipelineReport\import_report.json` shows `"total_missing": 0`
(109/109 incl. AM_ clips).

**Fail route**: missing meshes in report → `Docs/ASTRAWILD_COMPILE_RISK.md`
§4 (non-fatal — the game is zero-asset-first and renders placeholders;
see `Docs/ASTRAWILD_ART_ASSET_PLAN.md` §2 for every placeholder's bound
replacement asset).

**Milestone check (the DAY's minimum bar)**: in the open editor, press
**Play (PIE)** on the showcase map → you are WALKING in a lit world, and
the first frame is the framed hero vista (raised deck, golden-hour light,
volumetric haze — `Tools/Python/build_showcase_map.py`'s cinematic rig).
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
(the name encodes the contract); a logic failure here is a REAL BUG,
fix-forward like Hour 0-1.

---

## Hour 4-6 — PIE golden path + FEEL grading (the judge hours)

Play the game. Use `Launch_ASTRAWILD.bat` (standalone) or PIE in editor.
Grade against `Docs/ASTRAWILD_CINEMATIC_IDENTITY.md` §2 — every rule
below cites its traced number (all values live in
`Design/design_data.json` with {file,line} traces):

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

**Grading semantics**:
- **MISS (number)** — traced value didn't manifest = bug → fix-forward.
- **MISS (feel)** — number hit but sensation wrong = DESIGN change → edit
  the value IN SOURCE, re-run `Scripts/extract_design_data.py` +
  `Scripts/validate_design_data.py`, commit. Never hand-tune docs around
  code.

**Bonus probes** (nice if green, not the day's bar): capture a creature
(feed → weaken → capture), craft a Bandage, open the journal/map screens,
mount/skiff if reachable.

---

## Hour 7-8 — Package + report (stretch goal)

```bat
cd /d E:\AstrawildGame
Tools\package_windows.bat
```

Builds Win64 **Shipping** and archives to `Build\Windows\ASTRAWILD\`
(one-click; full command, artifact tree, and troubleshooting table in
`Docs/ASTRAWILD_PACKAGING_PLAN.md`).

**Expected**: `[OK] Packaging succeeded.` —
`Build\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe` launches standalone
on any Windows 10/11 x64 PC.

**Fail route**: `Docs/ASTRAWILD_PACKAGING_PLAN.md` §5 troubleshooting
table (engine root, MSVC on the runner, LFS smudge, zip size...). CI
release (optional): `git tag v0.1.0 && git push origin v0.1.0` — the
self-hosted workflow audits clean statically (L8).

---

## End-of-day report (10 min — makes the next session start warm)

1. Paste outcomes into `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` §1/§5
   (statuses from its §2 vocabulary only).
2. Append the day to `MEMORY.md` (one line per hour outcome).
3. Screenshot the dawn + dusk compositions into `Docs/ENGINE_LOGS/`.
4. Commit `docs(state): ENGINE-RUN-1 machine results <date>` + push.

---

## The day at a glance

| Hour | Gate | Command | Pass signal |
|---|---|---|---|
| 0 | ground truth | `Tools\preflight_check.bat` | `GO ... FAIL=0` |
| 0-1 | **compile** | `Build.bat` | 0 errors, DLL produced |
| 1 | source-side flow (6 steps) | orchestrator cmdlet | `[AWFIRST] VERDICT: PASS` |
| 2 | assets + map | `Setup_And_Play.bat` | `total_missing: 0`, PIE walks |
| 3 | logic gate | `Test.bat` | 134/134 `Result={Success}` |
| 4-6 | feel grading | play | ≥ 6/8 HIT on the probe table |
| 7-8 | package | `Tools\package_windows.bat` | exe launches |
| +10m | report | docs edits + push | state file truthful |

If any gate fails: STOP at that gate, run its fail route, and the day's
remaining budget goes to fix-forward — that is the plan working, not the
plan failing.
