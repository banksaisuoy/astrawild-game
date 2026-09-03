# ASTRAWILD — FINAL BUILD HANDOFF (Antigravity one-time integration)

> **Executability contract**: this document is a runbook, not a narrative. Every command is
> copy-pasteable on the Antigravity Windows machine. Machine assumptions: UE 5.8.2 at
> `E:\Epic Games\UnrealEngine`, repo at `E:\AstrawildGame`, .NET at `E:\dotnet`,
> archive at `E:\Astrawild_Packaged` (adjust if the workspace moved).
> Source state: `final-completion` — see §1 for the exact SHAs (ALL PUSHED to GitHub).

> [!NOTE]
> **REDO LANDED (2026-09-03, Final Completion Run)**: the FR-1..14 re-implementation is
> complete on branch **`final-completion`** and every batch is pushed to origin (the
> recovery-era warning about lost SHAs is retired — §1 below now lists the live commits).
> All commands, engine assumptions, and verification sequences remain valid as written.

---

## 1. FINAL COMMIT SHA

```
branch: final-completion   (base: f31f5e1 = agent/antigravity-ue5-v2 @ PR #4 head)
commits (oldest→newest, ALL pushed to origin/final-completion):
  99e4105  [BATCH-0] Recovery: restore Final Run control docs + validator after sandbox loss
  61c45e6  [BATCH-1] P0 Hardening: exploit fixes, save guard, fail-closed building
  93ee929  [BATCH-2] Act 3 The Storm Crown: final boss + 2 endings + save V5
  b9c1bd6  [BATCH-3] Polish: buildings complete + every NPC converses + canon matrix
  1d65587  [BATCH-4] Docs: control set restored to final state (v3.2) + test inventory
  4622464  [BATCH-5] Final content manifest — READY_FOR_FINAL_BUILD declared
  1be6e20  [FINAL-AUDIT-A] P0/P1 source fixes — 11 defects (drone compile/crash, POI/boss
           quest back-fill, MQ-17 ending gate, view-axis aiming, crafting screen wiring,
           owner identity, robot chassis, camp respawn, CampKitchen, MainMap default)
  69a1d65  [FINAL-AUDIT-B] Medium/low hardening — 20 defects (element canon unification,
           echo health persist, species DefeatLoot live, research sanitize, AI perception)
  a5aa74d  [FINAL-AUDIT-C] +5 regression contracts (67 → 72 tests)
  <TIP>    [FINAL-AUDIT-D] This documentation batch — the branch TIP (docs + registry +
           readiness re-issued at the final state). `git ls-remote origin final-completion`
           gives the exact tip SHA; `git log --oneline -12` must show the full list above.
```
**PR #4 is subsumed** — merging `final-completion` into `main` closes it (do not re-merge
PR #4 separately).

## 2. ENGINE VERSION

