# ASTRAWILD — Evidence-Based Status Audit (2026-11-04)

> **Mandate**: ตรวจสถานะปัจจุบันโดยใช้ Git และไฟล์จริงเท่านั้น — ไม่ใช้รายงานเก่าหรือ
> README เป็นหลักฐานว่าฟีเจอร์ทำงานแล้ว (user directive, this session).
> Every claim below was re-derived at HEAD by running commands, reading source
> files, and counting tracked content. Old reports were used only as *leads*,
> never as evidence.
>
> Audit HEAD: `baa8478` (pre-audit tip; this report's commit adds on top)
> Auditor: Z.ai Code (Lead Game Architect) — session AUDIT-R2.

---

## §1 Repo / branch / HEAD (proven)

| Item | Value | Evidence command |
|---|---|---|
| Working repo | `/home/z/my-project/astrawild-game` | local clone; `git status` clean after ff |
| Active branch | `final-completion` | repo convention (LIVE_STATE §1) |
| Local HEAD before this audit | was stale at `4c706fc` (Oct-20 clone) | `git reflog` — later sessions worked in other clones |
| Sync action | `git merge --ff-only origin/final-completion` → `baa8478` | zero force, zero overwrite |
| v1 GitHub (banksaisuoy/astrawild-game) | `main` = `final-completion` = `baa8478` | `git ls-remote origin` |
| v2 GitHub (banksaisuoy/astrawild-game-v2) | default `develop` = `3be3706` | `git ls-remote` via token |
| v2 unique work | **ZERO** — `3be3706` is an ancestor of `baa8478` | `git merge-base --is-ancestor 3be3706 baa8478` → exit 0 |
| v2 "new home" declaration | historical (2026-10-09 token-crisis era), preserved verbatim in `MEMORY.md` as HISTORICAL; canonical line = v1 `final-completion` (MERGE-1/MERGE-2) | `git log --oneline c84d889 -1`, MEMORY.md |
| v2 mirror drift | 4 commits behind (the AUDIT-R1 series `81cdb46`→`baa8478`) | `git rev-list 3be3706..baa8478 --count` = 4 |
| Engine version | UE 5.8 (`EngineAssociation "5.8"`, canon 5.8.2 Win) — NOT changed | `ASTRAWILD.uproject:3` |

No branch was deleted, no history rewritten, no force push performed in this audit.

## §2 final-completion work present at HEAD (commit + file evidence)

| Feature | Origin commit | Real file(s) at HEAD | Runtime wiring (verified) |
|---|---|---|---|
| Title menu | `af11ab4` (bookends) | `Public/AstrawildTitleMenuWidget.h` + Private .cpp | `PlayerController.cpp:135-137` (`bShowTitleScreenAtBoot && bStandaloneGame` → `ShowTitleScreen()`); NM_Client gate hardened by `81cdb46` (M-1) |
| Death overlay | `af11ab4` | `Public/AstrawildDeathOverlayWidget.h` + .cpp | shown on death `PlayerCharacter.cpp:2147`, hidden on respawn `:2228` |
| Victory screen | `af11ab4` | `Public/AstrawildVictoryScreenWidget.h` + .cpp | `PlayerController.cpp:1672` `ShowVictoryScreen()`; in `IsAnyScreenOpen()` `:1442` |
| Visual overhaul (VO-1R) | `1e48cd0`, merged via MERGE-2A `b0e7238` | `AstrawildVisualOverhaul.h/.cpp` (15,508 B / 8,520 B) | director spawned `WorldBootstrapper.cpp:480-483`; toggle `:257`; perf fixes (mote throttle, dedicated-server skip) in `81cdb46` (M-6/L-7/L-8) |
| EchoDex (EDX-1) | merged via MERGE-2B `4112c56` | `AstrawildEchoDexSubsystem.h/.cpp`, `AstrawildEchoDexScreenWidget.*` | bound to I key; 15th screen in `IsAnyScreenOpen()` |
| Canonical art paths (GS-1) | merged via MERGE-2C `82dbc41` | `CanonicalAssetPath()` in `AstrawildArtPack.h` | applied at every art bind site (ProductionContent/PlayerCharacter/ContentLibrary/ResourceNode/Projectile) |
| Night Raid gameplay fix | `b375d01` (FIX-1) | `WorldEventSubsystem.h:107` `CollectNightRaidPool` | consumed by `ResolveEventEffects`; regression contract in tests |
| Audit-R1 robustness sweep | `81cdb46` | 8 Source files (see `git show 81cdb46 --stat`) | title gate, HUD throttle, mote throttle, input parity 34, autosave null-guard, boot sun seed |
| v2-repo history preservation | MERGE-1 `c84d889` (parents `9a285f7`+`4a8b7bb`) | `MEMORY.md` BKE-1 HISTORICAL record | true merge, zero force-push |

## §3 Asset coverage (game ID → mesh → import → binding → animation → engine-tested)

Ground truth sources: `ArtSource/manifest.json` (189 entries, 100% `status: present`),
`git ls-files Content/` (414 .uasset + 2 .umap), LFS pointer scan (**588 pointers counted
byte-by-byte this session**: 414 uasset / 105 glb / 46 png / 16 wav / 4 fbx / 2 umap / 1 jpg),
code tables in `AstrawildArtPack.cpp` + `AstrawildProductionContent.cpp` + `AstrawildEchoMutator.cpp`.

Imported coverage by system (**109 / 189 manifest ue_paths resolve to tracked Content packages**):

| Game system (census count) | Source mesh | Imported asset | Runtime binding | Animation | Status | Engine-tested at tip? |
|---|---|---|---|---|---|---|
| Player survivor (1) | Kenney AnimatedCharactersSurvivors FBX | `SK_Survivor_Exosuit` + 6 materials + phys+skel (**16 files**) | `GetSurvivorArt()` paths → applied by ProductionContent | 7 clips (Idle/Walk/Run/Jump/Aim/Fire/Gather) — all imported | **complete** | **NO** (V2-30 NOT_RUN) |
| Weapons (8) | 5 unique CC0 meshes | 5 × `SM_Weapon_*` + 25 materials | `GetWeaponArt()` — 8 census IDs bound (3 share meshes by design: PlasmaCharger+LumenBeam→PlasmaCarbine, Starlance→Singularity) | n/a (mesh+socket) | **complete** | **NO** (V2-34 FX authoring pending) |
| Hero creatures Tier-A (15 species) | 6 × `SK_Echo_*.glb` + 9 × `SK_Boss_*.glb` (spare-pool bind, ArtPack.cpp:124-143) | **6 only**: Bastionbeetle, Cindermule, Deepdelver, Mistmender, Terraquill, Voltpylon (60 files incl. 18 AM) | `GetEchoArt()` explicit rows | Idle/Move/Hit per imported creature | 6/15 skinned; 9 → SK_Boss paths **not imported** → PMC fallback | **NO** (V2-31) |
| Tier-B creatures (36 species incl. 3 bosses) | 36 × `SK_Echo_*.glb` all present | **0 imported** | `BuildTierBMechPath()` convention | convention `AM_<Name>_Idle/Move` (not imported) | all PMC until AwPipeline import (V2-29/36) | **NO** |
| Bestiary base archetypes (178 species via 204-row mutation table) | 16 × `SK_Base_*.glb` all present (8 themes × 2) | **0 imported** (`Content/Characters/Echoes/BaseMeshes/` absent) | `BuildSciFantasyBaseMeshPath()` per theme+bodyplan | baked `AM_SK_Base_*` convention (not imported) | all PMC until `import_echo_bases.py` (V2-35) | **NO** |
| Boss spare pool | 14 × `SK_Boss_*.glb` present; 9 bound to Tier-A species, 5 intentionally unbound | **0 imported** (`Content/Characters/Echoes/Bosses/` absent) | Printf convention ArtPack.cpp:139-141 | ditto | PMC fallback | **NO** |
| Environment core (biomes, all 12 zones) | Kenney/Quaternius CC0 | trees 3 + rocks 4 + foliage 4 + cliff 1 + ruins 3 + resource nodes 4, each with materials (**56 files**) | `GetBiomeArt()` per-zone table (trees/rocks/grass + landscape mat + ambience) + `GetNodeArt()` | n/a | **complete** | **NO** (V2-32/33; landscape material = editor manual step HANDOFF §19) |
| VO-1R dressing extras | 9 × `SM_*` (flowers/mushroom/log/stump/ruins) present | **0 imported** | `BiomeDressingActor` scatter budgets | n/a | placeholder/shared fallback | **NO** |
| Vehicle (skiff) | `SM_Vehicle_DawnSkiff` source | **imported** + 4 materials | `Paths::VehicleDawnSkiff` (the only vehicle code path) | n/a | **complete** | **NO** |
| Audio | 36 wav sources | **36 imported** (`Content/Audio`: 5 ambience / 7 creature cries / 5 footsteps / 4 player / 7 UI / 8 weapon) | `AudioDirectorSubsystem` zone/weather/cue maps (CGC-3), fail-closed | n/a | **complete** | **NO** |
| Landmarks (resonance pillars, POI beacons) | ruin pillar (shared CC0) | **imported** (`Environment/Ruins/SM_Ruin_Pillar`) | `GetLandmarkArt()` → GS-1 canonical object-path | n/a | **complete** | **NO** |
| Maps | n/a | **1 game map** `Content/ASTRAWILD/Maps/MainMap.umap` (+1 template leftover `Lvl_ThirdPerson.umap`) | GameMode default map | n/a | present | **NO** |

**Species visual summary (229 census): 6 skinned in-engine today (2.6%); 51 direct-mesh
bound (sources 100% present, import pending engine-side); 178 on 16 shared SK_Base
archetypes (sources present, import pending); every unresolved path fails CLOSED to
the procedural mutated PMC body — the game renders and plays without them by design.**

## §4 Map / zone coverage (real count: **12 zones**, from `EAstrawildZone` enum + descriptor table)

Verified: `AstrawildTypes.h:215-231` (12 zones + None), `ZoneSubsystem.cpp:49-136`
(4×3 grid descriptors with bounds/hazards), one `TerrainTileActor` per zone
(`WorldBootstrapper.cpp:498-509`), runtime invoker navmesh (`DefaultEngine.ini:32`
`bGenerateNavigationOnlyAroundNavigationInvokers=True` + `NavSys->Build()` at
`WorldBootstrapper.cpp:325`), water planes for 3 sea zones (`SpawnWaterPlanes`
`:1120-1146`), camp PlayerStart fallback `:511`, dungeon portals `:889-930`
(Underlight + Sunken Vault pairs), PMC terrain collision `QueryAndPhysics`+BlockAll
(`TerrainTileActor.cpp:47-48`).

| Zone | Generator | Spawns | Collision | Navigation | Quest access | Engine-tested at tip? |
|---|---|---|---|---|---|---|
| DawnFields (starter, hazard 0) | TerrainTile 128 quads/side | camp ring nodes, starter Echoes, PlayerStart fallback | PMC terrain | invoker navmesh | MQ-1 auto-start + chain | **NO** |
| DuskMarsh / EmberRidge / FrostveilExpanse / Glimmerwood / HollowApproach (5 core) | same | zone wildlife table + WildlifeDirector (per-zone cap 24, CGC-1) | PMC | invoker | MQ chain + side quests | **NO** |
| AzureShallows / TidebreakerIsles / SunscarDesert / StormcrestHighlands / VerdantReach / PearlseaReef (6 Grand Expanse, Batch 8) | same + water planes (3 sea zones) | same + weather-gated rares | PMC | invoker | MQ chain touches all 12 (verified: quest objectives reference every zone) | **NO** |

Past engine evidence (NOT current-tip): `8313c61` (2026-09-02) recorded a real
packaged-exe playable log (`Docs/ENGINE_LOGS/PACKAGED_PLAYABLE_05E5204.log`) proving
the **6-zone-era** world: 6 terrain tiles built, navmesh build requested, zone
discovery fired. That log predates the 12-zone Batch 8, the CGC pack, MERGE-1/2 and
VO-1R by ~10 weeks — it is *historical evidence the approach works*, **not** evidence
the current tip works.

## §5 Toolchain reality (this sandbox)

`which UnrealEditor/unreal/ue5` → none; `/opt`, `/usr/local` → no Epic installs;
`which dotnet mono clang cmake` → **none**; `git-lfs` → **not installed**
(`validate_repository.sh` correctly fails LOUD on this — AUDIT-R1 H-2 fix working as
designed). **Conclusion: no Unreal, no compile, no PIE possible here.** Per mandate
§6: ran every check that CAN run, recorded limitations, changed nothing engine-side,
downloaded nothing.

## §6 Checks actually run this session (all at `baa8478`)

| Check | Result |
|---|---|
| `python3 Scripts/validate_final_run.py` | **ALL CHECKS PASSED** — 147-test gate + census 15/15 (229 species / 78 items / 58 recipes / 26 buildings / 17 techs / 22 quests / 13 NPCs / 12 zones / 11 loot / 8 weapons / 10 nodes / 8 work sites / 16 events / 17 POIs / 204 mutation rows). Validator itself prints: "static level — engine build/test still required on the target machine" |
| `python3 Scripts/validate_design_data.py` | **78/78 ALL PASS** |
| `python3 Scripts/lint_unreal_cpp.py` | **0 errors, 0 warnings** across 217 files (UCLASS=131, UENUM=51, USTRUCT=45) |
| `bash Scripts/validate_repository.sh` | **BLOCKED — environment only**: `git-lfs` binary absent in this sandbox (rule: loud fail, H-2). No repo defect |
| Automation test count (grep) | **147** `IMPLEMENT_SIMPLE_AUTOMATION_TEST` in `AstrawildAutomationTests.cpp` |
| LFS pointer count (byte scan of every tracked file) | **588** (414 uasset / 105 glb / 46 png / 16 wav / 4 fbx / 2 umap / 1 jpg; 416 Content + 170 ArtSource + 2 Docs) |
| Manifest integrity | **189/189 `status: present`**; 109/189 ue_paths resolve to imported Content packages; 80 unresolved are ALL `category: mesh` (16 SK_Base + 14 SK_Boss + 36 SK_Echo Tier-B + 2 SK_Survivor_T1/T2 + 9 dressing SM_* + 3 unused vehicles) — the designed AwPipeline engine-side import steps (V2-29/35/36) |
| Working tree | clean; only this report + the 2 doc fixes are new |

**These are static checks. None of them equals engine-tested.** No mock result in this
report is presented as runtime proof.

## §7 Defects found & fixed this round (AUDIT-R2)

Stale current-facing numbers in the two Windows-machine handoff docs — all would have
tripped or misinformed the real-machine run:

1. `ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md`: test count said **140/139/146** at four
   current-facing spots (incl. the Hour-3 **PASS criterion `count == 140`** and the
   troubleshooting row "Test count != 140" — a 147-success run would read as DRIFT) → all
   set to **147** (grep-verified at HEAD).
2. Same file: LFS pointer count **586** at four spots, incl. "P4.3 Verify LFS (expect
   exactly 586)" — the machine verify step would false-fail at the true 588 → set to
   **588** (byte-scan-verified this session).
