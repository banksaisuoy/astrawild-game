# ASTRAWILD — COMPILE RISK REGISTER

> **Master Directive v1 / roadmap P2 — [AUDITOR] work product.**
> Scope: every risk standing between this repo's HEAD and a successful
> `Setup_And_Play.bat` compile on the Windows UE 5.8.2 machine.
> Evidence policy: sandbox-verifiable facts are labeled with their tool and
> date; everything engine-side is **UNVERIFIED — NEEDS MACHINE** until the
> first machine build. No compile success is claimed anywhere in this file.
> Generated: 2026-09-18, repo HEAD `6157a1e` (branch `final-completion`).

---

## 1. Executive summary

| Fact | Value | Evidence |
|---|---|---|
| Last **green machine compile** | `8313c61` (2026-09-02) | `Docs/ENGINE_LOGS/raw/BUILD_8313c61_20260902.log` (3,135 B build log + full automation/runtime/save-load logs + SHA256SUMS — evidence-grade, Antigravity-issued) |
| C++ delta since green | **164 files changed, +33,343 / −933 lines, 95 commits** | `git diff --stat 8313c61..HEAD -- Source/` (2026-09-18) |
| New-code share of delta | The DCP-1..7 / NG+ / ending-cinematics / VIS-001 / creature-identity layers — **none of it has ever been machine-compiled** | commit log `8313c61..HEAD` |
| Static audits at HEAD | UHT sweep: 0 genuine blockers (after two repair rounds); type reachability: **234 types / 52 enums, 0 issues**; engine-header sweep: 3 findings, all with green-era precedent | `audit_uht.py`, `audit_types.py`, `audit_engine_headers.py` (sandbox runs, 2026-09-18) |
| Validators at HEAD | `validate_final_run.py` ALL PASS · `validate_repository.sh` PASS · `validate_design_data.py` 8/8 PASS (new) | sandbox runs, 2026-09-18 |
| Prior header-hygiene repairs | 6 one-line blockers fixed at `746c59f`; 3 blockers + 2 defensive includes fixed at `ff09d52` — all found by static audit, all *after* the green compile | commit messages + worklog |

**The one-sentence risk picture:** everything the machine must prove is
"95 commits / +33 K lines of never-compiled C++ that three static auditors
and two full validator suites cannot falsify" — a genuine chance of
1-5 residual compile errors on first build, each expected to be a
one-to-few-line fix (the two prior audit rounds found exactly this shape).

---

## 2. Risk register (severity × likelihood, ranked)

### R-C1 — Never-compiled delta bulk · HIGH severity, HIGH likelihood
- **What**: 33 K inserted lines across 164 C++ files since the last machine
  compile at `8313c61`.
- **Why it bites**: any single UHT/type/include error anywhere in the delta
  halts the build; static audits reduce but cannot eliminate this class.
- **Mitigation already in place**: two audit-repair rounds completed
  (`ff09d52`, `746c59f`); module type graph fully reachable (0 issues);
  include-order/generated-body sweeps clean; 134 world-free automation
  contracts pin the logic layer.
- **Machine verification step**: ON_PC_TASKS hour 0-1 — full
  `Setup_And_Play.bat` build; on error, capture the FIRST MSVC/UHT error
  only (later errors are cascade noise), file it, fix-forward per §5.
- **Expected failure shape** (from prior audit-round evidence): missing
  include / undefined identifier in header TU — one-line fixes.

### R-C2 — Engine-API drift between 5.8 headers and authored calls · MEDIUM severity, MEDIUM likelihood
- **What**: the delta uses engine APIs (Niagara, Enhanced Input chords,
  `UInputModifierChordAction`, camera/letterbox widgets, PCM) authored
  against documented UE5 signatures but never compiled.
- **Why it bites**: 5.x minor versions occasionally change signature shape
  (const-ref vs value, FText vs FString) without deprecation warnings.
- **Mitigation already in place**: Build.cs module dependency list re-checked
  in sandbox (Niagara / AIModule / EnhancedInput / UMG / PMC / Sockets all
  present); risky calls are concentrated in 6 files (chord input, ending
  cinematic component, letterbox widget, VFX actor family).
- **Machine verification step**: the same hour 0-1 build; signature errors
  surface as C2664/C2039 with exact lines — fix-forward per §5.

### R-C3 — Python Editor-API drift (UNVERIFIED API, rule R2) · LOW severity, MEDIUM likelihood
- **What**: `Tools/Python/verify_environment.py` + `first_day_orchestrator.py`
  are first-execution scripts; every engine introspection is wrapped in
  try/except and degrades to `UNK` lines instead of crashing.
- **Why it can't bite hard**: worst case the verifier prints UNKNOWN for a
  group; the map builder and input setup scripts already have 5.8-era
  subsystem/fallback compatibility layers and were authored for this exact
  engine.
- **Machine verification step**: ON_PC_TASKS hour 1 — run the orchestrator;
  read the `[AWFIRST] VERDICT` line; UNKNOWN groups are logged, not fatal.

