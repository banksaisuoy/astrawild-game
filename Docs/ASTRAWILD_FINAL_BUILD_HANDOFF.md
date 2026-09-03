# ASTRAWILD — FINAL BUILD HANDOFF (Antigravity one-time integration)

> **Executability contract**: this document is a runbook, not a narrative. Every command is
> copy-pasteable on the Antigravity Windows machine. Machine assumptions: UE 5.8.2 at
> `E:\Epic Games\UnrealEngine`, repo at `E:\AstrawildGame`, .NET at `E:\dotnet`,
> archive at `E:\Astrawild_Packaged` (adjust if the workspace moved).
> Source state: `glm/final-run` — see §1 for the exact SHA.

> [!WARNING]
> **RECOVERY NOTE (2026-09-03)**: the commit SHAs listed in §1 were lost with the sandbox
> before push and do not exist on any remote. This runbook becomes operative again when the
> recovery re-implementation of FR-1..13 lands on branch **`final-completion`** (same base
> f31f5e1). All commands, engine assumptions, and verification sequences below remain valid
> — only the final SHA will differ. Check `git log --oneline -8 origin/final-completion` for
> the current redo state before executing.

---

## 1. FINAL COMMIT SHA

```
branch: glm/final-run   (base: f31f5e1 = agent/antigravity-ue5-v2 @ PR #4 head)
commits (oldest→newest):
  f310698  fix(core): P0/P1 source hardening — final run batch 1 (FR-0001..FR-0017)
  0ae9764  feat(story): Final Run — Act 3 'The Storm Crown' (ending + post-game)
  aee4cc8  feat(polish): building shell completes, living villages, element canon, skiff mesh
  <DOCS>   docs(final): MASTER_CONTROL v3.0 + task registry + handoff + readiness report
```
Verify on your machine after pulling: `git log --oneline -6` must show all of the above.
**PR #4 is subsumed** — merging `glm/final-run` into `main` closes it (do not re-merge PR #4 separately).

## 2. ENGINE VERSION

Unreal Engine **5.8.2** (EngineAssociation "5.8"), Windows 11, MSVC 14.44 (vs2022 toolchain),
EnhancedInput + GameplayAbilities plugins (project file), target `ASTRAWILDEditor Win64 Development`.

## 3. REQUIRED LFS STATE

`.gitattributes` routes `*.uasset/*.umap/*.glb/*.png/*.wav/...` through Git LFS.
**Verified in this sandbox: 459/459 LFS objects resolve with byte-exact sizes (233 MB).**
Pre-flight on Windows:
```powershell
git lfs install
git lfs ls-files | Measure-Object -Line        # expect 459
git lfs fetch --all                            # pulls every object
# spot-check one object resolves:
git show HEAD:Content/Vehicles/SM_Vehicle_DawnSkiff.uasset | Select-Object -First 3
```
If any object is missing: `git lfs pull origin` before building (builds with pointer files
instead of binaries fail loudly at asset load).

## 4. CLEAN CLONE PROCEDURE

```powershell
cd E:\
git clone https://github.com/banksaisuoy/astrawild-game AstrawildGame
cd AstrawildGame
git lfs install
git fetch origin glm/final-run
git checkout glm/final-run
git lfs pull
python Scripts/validate_final_run.py     # 29/29 static checks must PASS
bash Scripts/validate_repository.sh      # structural ruleset PASS
```

## 5. CONTENT PREP COMMAND

None required beyond LFS pull — all 115+ ArtPack assets are committed (§3), and the code
content library is self-registering (CODE_DEFAULT definitions at world begin-play).
Optional asset regeneration (only if ArtSource changes):
```powershell
python Tools\ArtSourceGen\gen_all.py          # regenerates .glb/.png/.wav sources
python Content\Python\AwPipeline\import_all.py  # re-import into Content (editor Python)
```

## 6. ASSET IMPORT COMMAND

Not needed for the final build (assets already in git). If a re-import is forced:
```powershell
& "E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" E:\AstrawildGame\ASTRAWILD.uproject -run=pythonscript -script="Content/Python/AwPipeline/import_all.py" -stdout -unattended
```