3. `ASTRAWILD_ON_PC_TASKS.md`: "(586 objects …)" → **588**.
4. No other current-facing surface carries 140/139/146/586 as a *current* claim
   (MASTER_CONTROL W-21 already said 588 — correct).

Fixed in this commit; committed on `final-completion` (safe continuation branch, per
operating rules: no force push, no content deletion, engine version untouched).

## §8 Not yet proven (honest ledger)

1. **Current tip `baa8478` has never been compiled anywhere** — no UE exists on any
   machine that touched this tip.
2. **No PIE evidence at all for**: title→New Game flow, movement, interactions,
   MQ-1..17, capture/breeding, save→quit→load→continue, death/respawn overlay,
   victory screen, EchoDex, visual-overhaul visuals, LAN co-op. ENGINE queue rows
   V2-29..V2-36 + golden-path rows remain **NOT_RUN** (queue doc says so honestly).
3. **80/189 source meshes not yet imported** as UE packages (all import steps are
   editor-side scripts: `AwPipeline/import_all.py`, `import_echo_bases.py`,
   `Setup_And_Play.bat`).
4. **`MainMap.umap` interior unverifiable here** (LFS object not local, git-lfs absent)
   — its content (e.g. whether the landscape material is pre-assigned) is only
   verifiable on the Windows machine; HANDOFF §19 expects a manual assignment step.
5. `validate_repository.sh` unrunnable in this sandbox (git-lfs binary) — must pass on
   the Windows machine during P4.

## §9 Blocker + fastest path to playable

**The single blocker is unchanged: ENGINE-RUN-1 on the user's Windows UE 5.8.2 machine.**
Everything source-side is complete, statically green (147-gate + design 78/78 + lint 0/0
+ census 15/15), and the handoff docs are now count-accurate. Entry:
`Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md` (blank machine, P0→P13) or
`Docs/ASTRAWILD_ON_PC_TASKS.md` (prepared machine). Golden path the machine run must
prove: New Game → walk → interact → MQ-1 → save → quit → load → continue → package
Development build (`Package_Game.bat`), with V2-29..36 import passes and clips.

---
*End of AUDIT-R2. All numbers in this report were derived at HEAD by the auditor; the
authoritative gates remain `Scripts/validate_*.py|sh` (rule: read counts from the repo,
never from memory).*