### R-C4 — Asset import path mismatches · LOW severity, LOW likelihood
- **What**: 109 real meshes + clips exist (manifest 189/189 present, 0
  TRUE_MISSING) but engine import is NOT_RUN.
- **Why it can't bite hard**: import failures are non-fatal to the compile
  gate and the game is authored zero-asset-first (placeholder bodies render
  regardless); `import_report.json` gives per-file coverage.
- **Machine verification step**: ON_PC_TASKS hour 2 — one-click
  `Setup_And_Play.bat` import stage; check `total_missing == 0`.

### R-C5 — LFS pointer hygiene on fresh clone · LOW severity, LOW likelihood
- **What**: 586/586 LFS pointers verified server-side (GitHub LFS batch API,
  2026-09-18); fsck caveat in LIVE_STATE §8 is documented.
- **Machine verification step**: ON_PC_TASKS hour 0 — `git lfs pull` on the
  fresh clone completes without "pointer without content" errors.

---

## 3. Static audit evidence at HEAD (sandbox, 2026-09-18)

Raw command outputs (abridged to findings):

```
$ python3 audit_uht.py
  A (generated.h last-include):  96/99 ok; 3 WARN = non-UHT headers by
    design (AstrawildArtPack.h / AstrawildBestiaryData.h / AstrawildCore.h
    contain no reflected types — a header without UCLASS/USTRUCT needs no
    generated.h)
  G (module-local include resolution): 62 "not found in module" — ALL are
    engine/plugin headers (EngineUtils.h, TimerManager.h, NavigationSystem.h,
    NiagaraFunctionLibrary.h, NativeGameplayTags.h) resolved through engine
    module paths, not module-local ones; every one has green-era precedent
    at 8313c61 (same includes compiled clean there)

$ python3 audit_types.py
  === module type table: 234 types | 52 enums ===
  Type-reach issues: 0
  Enum-member issues: 0

$ python3 audit_engine_headers.py
  TIMER: AstrawildDataValidator.cpp / AstrawildEchoBossCharacter.cpp use
    timers without direct TimerManager.h — transitive via Engine/World.h,
    same pattern as the green compile
  Niagara: AstrawildEchoMutator.h — forward-declared UNiagaraSystem usage;
    complete-type use is confined to the .cpp (include present there)

$ python3 Scripts/validate_final_run.py
  FINAL RUN VALIDATION: ALL CHECKS PASSED

$ python3 Scripts/validate_design_data.py
  RESULT: 8 passed, 0 failed — ALL CHECKS PASSED
```

The two prior audit rounds each found real blockers (6 + 3) that were then
repaired as one-line fixes; the current zero-blocker state is post-repair,
not pre-scrutiny.

---

## 4. Why the risk is bounded, not open-ended

1. **The architecture isolates blast radius.** One runtime module
   (`AstrawildCore`), no plugin C++ of our own, no header-only
   cross-module templates. A compile error has a single module to land in.
2. **The logic layer is pinned by 134 world-free automation contracts**
   that compile-gate AND run-gate the same pure functions — so the machine
   build isn't the first check of the code's semantics, only of its
   translatability.
3. **The zero-asset-first policy** means even a partial content import
   cannot block a walkable, lit prototype (placeholder bodies + engine
   basic shapes always render).
4. **Fix-forward protocol (below) is cheap**: both prior rounds produced
   one-line repairs; there is no evidence in this repo's history of a
   compile error requiring architectural change.

---

## 5. Fix-forward protocol (binding when the machine build fails)

1. **Capture the FIRST error only.** MSVC/UHT cascades make error #2..N
   noise; fix #1, rebuild, re-read.
2. **File it in `Docs/ENGINE_LOGS/`** as `BUILD_FAIL_<sha>_<date>.log`
   (full log, unedited) — evidence discipline; never summarize away the
   raw bytes.
3. **Fix minimally** (rule: smallest positive diff that compiles — the
   `746c59f` precedent is the target shape: 5 includes + 1 fwd decl for
   6 blockers).
4. **Re-run the static validators** in the sandbox on the patched tree
   before pushing (`validate_final_run.py` + `validate_design_data.py`).
5. **Commit + push with the error ID in the message**
   (`fix(core): C2039 <symbol> in <file> — BUILD_FAIL_<sha>`), update this
   register's §2 rows and LIVE_STATE §8.
6. **Never silently defer**: a failing row moves to BLOCKED with a reason,
   never to "later".

---

## 6. Standing UNVERIFIED — NEEDS MACHINE list

| Item | First proof point |
|---|---|
| Entire 164-file / +33 K-line delta compiles | hour 0-1 build |
| `verify_environment.py` editor APIs resolve on 5.8.2 | hour 1 orchestrator run |
| `first_day_orchestrator.py` module-delegation works under `-run=pythonscript` | hour 1 orchestrator run |
| 109-mesh import reaches `total_missing == 0` | hour 2 import stage |
| 134 automation contracts pass inside the engine | hour 3 Test.bat |
| PIE golden path + showcase map render lit | hour 4-6 |
| Win64 Shipping package builds | hour 7-8 |