## 7. PROJECT GENERATION COMMAND

```powershell
& "E:\Epic Games\UnrealEngine\Engine\Build\BatchFiles\GenerateProjectFiles.bat" -project="E:\AstrawildGame\ASTRAWILD.uproject" -game -engine
```
(Only needed for a fresh IDE solution; UBT builds fine without it.)

## 8. BUILD COMMAND

```powershell
cd E:\AstrawildGame
.\Build.ps1
# equivalent direct form:
# & "E:\Epic Games\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" ASTRAWILDEditor Win64 Development -Project="E:\AstrawildGame\ASTRAWILD.uproject" -WaitMutex -FromMsBuild -NoUBA
```
**PASS = exit 0, 0 errors.** The previous integration attempt failed UBT at 8313c61 with
ExitCode 6 (FZ-A1) — if that recurs, capture `Engine\Intermediate\Build\BuildHistory` +
the UBT log tail BEFORE retrying; that is the artifact GLM needs to diagnose.

## 9. TEST COMMAND

```powershell
.\Test.ps1
# outputs E:\AstrawildGame\Automation_Output.txt
```
**PASS = 63/63 `Result={Success}`, 0 `Result={Fail}`.** New Final-Run contracts to watch:
`ASTRAWILD.Inventory.TransactionSafety` · `ASTRAWILD.Save.SchemaV5Ending` ·
`ASTRAWILD.Quest.FinalRunChain` · `ASTRAWILD.Echo.FinalRunBosses` ·
`ASTRAWILD.Tech.SkiffEngineering` · `ASTRAWILD.Dialogue.EndingChoice`.

## 10. PACKAGE COMMAND

```powershell
.\Build_Package.ps1
# archives to E:\Astrawild_Packaged
```
**PASS = RunUAT exit 0** (previous FZ-A1 failure was at the pre-Final-Run SHA; if cook
fails, save the full UAT log — do not retry blind).

## 11. LAUNCH COMMAND

```powershell
# PIE (editor): open Content/ASTRAWILD/Maps/MainMap.umap, Play
# Packaged:
E:\Astrawild_Packaged\Windows\ASTRAWILD.exe
```

## 12. EXPECTED FIRST SCREEN

Editor/PIE: MainMap loads → WorldBootstrapper builds the 12-zone world → camp with
workbench/campfire/rest point, 2 skiffs, Dawnstead village; HUD shows
Day 1 08:00, weather Clear, Research: 0 RP, quest tracker "First Light: Wood 0/10 …".
Log markers (approximate): `Content library defaults registered: 67 items, 49 recipes, 229 Echo…`,
`Final Run Act 3 content registered: …`, `Production V2 content registered: …`.
Packaged exe: same, after the loading screen.

## 13. EXPECTED PLAYER CONTROLS

WASD move · mouse look · Space jump · Shift sprint · Ctrl dodge · LMB attack (melee/ranged) ·
RMB block · E interact (nodes/NPCs/portals/skiff/capture/doors/crates/research) ·
Q capture · F scan · Tab inventory · C craft · B build mode (wheel/N/L cycle pieces) ·
K research · J journal · H deploy drone · V mount/dismount skiff (WASD + Space/Ctrl flight, Shift boost) ·
Esc pause (save/load/quit). Runtime Enhanced Input builds the mapping context (26 actions).

## 14. EXPECTED GAMEPLAY LOOP (golden path)

MQ-01 First Light (wood/stone) → MQ-02 First Echo (observe/capture Lumewisp) → MQ-03 base →
MQ-04 power → MQ-05 Gloomfangs → MQ-06 husbandry → MQ-07 Hollow Underlight + Warden →
MQ-08..12 Ember Ridge/isles/Vault/Colossus/epilogue gear → **MQ-13 The Three Anchors**
(read 3 POIs, kill Glass Tyrant in Sunscar) → **MQ-14 Crown Relay** (research Skiff
Engineering 25 RP, craft Stratos Coil, fly to the Eye Gate at 150 m) → **MQ-15 Eye of the
Maelstrom** (survey + 2 sentinels) → **MQ-16 The Drowned Sovereign** (2000 HP final boss) →
**MQ-17 First Dawn Again** (homecoming to Dawnstead) → **talk to Warden Maren → pick the ending**.

