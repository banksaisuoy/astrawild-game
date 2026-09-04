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
  d20152b  [FINAL-AUDIT-D] Docs: ONE truth + executable handoff
  baca0f6  [GDP] Gameplay Depth Pack — echo abilities/locomotion/attributes/affinity (84 tests)
  078c662  [GDP] Docs: manifest v1.2 amendment + GDP playtest additions
  a7a827f  [SCP-1] Runtime hardening — DataValidator/ErrorReporter/AssetFallback/durability/spoilage
  394ac81  [SCP-2] Base Terminal + creature sanity & healthcare
  edc6b08  [SCP-3] Creature mounting (Phase 5)
  6cd29e4  [SCP-4] Dual-tech combo reactions + dynamic difficulty (Phase 6/3)
  bbe2e3c  [SCP-5] Living world — NPC schedules, crops, offline production, turrets
  9864cce  [SCP-6] Breeding genetics + dynamic performance enforcement (99 tests)
  f9892b6  [SCP-7] Docs: plan-vs-repo audit matrix, master control v3.5 + registry §F
  8a3a0da  [FCR-0] Registry reconciliation — one authoritative value per metric
  9bca989  [FCR-1-A] Audit fixes: 2 compile blockers + 13 HIGH defects (GDP+SCP deep audit)
  30e9e44  [FCR-1-B] Audit fixes batch B: 10 MEDIUM + 11 LOW defects
  aea01ed  [FCR-1-C] +3 regression contracts (99 -> 102 tests) + exact test gate + docs sync
  43429a7  [FCR-FINAL] Phases 2-18 complete — READY_FOR_FINAL_BUILD re-affirmed
  26a7c7b  [AA] Asset Acquisition Pack — 6 CC0 Kenney packs (1071 source files, 43.4MB)
  a09e566  [AA-2] Asset Acquisition Batch 2 — 9 CC0 Kenney packs (2607 files, 32.4MB)
  981250d  [DP-1] Creature Visual Strategy — Tier A/B/C over 229 species + registry §I ledger
  a2e7783  [DP-2] Content Integration Matrix — 14-category readiness + per-pack tables
  c4012a0  [DP-1x] Tier-A boss meshes ×4 — DrownedSovereign/GlassTyrant/Dawnfang/EyeSentinel
  d9ebf86  [DP-1x] Tier-A story meshes ×4 — Lumewisp/Sprigling/Gloomfang/Auroraling
  675e5b4  [DP-1b] Boss opt-in skeletal body + §20c binding-patch sequence (this file)
  ffc7eca  [DP-3] Echo depth — locomotion signature abilities + party resonance + water mounts (+test 103)
  e6607b6  [DP-4] Player skill loadout — 3-slot bound actives (+test 104)
  8771519  [DP-5] Combat depth — weak points + weakness feedback + boss special sets (+test 105)
  89bd714  [DP-6] Base depth — 4 work sites + field consumables (+test 106)
  0087047  [DP-7] World depth — 7 zone events + hazards + 4 secret POIs (+test 107)
  0710dd0  [DP-8] NPC depth — affinity-gated dialogue + regional knowledge (+test 108)
  018a95a  [DP-9] Dungeon depth — room themes + puzzle rooms + room hazards (+test 109)
  <TIP>    [DP-10] Final gate — source audit PASS + readiness matrix/report/handoff/master-
           control v5.0 + registry closed (this docs batch; `git ls-remote origin
           final-completion` gives the exact tip SHA; `git log --oneline -30` must show
           the full list above).
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
python Scripts/validate_final_run.py     # ALL static checks must PASS (109-test gate + 15 census equality gates included)
bash Scripts/validate_repository.sh      # structural ruleset PASS
```
Final-audit note: the earlier text pointed at `glm/final-run`, a branch that never reached
GitHub and no longer exists. The live integration branch is **`final-completion`** (§1).

## 5. CONTENT PREP COMMAND

None required beyond LFS pull — all ArtPack assets are committed (§3), and the code
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
**PASS = 109/109 `Result={Success}`, 0 `Result={Fail}`** (count read from the repo — the validator gate pins the exact value). Contracts to watch:
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
Log markers (approximate): `Content library registered (live census): 78 items, 58 recipes, 229 Echo species, 26 buildings, 17 technologies, 17 quests, 11 loot tables, 11 NPCs, 8 weapon profiles, 10 resource nodes, 8 work sites, 16 world events, 17 POIs, 12 biomes, 11 dialogue trees, 3 robots.` (numbers are counted LIVE from the registry — the census line is the engine-side authority; if any number differs from this doc the log wins),
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
- 109 tests have never executed in a real engine (the audit's C-1 drone fix removed a likely
  build blocker; the first compile is the real proof).
- Eye dungeon floats 400 m up — verify no float-precision drift in room placement during PIE.
- Enhanced Input runtime mapping (26 actions) — verify no duplicate-context warnings in the log.
- Save schema 5 first migration (v4→v5) — run one old save through load to see the migration log line.
- The final audit changed the element weakness of 151 bestiary rows + 4 authored species and
  unified the boss resist to ×0.80 — combat feel needs the PIE pass more than ever.

## 20a. PIPELINE IDEMPOTENCY CONTRACT (Phase 14 — deterministic by construction)

The full chain §4→§11 (CLONE → LFS PULL → VALIDATORS → CONTENT PREP → ASSET IMPORT
→ PROJECT GENERATION → BUILD → TEST → PACKAGE) is safe to run TWICE on the same
working tree:

- **Asset import (§6)**: `import_all.py` guards every task with
  `does_asset_exist()` — a second run re-imports NOTHING, only re-loads assets and
  re-applies the same texture/material properties (same values = no change).
  Collision safety additionally uses `replace_existing=True`.
- **Project generation (§7)**: UnrealVersionSelector regenerates the identical
  `.sln`; UBT re-links only what changed (no-op when clean).
- **Build (§8) / Test (§9) / Package (§10)**: standard UBT/UAT incremental steps —
  a clean re-run is a no-op or an overwrite-in-place, never a duplication.
- **Validators**: pure read-only static checks — any number of runs is safe and
  MUST PASS before every stage transition.
- **Drift tripwires**: the validator's census equality gates (15 content-count
  contracts + the exact 109-test gate) fail loudly if a pipeline stage ever
  duplicated or dropped content.

A second full execution of the sequence therefore converges to the same state —
no duplicated assets, no double imports, no corrupted Content.

## 20. EXACT ANTIGRAVITY FINAL VERIFICATION SEQUENCE

```text
1  pull final-completion (§4) + git lfs pull + both static validators PASS (validate_repository + validate_final_run ALL)
2  Build.ps1 exit 0 (§8)                         → raw log Docs/ENGINE_LOGS/raw/BUILD_<sha>.log
3  Test.ps1 109/109 (§9)                           → raw log Docs/ENGINE_LOGS/raw/AUTOMATION_<sha>.log
4  PIE boot (§12): confirm 3 content-registration log lines + no Error spam
5  PIE golden path (§14): MQ-01 quick-run (gather/craft at the station screen/capture/build)
   + AW.FastForward Quest_TheDrownedSovereign to jump the chain: MQ-17 homecoming marker →
   ending A (check: weather clears, banner shows, save) → load the save (banner persists) →
   ending B on a second save (storm stays) + the 12 golden-path verify items below (GDP + DP-3..DP-9)
