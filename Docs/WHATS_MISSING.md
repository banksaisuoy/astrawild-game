# ASTRAWILD — What's Missing / Full Audit Report

**Date:** 2026-09-18
**Auditor:** GLM (Super Z session) — full repository inspection + completion-stage development
**Base commit inspected:** `94a398c` (main)
**Resulting commit:** this one — see `git log -1`

> **POST-MERGE ADDENDUM (2026-10-12; AUDIT-B synced 2026-10-19):** this audit was written on the `main` line
> where the suite count was 54 → 55. As of merge `1b03a39` the completion stage
> lives on `final-completion`, where the world-free suite is **141 contracts**
> (139 CGC-line contracts + the `ASTRAWILD.Quest.VictoryChainTermination` test shipped
> by the completion stage, unified at the INTEGRATION merge, + the FIX-1
> `ASTRAWILD.WorldEvent.NightRaidPoolContract` regression). The missing list in §3 is unchanged and still accurate; item 2 (first
> compile of the completion stage) is now folded into ENGINE-RUN-1 as the #1
> machine-day action.

---

## 1. Executive Summary

The repository held a **substantially complete C++ survival-adventure vertical slice** (~41,000 LOC, 145 source files, 54 automation tests, full quest chain, boss dungeon, save v4, villages, dialogue, economy, robotics). What actually blocked "a complete game that runs on your machine" was **not gameplay code** — it was:

1. a **Git LFS trap** (binary content invisible without git-lfs),
2. **no game-loop bookends** (no title screen, no victory screen, no death feedback),
3. **fragile hardcoded launch scripts**, and
4. **unverified post-certification source changes** (5 files edited after the last engine-verified commit).

All four are addressed in this commit. Details below.

---

## 2. Findings (what was actually broken / missing)

### F-1 — Git LFS pointer trap (CRITICAL — silently breaks fresh clones)
- **Finding:** 46 tracked files (all `ArtSource/Textures/*.png`, `Content/ThirdPerson/Lvl_ThirdPerson.umap`, `Content/ASTRAWILD/Maps/MainMap.umap`, `Content/Materials/M_Master_Surface.uasset`) are stored via Git LFS. A clone made **without git-lfs installed** produces ~130-byte ASCII pointer files instead of the real binaries — and the game then cannot boot (`GameDefaultMap` points at a broken file). This exact failure mode reproduced during this audit.
- **Good news:** the LFS objects **do exist on GitHub's LFS storage** (60 MB). `git lfs pull` restores everything. Verified: both maps are genuine UE packages (magic `0x9E2A83C1`, package GUIDs present), textures are real 1024×1024 PNGs, the material is a real package.
- **Fix shipped:** `Setup_FirstRun.bat` — installs/verifies LFS, pulls objects, and **validates real file sizes** (pointer files are <1000 bytes and are detected and rejected with instructions).