Unreal Engine **5.8.2** (EngineAssociation "5.8"), Windows 11, MSVC 14.44 (vs2022 toolchain),
EnhancedInput + ProceduralMeshComponent plugins (project file; GameplayAbilities/StateTree were
disabled in the final audit — no code referenced them), target `ASTRAWILDEditor Win64 Development`.

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
git checkout final-completion              # the default clone already lands here (origin HEAD follows main; checkout explicitly)
git pull origin final-completion
git lfs pull
python Scripts/validate_final_run.py     # 46/46 static checks must PASS (72-test gate included)
bash Scripts/validate_repository.sh      # structural ruleset PASS
```
Final-audit note: the earlier text pointed at `glm/final-run`, a branch that never reached
GitHub and no longer exists. The live integration branch is **`final-completion`** (§1).

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
**PASS = 72/72 `Result={Success}`, 0 `Result={Fail}`.** Contracts to watch:
`ASTRAWILD.Inventory.TransactionSafety` · `ASTRAWILD.Save.SchemaV5Ending` ·
`ASTRAWILD.Quest.FinalRunChain` · `ASTRAWILD.Echo.FinalRunBosses` ·
`ASTRAWILD.Tech.SkiffEngineering` · `ASTRAWILD.Dialogue.EndingChoice` ·
`ASTRAWILD.Quest.OneShotBackFill` · `ASTRAWILD.Quest.DefeatCountImportSafety` ·
`ASTRAWILD.Quest.DismantleIsNotPlacement` · `ASTRAWILD.Research.ImportSafety` ·
`ASTRAWILD.Save.FinalAuditContracts` (the last five pin final-audit fixes).

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

Final-audit correction — this list now matches the actual runtime bindings
(`BuildRuntimeInputDefaults`, 26 actions):
WASD move · mouse look · Space jump · Shift sprint · **Q dodge** · LMB attack (melee/ranged;
confirm while building) · F heavy attack · RMB block (guard pose = aim stance) ·
**E interact** (nodes/NPCs/portals/skiff/capture-wild-echo/doors/crates/research-desk/
workstations/**crafting stations — opens the crafting screen**) · **C party command**
(not craft) · R feed target echo · G smart-consume · X equip-best · V scan (hold) ·
B build mode (wheel cycle · N rotate · Z dismantle) · Tab inventory (TAB/ESC close) ·
**K research** (K/ESC close) · H deploy/recall drone · J deploy robot · F5 save · F9 load ·
Esc pause (ESC resumes) · LeftCtrl skiff descend (board/dismount with E; Shift boost).
Crafting is done at stations via the crafting screen (E), not a hotkey.
Journal data exists (scanner observation) but has **no viewing UI yet** (known gap, §18).

## 14. EXPECTED GAMEPLAY LOOP (golden path)

MQ-01 First Light (wood/stone) → MQ-02 First Echo (observe/capture Lumewisp) → MQ-03 base →
MQ-04 power → MQ-05 Gloomfangs → MQ-06 husbandry → MQ-07 Hollow Underlight + Warden →
MQ-08..12 Ember Ridge/isles/Vault/Colossus/epilogue gear → **MQ-13 The Three Anchors**
(read 3 POIs, kill Glass Tyrant in Sunscar — the Tyrant drops its authored loot now) →
**MQ-14 Crown Relay** (research Skiff Engineering 25 RP, craft Stratos Coil at a crafting
station, fly to the Eye Gate at 150 m) → **MQ-15 Eye of the Maelstrom** (arrive + survey) →
**MQ-16 The Drowned Sovereign** (2000 HP final boss) → **MQ-17 First Dawn Again**
(homecoming marker at Dawnstead) → **talk to Warden Maren → pick the ending** (the crown
choices appear only after MQ-17 — the audit re-gated them to canon).

PIE shortcut for the tail: `AW.FastForward Quest_TheDrownedSovereign` completes the chain
through MQ-16 (rewards fire exactly once), then walk the MQ-17 marker for the ending.

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
- One-shot bosses (Warden/Colossus/Tyrant/Sovereign) killed pre-quest: the quest now BACK-FILLS
  from lifetime defeat counters (final audit G-3) — no dead objectives; the counters persist.
- Supply-crate withdraw needs pack weight headroom (by design; leftovers stay stored).
- Co-op pure-client gaps (documented, deferred CV-6): inventory Items not replicated, E-interact
  family and screen actions are host/listen-server only, research screen reads host state.
  Single-player and listen-host are the supported configurations.
- Journal/bestiary viewing UI and radar compass are not implemented (data + scanner exist);
  PLAYABLE_BUILD_STATUS.md "PRODUCTION READY" claims for them are HISTORICAL (superseded).
- Overworld defeated creatures leave corpses until the spawner recycles populations (actor
  accumulation is bounded by the spawner's population caps).

## 19. KNOWN ENGINE-ONLY RISKS

- UBT ExitCode 6 recurrence (FZ-A1) — capture UBA logs immediately if seen.
- 72 tests have never executed in a real engine (the audit's C-1 drone fix removed a likely
  build blocker; the first compile is the real proof).
- Eye dungeon floats 400 m up — verify no float-precision drift in room placement during PIE.
- Enhanced Input runtime mapping (26 actions) — verify no duplicate-context warnings in the log.
- Save schema 5 first migration (v4→v5) — run one old save through load to see the migration log line.
- The final audit changed the element weakness of 151 bestiary rows + 4 authored species and
  unified the boss resist to ×0.80 — combat feel needs the PIE pass more than ever.

## 20. EXACT ANTIGRAVITY FINAL VERIFICATION SEQUENCE

```text
1  pull final-completion (§4) + git lfs pull + both static validators PASS (46/46)
2  Build.ps1 exit 0 (§8)                         → raw log Docs/ENGINE_LOGS/raw/BUILD_<sha>.log
3  Test.ps1 72/72 (§9)                           → raw log Docs/ENGINE_LOGS/raw/AUTOMATION_<sha>.log
4  PIE boot (§12): confirm 3 content-registration log lines + no Error spam
5  PIE golden path (§14): MQ-01 quick-run (gather/craft at the station screen/capture/build)
   + AW.FastForward Quest_TheDrownedSovereign to jump the chain: MQ-17 homecoming marker →
   ending A (check: weather clears, banner shows, save) → load the save (banner persists) →
   ending B on a second save (storm stays)
6  Test_RealSaveLoad.ps1 (3-cycle persistence)   → raw log
7  Build_Package.ps1 exit 0 (§10)                → raw UAT log
8  Packaged exe boots to MainMap, input works    → raw RUNTIME_<sha>.log
9  commit raw logs + push final-completion:main; close PR #4 as absorbed
10 any engine-only defect: smallest fix on a branch; anything architectural → back to GLM
   with the logs; do NOT redesign systems
```

**Completion declaration**: when 1–9 pass, ASTRAWILD is GAME-COMPLETE (source-complete +
engine-verified + packaged). Record the final SHA + log manifest in
`Docs/ASTRAWILD_FINAL_READINESS_REPORT.md` §J.


## GDP Playtest Additions (v3.4)

During the PIE golden path, additionally verify:
1. Press **T** near hostiles with a captured party — every party Echo should visibly cast (projectile/heal/shield per its loadout; HUD line shows readiness counts).
2. Press **Y** — the smart-cast should fire (or log "nothing ready" early on; after Might 3 etc. it lights up). Verify Power Strike doubles the next swing's damage number.
3. Capture a flying species (Avian family) — it should path through the air after capture (follow command), not walk.
4. Talk to a vendor twice on two different in-world days — affinity tiers should climb and the purchase price should drop at tier 1+ (up to -15%).
5. Save + load — attribute levels and NPC affinity must survive the round-trip (tests 81/83 pin the logic; PIE confirms serialization).
6. Automation now expects **84/84** (was 72).
