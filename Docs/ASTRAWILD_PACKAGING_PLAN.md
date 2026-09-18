# ASTRAWILD PACKAGING & CI PLAN — LONG-RUN DIRECTIVE L8

> Grounded in the REAL tooling: `Tools/package_windows.bat` (the one-click
> Win64 SHIPPING packager) and `.github/workflows/release.yml` (the
> self-hosted release workflow). Everything below is statically checkable
> from the repo; anything that needs the machine is marked NEEDS MACHINE.

## 1. The packaging command (verbatim from Tools/package_windows.bat)

```
"%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" -ScriptsForProject="<repo>\ASTRAWILD.uproject" BuildCookRun ^
 -nocompileeditor -installed -nop4 ^
 -project="<repo>\ASTRAWILD.uproject" ^
 -cook -allmaps -stage -pak -package ^
 -clientconfig=Shipping ^
 -platform=Win64 ^
 -build ^
 -archive -archivedirectory="<repo>\Build"
```

Defaults (all overridable via environment):

| Variable | Default | Notes |
|---|---|---|
| `UE_ROOT` | `E:\Epic Games\UnrealEngine` | non-standard install path WITH A SPACE — every use in the .bat is quoted (`"%RUNUAT%"`, `"%PROJECT%"`), verified statically |
| `ASTRAWILD_ARCHIVE` | `<repo>\Build` | archive root; final tree lands in `Build\Windows\ASTRAWILD` |
| `PROJECT` | `<repo>\ASTRAWILD.uproject` | resolved via `%~dp0` (script's own dir) |

Pre-flight guards in the .bat (statically verified): missing `.uproject`
→ exit 1 with message; missing `RunUAT.bat` under `UE_ROOT` → exit 1 with
message; UAT errorlevel → exit 1; success → prints the executable path.

## 2. Expected artifacts

| Artifact | Path | Produced by |
|---|---|---|
| Game executable | `Build\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe` | UAT `-archive` |
| Staged content paks | `Build\Windows\ASTRAWILD\Content\Paks\*.pak` | `-cook -allmaps -stage -pak` |
| CrashReportClient / engine-side staged folders | alongside the binary | UAT stage |
| Release zip | `Build\ASTRAWILD-Win64-Shipping.zip` | workflow zip step (bsdtar) |

Staged zip contents = the archived `ASTRAWILD` folder: `ASTRAWILD.exe`,
`Binaries\Win64\*`, `Content\Paks\*` (single-file pak), plus UAT's staged
support files — a self-contained tree that runs on any Windows 10/11 x64
PC with no engine installed (stated in the .bat's success message).

Runtime data note: saves/config write under `%LOCALAPPDATA%\ASTRAWILD\`
(USaveGame default routing) — nothing is written back into the zip tree.

## 3. CI workflow (`.github/workflows/release.yml`) — static audit

| Check | Verdict | Evidence |
|---|---|---|
| Trigger | tags `v*` + manual `workflow_dispatch` | `on:` block |
| Runner | `[self-hosted, Windows, X64]` | matches the doc'd machine layout |
| LFS content pulled | YES | `actions/checkout@v4` with `lfs: true` — required: 586 LFS pointers (586/586 tracked per asset-truth) |
| Packaging step runs the SAME .bat | YES | `Tools\package_windows.bat` via `shell: cmd` — one command path, no CI-only fork |
| `UE_ROOT` variable wiring | SOUND | `env: UE_ROOT: ${{ vars.UE_ROOT }}`; when the repo variable is unset the env arrives empty, the .bat's `if "%UE_ROOT%"==""` guard falls back to the default path — both paths work |
| Zip method | SOUND | `tar -a -c -f` (bsdtar, built into Win10+) — avoids the 2 GB `Compress-Archive` limit; `-C Build\Windows ASTRAWILD` path matches the .bat's archive output exactly |
| Release attach | SOUND | `softprops/action-gh-release@v2`, `fail_on_unmatched_files: true`, `generate_release_notes: true` |
| Permissions | CORRECT | `permissions: contents: write` — minimum needed to create releases |
| YAML validity | PARSES | checked statically in this run |

Static audit found NO defects. What static checking CANNOT prove (NEEDS
MACHINE): that UAT itself succeeds (first compile!), that the runner
service account can read `E:\Epic Games\UnrealEngine`, that git-lfs is on
the service account PATH, and that the 2 GB+ zip uploads cleanly.

## 4. Why self-hosted (carried from the workflow's own record)

1. GitHub-hosted runners have no UE 5.8; the engine + DDC caches are a
   tens-of-GB local install and Epic's EULA ties source builds to the
   accepting account.
2. Compile + cook time is long even on the dev machine; hosted runners
   would multiply it every run and time out.
3. Reproducibility: the self-hosted runner reuses the exact engine + MSVC
   toolchain that built the project (ENGINE_LOGS evidence chain).

One-time runner setup (from the workflow comments): register a self-hosted
Windows runner (keep default labels), `.\svc install` + `.\svc start`,
ensure git + git-lfs on the service account PATH, optionally set the
`UE_ROOT` repository variable if the engine is not at the default path.

## 5. Troubleshooting table

| Symptom | Likely cause | Route |
|---|---|---|
| `[ERROR] RunUAT.bat not found` | wrong `UE_ROOT` | set `UE_ROOT` env (bat) or repo variable `UE_ROOT` (CI) to the engine root containing `Engine\Build\BatchFiles\RunUAT.bat` |
| UAT: "could not find platform tools" / MSVC errors | VS2022 workloads absent on the runner service account | run `Tools/preflight_check.bat` (L9) on that machine; install the C++ game-dev workload |
| UAT: compile errors in AstrawildCore | first real compile surfaced an issue | the compile gate is ON_PC hour 1-2; file the exact error against `Docs/ASTRAWILD_COMPILE_RISK.md` |
| Cook errors on missing assets | 63 opt-in GLB imports not run | run the first-day orchestrator (L5) BEFORE packaging; the runtime is fail-closed to BasicShapes but the cook must pass |
| LFS "smudge" errors during checkout | git-lfs missing on runner PATH | install Git LFS on the machine; re-run |
| Zip step: tar not recognized | pre-Win10 build environment | use any bsdtar/7z; the workflow targets Win10+ per the .bat's runtime claim |
| Release upload 413/timeout | zip too large for one upload | split paks or use `softprops` asset pagination; current estimate is a few GB — within GitHub's 2 GB asset guideline but monitor |
| Packaging succeeds but game black-screens | maps not cooked (-allmaps covers it) or DDC stall | check `Saved\Logs` on the target PC; re-run with `-DDC=NoShared` if DDC flakiness appears |

## 6. Recommended order (ties to ON_PC_TASKS v2)

1. `Tools/preflight_check.bat` (L9) — GO/NO-GO
2. first-day orchestrator (compile gate is implicit: the editor loads the
   module before any Python step)
3. automation tests (Test.bat)
4. PIE golden path on L_Proto_01 + L_Showcase_ArtOverhaul
5. `Tools\package_windows.bat` (local smoke)
6. `git tag v0.1.0 && git push origin v0.1.0` (CI release)