## 15. EXPECTED ENDGAME

Eye of the Maelstrom dungeon (5 rooms) above Stormcrest; Sovereign phases at 66%/33% + enrage
(180 s) + 2 sentinel adds + telegraphed AoE + hazard pools + weak-point windows; Light-element
weapons deal ×1.5. Loot_EyeCore drops the Sovereign Core + Maelstrom Glass + AncientCore ×2 +
DawnShard ×10. Post-boss: 25 RP + dungeon-complete log line.

## 16. FINAL BOSS

`Creature_DrownedSovereign` — defeat publishes Event.HostileDefeated (completes MQ-16),
boss bar shows "The Drowned Sovereign — Phase N" (per-boss display names are a Final-Run fix).

## 17. ENDING

Maren's dialogue → "Talk about the cage" (visible once MQ-17 is complete) →
A "Break the cage" (Ending_BreakCage): weather pins Clear permanently; HUD banner
"THE DAWN THAT STAYS / POST-GAME — the world is yours" ·
B "Let it sleep" (Ending_StormSleeps): storm remains; banner "THE STORM THAT SLEEPS".
One-way, server-authoritative, persisted (schema V5 `EndingState`); reload keeps it.
Post-game: world events, hunts, dungeons, automation and vendors keep running.

## 18. KNOWN MINOR ISSUES

- Door open/close visual on pure clients lags (bIsSwitchedOn has no OnRep) — fine in SP/listen-server.
- Imported skiff mesh orientation is unverified (glTF axis assumption) — cosmetic only; flip `HullMesh` relative yaw if reversed.
- Ranged damage = weapon profile + item ATK (additive by design, documented).
- MQ-13 Glass Tyrant killed pre-quest in one session: reload respawns it (world actor, unsaved) — documented completion path.
- Supply-crate withdraw needs pack weight headroom (by design; leftovers stay stored).

## 19. KNOWN ENGINE-ONLY RISKS

- UBT ExitCode 6 recurrence (FZ-A1) — capture UBA logs immediately if seen.
- 63 tests have never executed in a real engine.
- Eye dungeon floats 400 m up — verify no float-precision drift in room placement during PIE.
- Enhanced Input runtime mapping (26 actions) — verify no duplicate-context warnings in the log.
- Save schema 5 first migration (v4→v5) — run one old save through load to see the migration log line.

## 20. EXACT ANTIGRAVITY FINAL VERIFICATION SEQUENCE

```text
1  pull glm/final-run (§4) + git lfs pull + both static validators PASS
2  Build.ps1 exit 0 (§8)                         → raw log Docs/ENGINE_LOGS/raw/BUILD_<sha>.log
3  Test.ps1 63/63 (§9)                           → raw log Docs/ENGINE_LOGS/raw/AUTOMATION_<sha>.log
4  PIE boot (§12): confirm 3 content-registration log lines + no Error spam
5  PIE golden path (§14): MQ-01 quick-run (gather/craft/capture/build) + AW.* cheats to
   jump MQ-13..17: anchors → coil craft → Eye Gate → Sovereign kill → ending A
   (check: weather clears, banner shows, save) → load the save (banner persists) → ending B
   on a second save (storm stays)
6  Test_RealSaveLoad.ps1 (3-cycle persistence)   → raw log
7  Build_Package.ps1 exit 0 (§10)                → raw UAT log
8  Packaged exe boots to MainMap, input works    → raw RUNTIME_<sha>.log
9  commit raw logs + push glm/final-run:main; close PR #4 as absorbed
10 any engine-only defect: smallest fix on a branch; anything architectural → back to GLM
   with the logs; do NOT redesign systems
```

**Completion declaration**: when 1–9 pass, ASTRAWILD is GAME-COMPLETE (source-complete +
engine-verified + packaged). Record the final SHA + log manifest in
`Docs/ASTRAWILD_FINAL_READINESS_REPORT.md` §J.
