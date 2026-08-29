# ASTRAWILD — Build Status

## Status

- Overall: `PARTIAL`
- Last updated: 2026-08-28
- Branch: `main`
- Latest change: capture-chance system, Echo damage/defeated events, web playable slice reference

## Environment

- Unreal Engine: Not run in sandbox environments; target is 5.8
- Compiler: Not run in sandbox environments
- OS: Repository validation run in Ubuntu sandbox; target build is Windows
- CPU/GPU/RAM/Storage: Not measured

## Compile

- Target: `ASTRAWILDEditor Win64 Development` — pending Antigravity
- Result: `NOT_RUN`
- Errors: Not measured; Unreal Editor unavailable in sandbox environments
- Warnings: Not measured
- Build duration: Not measured

Static repository validation passed with `Scripts/validate_repository.sh`.

## Changes in this round (2026-08-28, Z.ai Code)

| Change | File | Purpose |
|---|---|---|
| `FAstrawildEchoStats::CaptureResilience` | `AstrawildTypes.h` | Data-driven capture difficulty per species |
| `OnDamaged` / `OnDefeated` delegates | `AstrawildEchoCharacter.h/.cpp` | Enables AI reaction (flee/alert) and UI feedback per gameplay spec §4 |
| `GetHealthFraction()` / `ComputeCaptureChance()` | `AstrawildEchoCharacter.h/.cpp` | Capture reads the situation: weaken first or build trust, never at full HP, never defeated |
| Chance-based capture roll + `PreviewCaptureChance()` | `AstrawildCaptureComponent.h/.cpp` | Closes the design gap where a full-health Echo could be captured unconditionally |
| Web Playable Slice reference | `Docs/WEB_PLAYABLE_SLICE.md` | Browser prototype of the Vertical Slice loop for design validation |

All changes are additive and compile-safe (no signature changes to existing public functions).

## Unreal assets created by Antigravity

| Asset | Path | Status | Notes |
|---|---|---|---|
| Player Blueprint |  |  |  |
| Echo definitions |  |  |  |
| Input assets |  |  |  |
| Prototype map |  |  |  |
| UI |  |  |  |

## Playtest

| Test | Result | Notes |
|---|---|---|
| Open project | NOT_RUN |  |
| Compile Development Editor | NOT_RUN |  |
| Player movement/camera | NOT_RUN |  |
| Interaction | NOT_RUN |  |
| Harvest resource | NOT_RUN |  |
| Capture Echo | NOT_RUN |  |
| Craft recipe | NOT_RUN |  |
| Activate rest point | NOT_RUN |  |
| Save snapshot | NOT_RUN |  |
| Load snapshot | NOT_RUN |  |

A browser-based playable slice of the core loop (explore → harvest → capture → craft → rest → save) is
documented in `Docs/WEB_PLAYABLE_SLICE.md`. It is a design-validation prototype only; the source of
truth for gameplay rules remains this C++ module.

## Known issues

| Severity | Issue | File/asset | Reproduction | Owner/next action |
|---|---|---|---|---|
| High | No `.umap`/Blueprint/Data Asset binaries yet | `Content/ASTRAWILD/` | Open project → empty content tree | Antigravity: M1 map creation |
| Medium | Capture chance formula not yet playtested in-engine | `AstrawildCaptureComponent.cpp` | Capture weakened Echo repeatedly | Tune `CaptureResilience` per species after playtest |

## Handoff to Antigravity

The C++ core, placeholder combat visuals, data contracts, save schema, validation script, visual
target, game-dev workflow, and Antigravity instructions remain committed on `main`. Antigravity must
still pull, compile `AstrawildCore`, create the required Blueprint/Data Asset/Map binaries, and fill
this report with real test results. Do not mark `COMPLETE` until Compile, map/Blueprint creation,
core-loop Playtest, and Save/Load have all passed.