### F-2 — No game bookends (the "สมบูรณ์/complete game" gap)
- **Finding:** Booting the game dropped you straight into the world. No title screen, no new/continue flow (the `bAutoLoadLatestOnBeginPlay` flag existed but defaulted off with a comment "enable in a main-menu GameMode subclass later"). Completing the final quest (`Quest_VanguardProtocol`) fired an event nobody consumed. Dying froze input for 5 silent seconds.
- **Fix shipped (pure-C++ UMG, zero new assets, consistent with the project's widget conventions):**
  - `UAstrawildTitleMenuWidget` — title screen with **New Journey / Continue (save-aware) / Controls / Quit**, shown on standalone & packaged boots (skipped in PIE for iteration speed; skipped on dedicated servers). Pause-gated so the procedural world finishes generating behind it.
  - `UAstrawildVictoryScreenWidget` — raised when `VictoryQuestId` (default `Quest_VanguardProtocol`) completes; shows session stats (days survived, echoes bonded, journal species) and routes back into the free hunt. World is never torn down — matches the authored "chain closes — free hunt begins" design.
  - `UAstrawildDeathOverlayWidget` — "You fell to the Vale" + live respawn countdown, auto-hidden on respawn.
  - `AAstrawildPlayerController` extensions: screen lifecycle, input-mode/cursor discipline, `OnVictoryAchieved` broadcast, `IsAnyScreenOpen()` now covers the new screens. `AAstrawildPlayerCharacter` death/respawn hooks updated (respawn delay now sourced from `GameMode->RespawnDelaySeconds` instead of a magic `5.0f`).
  - New automation test `ASTRAWILD.Quest.VictoryChainTermination` (tests 54 → **55**) guards the chain-closer contract so content edits can't silently orphan the victory wiring.

### F-3 — Hardcoded launch tooling (breaks on any other machine/path)
- **Finding:** `Launch_ASTRAWILD.bat`, `Launch_ASTRAWILD_Editor.bat`, `Build.ps1` hardcoded `E:\AstrawildGame` and `E:\Epic Games\UnrealEngine`. Any clone to another directory fails. There was also no one-click packager and no first-run/LFS automation.
- **Fix shipped:**
  - `_FindEngine.bat` — engine locator (env override → HKLM launcher registry → HKCU source-build registry → common paths) used by every script.
  - `Setup_FirstRun.bat` (LFS bootstrap + content validation), `Launch_ASTRAWILD.bat`, `Launch_ASTRAWILD_Editor.bat` (relative `%~dp0` project paths), `Import_Art.bat` (one-click AwPipeline import), `Package_Game.bat` (RunUAT shipping Win64 → `Packaged\Windows`), `Build_Project.bat` (UBT compile only).

### F-4 — Unverified source drift after the last certified build
- **Finding:** The last engine-verified commit was `03c2fe6` (per `Docs/BUILD_STATUS.md`). After it, 5 files changed (+291/−126): `AstrawildPlayerCharacter.cpp`, `AstrawildBiomeDressingActor.cpp`, `AstrawildTerrainTileActor.cpp`, `AstrawildWorldBootstrapper.cpp`, `AstrawildGameMode.cpp` — including the "Manny mannequin" integration commit `94a398c`.
- **Static review result (this audit):** all diffs reviewed line-by-line; no compile blockers found (`TArray<const TCHAR*>` init-lists, `UDirectionalLightComponent` property access, `USkyLightComponent::bRealTimeCapture`, TSoftObjectPtr assignment from `LoadObject` are all valid UE 5.x patterns). The changes also keep graceful fallbacks (Manny → exosuit → procedural body).
- **Known dependency note:** `94a398c` references `/Game/Characters/Mannequins/...` (UE template mannequin) which is **not committed to this repo** — it only exists if present on the local machine. The code degrades gracefully (soft-fallback to `/Game/Characters/Survivor/SK_Survivor_Exosuit` after `Import_Art.bat`, then to the procedural PMC body). No action required, but a machine without template content simply sees the exosuit/procedural tier.

### F-5 — Smaller observations (no action needed now)
- `Config/DefaultEngine.ini` `GameDefaultMap=/Game/ThirdPerson/Lvl_ThirdPerson` — correct and the map is real (LFS-verified).
- The `A_Amb_*.wav` ambient loops and SFX (36 files) are real WAVs; the code dispatches them with soft refs and procedural fallbacks.
- Heightmaps (`Content/Heightmaps/*.r16`, 13 zones) are present for the optional editor-Landscape path.
- The AwPipeline (`Content/Python/AwPipeline/import_all.py`) imports 112 assets per `ArtSource/manifest.json`; report lands in `Saved/AwPipelineReport/import_report.json`.

---

## 3. What is genuinely still missing (post-this-commit honest list)

| # | Item | Detail | Priority |
|---|---|---|---|
| 1 | **Co-op multiplayer (1–4 players)** | Server-authoritative logic exists game-wide, but client RPC coverage is incomplete (gap H-12: eat/feed/equip/capture/command are not RPCs yet; Echo vitals not fully replicated). Single-player is complete; co-op needs a dedicated client-parity batch. | High (if co-op is a goal) |
| 2 | **First compile of the completion stage** | This commit adds ~1,000 LOC that were **statically reviewed but not engine-compiled** (no UE on the audit machine). Expect zero errors, but per project rules: compile on the UE 5.8 target machine and update `Docs/BUILD_STATUS.md` with the real result. | High (first action on machine) |
| 3 | **Professional animation layer** | Locomotion is code-driven single-node clips (no AnimBP). Works; will look stiff compared to AAA blending. | Medium |
| 4 | **BGM / music** | 36 SFX + ambient beds exist; no score. | Medium |
| 5 | **World map / minimap UI** | HUD + zone banners + POI discovery notifications exist; no spatial map. | Medium |
| 6 | **Manny template content** | Referenced by the last upstream commit but not in the repo (local-machine-only asset). Fallbacks cover it. | Low |
| 7 | **Localization** | UI strings are English-only. | Low |
| 8 | **Boss AI controller polish** | Boss phases/elements/hazards are data-driven and tested; the controller is minimal per Ultimate Gap Analysis H-1 remainder. | Low |

---

## 4. Recommended next session (on the UE 5.8 machine)

1. `git pull` → `Build_Project.bat` (or open editor) → confirm 0 errors; run automation (`Test.bat` / Automation tab): expect **55/55**.
2. PIE smoke: title screen skip (PIE), HUD, controls from `README.md`, die → death overlay → respawn input intact.
3. Standalone (`Launch_ASTRAWILD.bat`): title screen appears → New Journey → world behind → ESC pause works.
4. Optional: `Import_Art.bat` for the full art tier; then `Package_Game.bat` for the shipping exe.
5. Update `Docs/BUILD_STATUS.md` with the verified result (the project's own Definition-of-Done rule: a text report alone is not proof).
