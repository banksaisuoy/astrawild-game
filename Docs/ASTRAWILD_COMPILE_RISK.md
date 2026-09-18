# ASTRAWILD COMPILE_RISK — LONG-RUN DIRECTIVE L4 output

> Static-linter findings against the 197 Source files (99 .h + 98 .cpp) that
> have never been seen by a compiler. Tool: `Scripts/lint_unreal_cpp.py`
> (9 check families A–I). Nothing here claims anything compiles — R3 stands:
> the first real MSVC compile is still the only proof.

---

## 1. Before / after

| Metric | First full pass | Final pass |
|---|---|---|
| ERROR-severity issues | 93 (all one root cause: linter bug — see §3) | **0** |
| WARNINGS | 3,954 (≈99% linter false positives) | **1** |
| Real compile blockers found & fixed | — | **2** (§2) |

The first-pass numbers were dominated by linter defects (trailing-quote
include matching, forward-declaration misattribution, comment prose parsed
as declarations). Each linter fix is itself listed in §3 for honesty — the
tool earned its final precision through 8 iterations of bug-fixing against
ground truth.

## 2. FIXED — unambiguous compile blockers (minimum diff, this run)

### CR-1 — `UAstrawildPlayerController` does not exist (would not compile)

- **Evidence**: `AstrawildPlayerController.h:30` declares
  `class ASTRAWILDCORE_API AAstrawildPlayerController : public APlayerController`
  — the only player-controller class in the module. The DCP-3 ending-cinematic
  code referenced a *nonexistent* `UAstrawildPlayerController`:
  - `AstrawildEndingCinematicComponent.h:74` —
    `class UAstrawildPlayerController* GetAstrawildController() const;`
  - `AstrawildEndingCinematicComponent.cpp:59` — return type
  - `AstrawildEndingCinematicComponent.cpp:61` — `Cast<UAstrawildPlayerController>(GetOwner())`
- **Why it cannot compile**: the header forward-declares the wrong name, so
  `UAstrawildPlayerController` is an incomplete type; `Cast<T>` requires the
  complete type (instantiates `T::StaticClass()` path). Hard error at the
  DCP-3 translation unit. Semantically wrong too — a controller is an
  A-prefixed actor.
- **Fix applied (3 lines, name-only)**: all three sites renamed to
  `AAstrawildPlayerController`. The forward declaration in the header now
  matches the real class; the .cpp already includes
  `AstrawildPlayerController.h`.

### CR-2 — `AAstrawildPlayerController` used with no reachable include

- **Evidence**: `AstrawildSaveSubsystem.cpp` uses `AAstrawildPlayerController`
  in real code at lines 182, 184, 190, 192, 936, 1370, 1393, 1511, 1525
  (co-op save blocks, `Cast<AAstrawildPlayerController>`) but its include
  list has `AstrawildPlayerCharacter.h` and NO path to
  `AstrawildPlayerController.h` (PlayerCharacter.h itself includes only
  CoreMinimal/Character/Types — verified). Incomplete-type hard error.
- **Fix applied (1 line)**: `#include "AstrawildPlayerController.h"` added
  directly after the PlayerCharacter include.

Post-fix regression: ALL validators re-run green —
`validate_design_data.py` 78/78, `validate_final_run.py` ALL PASS,
`dryrun_unreal_tools.py` 28/28, extractor census 15/15, linter 0 errors.

## 3. Linter defects fixed during L4 (recorded for honesty)

1. `.generated.h` include matched without the trailing quote → 90 false errors.
2. `[AUF]` type-extraction regex omitted `E` (enums) and matched container
   fragments (`Array` inside `TArray`).
3. Forward declarations (`class X;`) and inline-qualified method decls
   (`class X* Get() const;`) were misattributed as type DEFINITIONS via
   `setdefault`, corrupting the symbol table (fixed by requiring `:`/`{`/EOL
   after the name).
4. `meta=(ClampMin="0", ClampMax="10")` split on commas → ClampMax flagged
   as an unknown specifier (fixed with a paren-aware split).
5. Header/impl drift matched definitions by FILE stem (missing the U/A class
   prefix) → 1,284 false positives (fixed: any-qualifier + RPC
   `_Implementation`/`_Validate` suffixes).