6  Test_RealSaveLoad.ps1 (3-cycle persistence)   → raw log
7  Build_Package.ps1 exit 0 (§10)                → raw UAT log
8  Packaged exe boots to MainMap, input works    → raw RUNTIME_<sha>.log
9  commit raw logs + push final-completion:main; close PR #4 as absorbed
10 any engine-only defect: smallest fix on a branch; anything architectural → back to GLM
   with the logs; do NOT redesign systems
```

**Asset sub-sequences inside this run (not optional, not blockers)** — both execute
inside the step 4-5 PIE window, in this order, with their own report-back rules:

- **§20b acquired-asset checklist** (Kenney imports + fitness/retarget/tone checks +
  decision reporting) — run AFTER the baseline `import_all.py` pass and BEFORE any
  binding decision; its 6 steps are self-contained.
- **§20c Tier-A creature-mesh import + binding patch** (14 bespoke echo meshes, 8 of
  them new) — run AFTER the §20b baseline pass: import → commit `.uasset`s (LFS) →
  apply the verbatim `GetEchoArt()` binding rows → re-run both validators → PIE
  spot-check. The cone placeholder stays until the mesh resolves — that is the
  contract, not a defect.

**Completion declaration**: when 1–9 pass, ASTRAWILD is GAME-COMPLETE (source-complete +
engine-verified + packaged). Record the final SHA + log manifest in
`Docs/ASTRAWILD_FINAL_READINESS_REPORT.md` §J.

## 20b. ACQUIRED-SOURCE ASSET CHECKLIST (AA batches 1+2 — Antigravity verification)

The 15 Kenney CC0 packs (3,678 accepted source files, 75.8 MB — `ASSET_MANIFEST.json`
is the authoritative per-file record with SHA256s) are IMPORT-READY sources, NOT
engine-verified content. Verify them during the ONE-TIME integration run as follows —
all steps ride the EXISTING Interchange importer; do not build a second importer.

1. **Baseline first (unchanged contract)**: run the standard `import_all.py` pass
   BEFORE any Kenney import — the flat-contract procedural import (120 files on disk at
   DP-10: the 112-asset contract + the 8 DP-1 Tier-A echo meshes — §20c imports those
   exactly like the 6 heroes), soft-path fallbacks and zero-asset boot guarantee must
   be intact (§20a idempotency).
2. **Import the Kenney sources** (Interchange, `replace_existing=True`,
   `does_asset_exist` guards — suggested destinations, binding decisions are yours):
   - `ArtSource/Audio/Kenney_*/Wav/*.wav` → `/Game/Audio/Kenney/<Pack>/` — import
     BEFORE any audio binding steps. **602 WAVs will lengthen the first cook —
     expected, not an error.** Do NOT import the `Ogg/` originals (provenance only).
   - `ArtSource/Models/Kenney_NatureKit|SpaceKit|SurvivalKit|CityKitIndustrial|
     ModularSpaceKit|ModularDungeonKit/GLB/*.glb` →
     `/Game/Environment/Kenney/<Pack>/` (dressing/dungeon-tile candidates).
   - `ArtSource/Models/Kenney_BlasterKit/GLB/*.glb` → `/Game/Weapons/Meshes/Kenney/`
     (CANDIDATE_REPLACEMENT pool — see step 5 before any rebind).
   - `ArtSource/Models/Kenney_AnimatedCharactersSurvivors/FBX/*.fbx` →
     `/Game/Characters/Kenney/Survivors/` — import `characterMedium.fbx` as a
     **Skeletal Mesh**, then the idle/run/jump FBX as animations; **retarget
     compatibility to SK_Survivor_Exosuit is an explicit check** (the pack ships its
     own rig — mismatch is a valid finding, not a defect of the source).
   - `ArtSource/Textures/Kenney_ParticlePack/PNG/*.png` → `/Game/FX/Kenney/ParticlePack/`
     — then **fitness check**: load 2–3 sprites into a test Niagara flipbook/sprite
     emitter BEFORE any combat-VFX binding. Kenney ships palette PNGs (indexed color
     with tRNS alpha); if the engine imports them without usable alpha, record it and
     stop that binding (do not batch-convert — report back).
   - `ArtSource/Textures/Kenney_UIPackSciFi/PNG/**` → `/Game/UI/Kenney/SciFi/`
     (subfolders carry the asset identity: color family × Default/Double state) and
     `Fonts/*.ttf` → `/Game/UI/Fonts/` (offline font import for UMG).
   - `ArtSource/Textures/Kenney_CrosshairPack/PNG/**` → `/Game/UI/Kenney/Crosshairs/`
     (pick a handful for HudWidget hip/aim states — 1,600 are a library, not a
     to-do list).
   - `ArtSource/Textures/Kenney_Skyboxes/PNG/*.png` →
     `/Game/Environment/Kenney/Skyboxes/` (equirectangular — import as long-lat,
     cube-map conversion optional).
3. **Tone check (feeds the GLM wayfinder map, ticket "Kenney Tone Usage Policy")**:
   build ONE cheap PIE scene mixing Kenney Nature/Space props with the existing
   procedural assets in a representative zone; screenshot at gameplay camera
   distance, next to the same scene procedural-only. The keep/constrain/reject
   verdict for the low-poly style against the bioluminescent frontier is a HUMAN
   decision — send both screenshots back with observations; do not decide unilaterally.
4. **CANDIDATE_REPLACEMENT comparison (feeds map ticket "Weapon Replacement
   Decision")**: side-by-side in PIE, procedural weapon mesh vs Kenney blaster per
   weapon archetype; criteria = readability at combat distance, silhouette clarity,
   energy-weapon material tone, consistency with the tone verdict from step 3.
   **Rebind ONLY if the comparison clearly favors Kenney — never auto-replace; the
   procedural sources stay on disk regardless.**
5. **Import-order + size expectations**: audio → models → textures is a safe order;
   ~3,400 new import-ready files total (602 WAV + 657 GLB + 4 FBX + 2,396 PNG +
   2 TTF). First-cook time increase is expected; cook failures on specific files
   are engine-side findings (record the file + error, skip it, continue).
6. **Reporting back**: observations that are DECISIONS (tone verdict, weapon
   keep/rebind, particle fitness) route back to the GLM wayfinder map as ticket
   answers (see `Docs/ASSET_ACQUISITION_REPORT.md` §9 for the import-side summary);
   mechanical defects (a file that will not import, a corrupted asset) are
   engine-side fixes per §20 rule 10 — smallest fix, branch, never redesign.
   Kenney mirror packs exist on OpenGameArt — if sourcing extra copies, do NOT
   re-import over these (different bytes, same content: SHA256 will differ).


## GDP Playtest Additions (v3.4)

During the PIE golden path, additionally verify:
1. Press **T** near hostiles with a captured party — every party Echo should visibly cast (projectile/heal/shield per its loadout; HUD line shows readiness counts).
2. Press **Y** — the smart-cast should fire (or log "nothing ready" early on; after Might 3 etc. it lights up). Verify Power Strike doubles the next swing's damage number.
3. Capture a flying species (Avian family) — it should path through the air after capture (follow command), not walk.
4. Talk to a vendor twice on two different in-world days — affinity tiers should climb and the purchase price should drop at tier 1+ (up to -15%).
5. Save + load — attribute levels and NPC affinity must survive the round-trip (tests 81/83 pin the logic; PIE confirms serialization).
6. Automation now expects **109/109** (was 72 → 84 at GDP → 99 at SCP → 102 at FCR → 103 at DP-3 → 104 at DP-4 → 105 at DP-5 → 106 at DP-6 → 107 at DP-7 → 108 at DP-8 → 109 at DP-9; the validator gate enforces the exact value — always read the count from the repo, never from memory).
7. DP-4 skill loadout (ESC pause menu — SKILL LOADOUT section): cycle a slot onto an unlocked
   skill, close the menu, press **Y** — only the bound skills may fire (an unbound unlocked skill
   like Second Wind must stay silent even when hurt); clear every slot and the Y key returns to
   the legacy all-unlocked smart-cast. Save + load — the loadout survives (v5 additive field on
   the attribute payload), and the HUD skill line shows the bound count (x/3 bound).
8. DP-5 combat depth: fight a Large/Huge wild creature and watch for the periodic glow pulse
   (~20s period, 4s window) — hits during it deal x1.5; hit any creature with its weakness
   element and verify the "WEAKNESS HIT" toast + impact cue; the four bosses must fight with
   DIFFERENT specials (Vault = paired bolts/wide blast, Tyrant = triple-bolt shard volleys,
   Sovereign = triple blasts + triple hazard waves — see `ASTRAWILD.DP5.BossSpecialSets`).
9. DP-6 base depth: travel to the Tidebreaker Isles / Verdant Reach / Stormcrest Highlands /
   Hollow Approach and confirm the four new work sites spawn (Cargo Dock, Field Lab, Dynamo
   Hall, Bulwark Post — definition-placed at zone center + offset); feed the depot its inputs
   (kitchen meat + berries, E to deposit) with a Transport-affinity Echo assigned and collect
   Field Rations; use one (inventory screen) and sprint — stamina should visibly out-last the
   drain for ~90s; use a Pulse Tonic and throw a Resonator — capture chance should read
   +25% while the 30s focus window runs (see `ASTRAWILD.DP6.BaseDepth`).
10. DP-7 world depth: walk from the Dawn Fields into the Frostveil Expanse under clear skies
   and watch the HUD temperature drop (same weather, colder zone — the −12°C hazard layer;
   cold/heat insulation now matters per-region); stand in the Hollow Approach and sprint a few
   laps — stamina should visibly regenerate slower (ash lung, −6/s, never below zero); wait for
   one of the 7 zone events (Mist Tide / Cinder Fall / Dune-Buried Cache / Reef Bloom / Wreck
   Surge / Storm Front / Pearlsong — "WORLD EVENT" toast + event name on the HUD line); craft
   the ancient signal tracker and sweep the four high-threat zones for the new scanner-gated
   secrets (Undergate Vault, Machine Coffin, Hold Room, Tidecache — "Discovered:" toast with
   lore + real loot/research; see `ASTRAWILD.DP7.WorldDepth`).
11. DP-8 NPC depth: on day 1 talk to Trader Tam / Elder Rowan / Guard Sela / Farmer Jori and
   note their new knowledge replies (goods sources, far lands, hazard map, weather almanac);
   then talk AND trade on several in-world days until affinity crosses 25 (Acquaintance) —
   Sela's "Share your patrol knowledge" must appear (dungeon-gate map + one-time research);
   keep going to 50 (Friend) — Tam's supply-line reply + Nima's rare-goods reply appear and
   their follow-ups bridge into the shop screen; at 75 (Confidant) Rowan's "The old doors"
   deep-lore reply appears (dungeon gates + scanner vaults + one-time research). Re-entering
   the same conversation after a tier-up must show the new replies WITHOUT a relog (the gate
   reads the live affinity; see `ASTRAWILD.DP8.AffinityDialogue`).
12. DP-9 dungeon depth: walk the three dungeons and confirm they read DIFFERENTLY in-room —
   the Underlight's dark tight halls + ember light + cliff-shard dressing (stamina visibly
   regenerates slower inside uncleared rooms — the room ash lung, gone the moment the room
   clears), the Sunken Vault's wide flooded halls + glow-reeds (movement visibly slowed while
   wading an uncleared room, removed on clear), the Eye's tall monolith shells + pulsing
   storm light + periodic energy-tile discharges in uncleared rooms. In each dungeon's
   PUZZLE room: attune the three resonance pillars I → II → III in order (wrong order resets
   all three; run out the ~45s window and it also resets) AND defeat the light guard — the
   gate only unseals after both. Clearing a room must visibly shed its hazard (regen/speed
   return immediately; see `ASTRAWILD.DP9.DungeonIdentity`).

## 20c. TIER-A CREATURE MESH IMPORT + BINDING PATCH (Creature Visual Strategy DP-1)

Source-side state (this branch): 8 new bespoke echo meshes live in
`ArtSource/Meshes/Echoes/` (generated by `Tools/ArtSourceGen/gen_echo_*.py`,
manifest-recorded, IMPORT_READY): **Lumewisp, Sprigling, Gloomfang, Auroraling**
(story species) + **Dawnfang, GlassTyrant, EyeSentinel, DrownedSovereign** (bosses).
`AAstrawildEchoBossCharacter` already carries the opt-in skeletal path (definition-
driven: it activates the skinned body and hides the cone the moment the mesh
resolves — until import the cone stays, which is the contract, not a defect).

Sequence (engine machine, after the §20b baseline pass):

1. **Import** the 8 GLBs exactly like the 6 heroes: same flat folder, same
   Interchange pipeline, destination `/Game/Characters/Echoes/` — skeletal mesh +
   the 3 embedded clips (`AM_<Name>_Idle/Move/Hit`).
2. **Commit the imported `.uasset`s** (LFS) BEFORE step 3 — the binding rows only
   pass `validate_final_run.py` check 8 once the referenced assets exist in
   `Content/` (assets-first, binding-second; this is how the 6 heroes landed).
3. **Apply the binding-row patch** — append these 8 rows to the
   `GetEchoArt()` table in `Source/AstrawildCore/Private/AstrawildArtPack.cpp`
   (verbatim; then run both validators on the engine machine and commit):

```cpp
{ TEXT("Echo_Lumewisp"), TEXT("/Game/Characters/Echoes/SK_Echo_Lumewisp"), TEXT("/Game/Characters/Echoes/AM_Lumewisp_Idle"), TEXT("/Game/Characters/Echoes/AM_Lumewisp_Move") },
{ TEXT("Echo_Sprigling"), TEXT("/Game/Characters/Echoes/SK_Echo_Sprigling"), TEXT("/Game/Characters/Echoes/AM_Sprigling_Idle"), TEXT("/Game/Characters/Echoes/AM_Sprigling_Move") },
{ TEXT("Echo_Gloomfang"), TEXT("/Game/Characters/Echoes/SK_Echo_Gloomfang"), TEXT("/Game/Characters/Echoes/AM_Gloomfang_Idle"), TEXT("/Game/Characters/Echoes/AM_Gloomfang_Move") },
{ TEXT("Echo_Auroraling"), TEXT("/Game/Characters/Echoes/SK_Echo_Auroraling"), TEXT("/Game/Characters/Echoes/AM_Auroraling_Idle"), TEXT("/Game/Characters/Echoes/AM_Auroraling_Move") },
{ TEXT("Echo_Dawnfang"), TEXT("/Game/Characters/Echoes/SK_Echo_Dawnfang"), TEXT("/Game/Characters/Echoes/AM_Dawnfang_Idle"), TEXT("/Game/Characters/Echoes/AM_Dawnfang_Move") },
{ TEXT("Echo_GlassTyrant"), TEXT("/Game/Characters/Echoes/SK_Echo_GlassTyrant"), TEXT("/Game/Characters/Echoes/AM_GlassTyrant_Idle"), TEXT("/Game/Characters/Echoes/AM_GlassTyrant_Move") },
{ TEXT("Echo_EyeSentinel"), TEXT("/Game/Characters/Echoes/SK_Echo_EyeSentinel"), TEXT("/Game/Characters/Echoes/AM_EyeSentinel_Idle"), TEXT("/Game/Characters/Echoes/AM_EyeSentinel_Move") },
{ TEXT("Echo_DrownedSovereign"), TEXT("/Game/Characters/Echoes/SK_Echo_DrownedSovereign"), TEXT("/Game/Characters/Echoes/AM_DrownedSovereign_Idle"), TEXT("/Game/Characters/Echoes/AM_DrownedSovereign_Move") },
```

4. **PIE spot-check** (all 4 dungeon/world/final bosses + the 4 story species at
   gameplay camera distance): boss body swaps cone→skinned automatically
   (size-class scale: Large 1.4 / Huge 1.9 on top of authored boss proportions —
   the Sovereign is MEANT to be huge); weak-point sphere still glows on schedule
   beside the skinned body; Underlight Warden shows the Gloomfang mesh; Lumewisp
   reads as a lantern at companion distance. Silhouette separability = the
   strategy doc §11 acceptance criteria.
5. **Report back** per the §20b step-6 split: mesh scale/animation findings are
   mechanical defects (smallest fix, branch); anything about the tier design
   itself routes back to the GLM wayfinder map (ticket 07 follow-ups).

## 21. SOURCE-SIDE STOP CONDITIONS (user directive "MAKE IT A REAL GAME" — 12 points)

The source-side run that produced this branch ends only when all 12 hold — they were
the final arbiter before `READY_FOR_FINAL_BUILD` was declared (registry §I DP-10;
status at the DP-10 final gate):

1. Planned depth work complete — DP-1..DP-10 all COMPLETE in registry §I (no orphans).
2. Wayfinder visual gaps addressed — every P0/P1 gap from the gap analysis is either
   acquired (IMPORT_READY), authored (14 Tier-A meshes), or carries a documented
   fallback (integration matrix §4).
3. Content pipeline deterministic — import_all.py idempotency contract (§20a) +
   acquisition tooling idempotency re-proven.
4. Creature visual strategy complete — strategy doc v1.2 issued; Tier-A bespoke set
   complete (14 meshes IMPORT_READY); Tier-B library is the documented P1 residual.
5. World has meaningful content density — 12 zones, ≥1 anchored event + hazard identity
   per zone (DP-7), 17 POIs incl. 4 scanner-gated secrets, 8 work sites (DP-6).
6. NPCs useful — schedules, shops, affinity tiers + affinity-gated dialogue evolution
   and regional knowledge (DP-8).
7. Player progression meaningful — attributes, 7 milestone skills, 3-slot skill
   loadout (DP-4), T0→T5 gear, 17 techs with branch wiring pinned 17/17.
8. Echoes have meaningful abilities/roles — 44+ ability templates + locomotion
   signatures (DP-3), party resonance, water mounts, weak-point windows (DP-5).
9. Final story fully playable — MQ-01..17 + two endings + post-game (source-side,
   engine walk pending AG-4).
10. Content manifest complete — FINAL_CONTENT_MANIFEST (459/459 LFS, 65/65 /Game refs,
    every content family has a CODE_DEFAULT single source of truth).
11. Final source audit passes — both validators ALL PASS at tip; doc-consistency sweep
    executed at DP-10; census values unified across live docs.
12. FINAL_BUILD_HANDOFF executable — this document: one executable path, counts read
    from the repo (never memory), §20b/§20c sub-sequences included, 12 golden-path
    verify items, stop conditions reflected.

All 12 hold at the DP-10 final-gate commit → **READY_FOR_FINAL_BUILD** (source-side).
Engine-side verification (AG-1..6) remains the exclusive conversion gate — nothing in
this document claims it.