6. Comment prose (`NPC lines play in order (click...)`) parsed as method
   declarations (fixed: block-comment state tracking).
7. Comment-only type mentions flagged as unresolved usages (same fix).
8. RPC methods (`ServerX`/`ClientX`) declared without `_Implementation`
   bodies flagged (fixed by suffix-aware search).

## 4. OPEN — ambiguous items, ranked by confidence

| # | Confidence | Item | Evidence | Proposed fix (NOT applied — needs owner/machine) |
|---|---|---|---|---|
| O-1 | HIGH (dead code, zero link risk) | `AstrawildItemRegistrySubsystem.h:224` — `void BuildContentDefaults();` declared, never defined, never called | `rg BuildContentDefaults Source/` → exactly 1 hit (the declaration) | Delete the declaration (1-line diff) — or implement if it was meant as a content-bootstrapping seam. Left in place: removing a public header member is owner-territory. |
| O-2 | MEDIUM (engine conformance) | `Tools/Python/_mock_unreal` symbol `RowStruct` is UNVERIFIED (49/50 mock symbols VERIFIED — see L3 harness output) | UE 5.8 Python docs not checkable from this sandbox; symbol unused by current tools, reserved for L5 DataTable generation | Verify `unreal.RowStruct`/DataTable struct-pick API name on the first engine run (ON_PC hour 0); adjust L5 tooling before running it. |
| O-3 | MEDIUM (API drift, unverifiable here) | EnhancedInput `BindAction` overloads taking `(UObject*, FName)` were deprecated in later UE5 releases; the project compiles against 5.8 headers | Source uses delegate-style bindings (`BindAction(ValueAction, ...)`) in `AstrawildPlayerCharacter` — no FName-overload call sites found by the linter's citable-pattern scan | None needed statically; if the 5.8.2 compiler emits deprecation warnings, they are warnings, not blockers. |
| O-4 | LOW (residual static-analysis limits) | The linter cannot prove: template instantiation errors, UHT-generated code expectations (e.g. `TObjectPtr<>` vs raw ptr in UPROPERTY under 5.8 default settings), PCH policy interactions, linker symbol dedup | By construction — these need MSVC + UHT | The machine compile in ON_PC hour 1-2 is the only closer. |
| O-5 | LOW | 8 log categories DECLAREd in `AstrawildLog.h`, 8 DEFINEs found, 0 duplicates, 0 missing — clean (check I green) | linter output §I | none |

## 5. Checks performed and clean (the A–I families)

- **A** macro integrity: 119 UCLASS bodies all carry `GENERATED_BODY()`;
  44 USTRUCTs carry `GENERATED_BODY()` (modern form, valid in UE5); 49 UENUMs
  are `enum class : uint8`; no macro-ordering violations.
- **B** all 93 reflected headers end with their `.generated.h` include.
- **C** every UPROPERTY member type resolves (0 unresolved after symbol-table
  fixes — the C-check is the one that exposed CR-1/CR-2's siblings).
- **D** 0 unknown UPROPERTY/UFUNCTION specifiers; 0 known-bad specifier
  combinations (the 57 first-pass hits were the meta-comma bug, §3.4).
- **E** include-closure: after fixes, every project-type usage resolves
  transitively EXCEPT the two real blockers (now fixed) — currently 1
  comment-proof residue: none.
- **F** header/impl drift: 1 remaining (O-1); BlueprintImplementableEvent
  methods exempted by design.
- **G** module closure: all 8 used engine modules present in
  `AstrawildCore.Build.cs` (Niagara, EnhancedInput, ProceduralMeshComponent,
  NavigationSystem, AIModule, UMG, GameplayTags, Sockets).
- **H** API drift patterns: 0 hits for `TBaseDelegate`,
  `FPostConstructInitializeProperties`, `FApp::GetGameTime`, legacy
  `Runtime/`-prefixed includes.
- **I** log categories: 8/8 DECLARE→DEFINE pairs, no duplicates, no orphans
  (the historical FCR-1-b double-declare fix holds).

## 6. Standing rule

R3 unchanged: "static level — engine build/test still required on the target
machine." This document narrows the UNKNOWN from "33K unreviewed lines" to
"1 dead declaration + engine-conformance residuals (O-2..O-4)."
