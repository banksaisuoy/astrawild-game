# ASTRAWILD — FRESH MACHINE AI PLAYBOOK (เครื่องเปล่า → เล่นได้ครบ)

> **FOR THE AI AGENT THAT JUST LANDED ON A BRAND-NEW WINDOWS MACHINE.**
> สำหรับ AI agent บนเครื่อง Windows เปล่าๆ ที่ต้องการพา ASTRAWILD ไปสู่สถานะ
> "เล่นได้ครบทุกระบบตามที่ออกแบบไว้" — อ่านไฟล์นี้ตั้งแต่ต้นจนจบก่อนทำอะไร
> This file is the SPINE of the fresh-machine run: it walks from an empty OS
> (no Git, no Python, no Visual Studio, no Unreal Engine) all the way to a
> fully playable, tested, packaged, LAN-ready game with evidence.
>
> **Status honesty (binding)**: at the moment this playbook was authored,
> everything engine-side is **ENGINE-UNVERIFIED** — the source repository is
> complete and statically verified, but the engine run itself has never
> happened on the current tip. This playbook is the procedure that converts
> ENGINE-UNVERIFIED into ENGINE_VERIFIED with raw evidence. It never assumes
> success; every phase below is a check, not a formality.
>
> Task lineage: FMP-1 (v9.6 Fresh-Machine Playbook pack, 2026-09-06).
> Author: GLM 5.3 (source-side). You (the reading agent) act in the
> **Antigravity role**: exclusive runtime authority. See
> `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` §6 for the locked role table.

---

## 0. OPERATING RULES (read first — violation of any of these poisons the run)

1. **Evidence or it did not happen.** Every claim of PASS must be backed by a
   raw artifact (log file, report JSON, screenshot, clip) captured on THIS
   machine at THIS commit. Commit the artifacts into `Docs/ENGINE_LOGS/raw/`
   with the SHA in the filename.
2. **Never fake a green.** If something fails, record FAIL + the exact error
   output, then fix-forward (rule 3). A false "100% PASS" is the single worst
   defect class this project has fought since day one.
3. **Fix-forward, smallest change.** A compile error gets a minimal mechanical
   fix on a branch; anything architectural goes BACK to GLM with the logs —
   do NOT redesign systems. The queue's §1 note ("prefer the minimal
   mechanical fix, e.g. drop a UPROPERTY specifier, over restructuring") is
   binding.
4. **Read counts from the repo, never from memory.** The static validator
   (`Scripts/validate_final_run.py`) pins the exact expected numbers (tests,
   census). When this playbook says "133 tests", the validator's gate is the
   authority — if they disagree, the repo wins and this playbook is stale.
5. **Git discipline.** Work on `final-completion` (or an
   `agent/<your-task>` branch off it). Push every completed phase with the
   task ID + SHA + state in the commit message. **Never force-push, never
   rewrite history, never delete unrelated work.**
6. **The make-ready loop.** For every prerequisite gap:
   `run preflight → see FAIL → install/fix per the referenced section →
   re-run preflight → repeat until every REQUIRED row is PASS`.
   The preflight script is `Scripts/fresh_machine_preflight.ps1`.
7. **Known non-corruption warning (do not "repair" it).** `git lfs fsck`
   exits 1 with ~3,954 "should have been a pointer" flags in
   `ArtSource/Textures/Kenney*`, `Models/Kenney`, `Audio/Kenney`,
   `Models/Quaternius` — this is a documented pre-existing convention
   mismatch (raw pack sources committed as plain git objects while
   `.gitattributes` declares those types LFS). Every current pointer's
   object resolves OID-matched. It is NOT corruption. Do NOT run history
   rewrites, `git lfs migrate`, or dedup "fixes" for it.
8. **Stop conditions.** If you hit something you cannot mechanically fix
   (engine crash loop, UBT failure after one retry + log capture, material
   import failure), STOP that phase, capture the full log, commit it, and
   write the failure into `Docs/BUILD_STATUS.md` + the queue row. That is a
   successful run of the verification process — a documented FAIL is worth
   more than an invented PASS.
9. **The engine log census line is the authority** for world content. When
   PIE boots, compare the `Content library registered (live census)` log
   line against Appendix A. The log wins over every doc, including this one.

**Read order for you (the agent):** this file →
`Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` (queue/state) →
`Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` (the PASS/FAIL rows) →
`Docs/ASTRAWILD_FINAL_BUILD_HANDOFF.md` §20/§20b-e (the canonical engine
sequence — this playbook is its fresh-machine spine) →
`Docs/ASTRAWILD_PLAYER_RULES.md` (controls, when you reach the play phases).

---

## 1. WHAT YOU ARE BUILDING (the game at a glance)

ASTRAWILD — Echoes of the First Dawn: a third-person co-operative survival
adventure for Unreal Engine 5.8.2 (C++ AstrawildCore module, data-driven
content, zero-Blueprint gameplay). Product canon (v9.5):

| Thing | Count |
|---|---|
| Zones (procedural world) | 12 |
| Echo species (bestiary rows 204 + 25 authored) | 229 |
| Quests (17 main MQ-01..17 + 5 post-game) | 22 |
| Technologies / recipes | 17 / 58 |
| Building pieces | 26 |
| NPCs (dialogue trees) | 13 |
| Dungeons / bosses | 3 / 4 |
| Endings + post-game + New Game Plus | A/B + yes + yes |
| LAN co-op | private 4-player listen-server |
| Automation contracts (your Test.ps1 target) | 133 |
| ArtSource real-mesh catalog | 109 assets, manifest 189/189 present |
| Git LFS objects | 586 pointers |
| Content (.uasset/.umap) packages | 416 |
| Engine | UE 5.8.2 (EngineAssociation "5.8"), VS2022 MSVC v143 |

Repository: `https://github.com/banksaisuoy/astrawild-game` (**private** —
you need GitHub credentials with read access + a PAT with `repo` scope for
the LFS endpoints). Integration branch: **`final-completion`**. At playbook
authoring the tip was `1699d58` (v9.5 DCP-FINAL) plus this v9.6 docs commit —
the live tip is whatever `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` §1 says.

---

## 2. PHASE P0 — MACHINE PRE-FLIGHT (hardware / OS / disk)

**Goal:** prove the machine can carry UE + VS + repo + build outputs.

Run from any PowerShell (elevated is fine):

```powershell
Get-CimInstance Win32_OperatingSystem | Select-Object Caption, Version, OSArchitecture
Get-CimInstance Win32_ComputerSystem  | Select-Object @{n='RAM_GB';e={[math]::Round($_.TotalPhysicalMemory/1GB,1)}}
Get-CimInstance Win32_VideoController | Select-Object Name, DriverVersion
Get-PSDrive -PSProvider FileSystem | Select-Object Name,
    @{n='FreeGB';e={[math]::Round($_.Free/1GB,1)}},
    @{n='UsedGB';e={[math]::Round($_.Used/1GB,1)}}
```

**Expected / minimums:**

| Check | Minimum | Recommended | On fail |
|---|---|---|---|
| OS | Windows 10 1909+ / Windows 11, x64 | Windows 11 | P0.1: cannot fix in software — report and stop |
| RAM | 16 GB | 32 GB | P0.2: close background apps; if < 16 GB the editor build may thrash — report |
| GPU | any DX12-capable GPU, 4+ GB VRAM | GTX 1660 Ti class (the reference machine) | P0.3: update drivers; RHI fallback `-d3d11` exists for PIE sanity but evidence builds should use default |
| Disk (install drive) | 120 GB free | 250 GB free | P0.4: clean up, or install UE/VS/repo on a drive with headroom |
| Admin rights | needed for installs | — | P0.5: elevate (`Start-Process powershell -Verb RunAs`) |

**Space budget** (why 120 GB): UE 5.8.2 ~60 GB + VS2022 with workload ~35 GB
+ repo working tree + LFS ~2 GB + build intermediates 20–40 GB + packaged
build ~10 GB + DerivedDataCache (shared, grows) ~10–20 GB.

---

## 3. PHASE P1 — INSTALL BASE TOOLS (Git + LFS, Python)

**P1.1 Git for Windows (includes Git LFS + Git Bash).**

```powershell
winget install --id Git.Git -e --source winget
# manual fallback: https://git-scm.com/download/win (64-bit installer,
# default options: "Git from the command line and also from 3rd-party
# software" PATH mode, keep LFS checked)
```

Verify (NEW shell after install):

```powershell
git --version          # expect: git version 2.4x.x.windows.x
git lfs version        # expect: git-lfs/3.x.x
```

If `git lfs version` fails: re-run the Git installer ("Windows Explorer
integration → Git LFS"), or `winget install --id Git.Git.LFS -e`, then
`git lfs install` once.

**P1.2 Python 3 (validators + asset fetch tooling need it).**

```powershell
winget install --id Python.Python.3.12 -e
# manual fallback: https://www.python.org/downloads/ (64-bit,
# tick "Add python.exe to PATH" on the first installer page)
```

Verify:

```powershell
python --version       # expect: Python 3.12.x (3.9+ acceptable)
```

**P1.3 (optional but recommended) PowerShell 7** — not required; every
command in this playbook is Windows PowerShell 5.1-compatible:

```powershell
winget install --id Microsoft.PowerShell -e
```

---

## 4. PHASE P2 — INSTALL VISUAL STUDIO 2022 (C++ game workload)

The project compiles the `AstrawildCore` C++ module through UBT with the
MSVC v143 toolchain. Install VS 2022 Community with the exact workload:

```powershell
winget install --id Microsoft.VisualStudio.2022.Community -e --source winget --override "--quiet --wait --add Microsoft.VisualStudio.Workload.NativeGame --includeRecommended"
```

That workload ("Game development with C++") brings MSVC v143
x64/x86 build tools, Windows 10/11 SDK, and the Unreal Engine
integration bits. Manual fallback: download the VS2022 Community bootstrapper
from https://visualstudio.microsoft.com/downloads/, run it, and in the
Workloads tab tick **"Game development with C++"** with default optional
components (Installation details must show
`MSVC v143 - VS 2022 C++ x64/x86 build tools` and a Windows 10/11 SDK).

**Verify (the exact gate the preflight uses):**

```powershell
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
& $vswhere -products * -requires Microsoft.VisualStudio.Workload.NativeGame -property installationPath
# expect: C:\Program Files\Microsoft Visual Studio\2022\Community
& $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
# expect: same path again (MSVC v143 present)
```

On fail: launch the "Visual Studio Installer" app → Modify → tick the
workload + the component above → Install. If winget itself is missing:
Settings → Apps → Advanced app settings → App installer, or grab it from
https://aka.ms/getwinget.

---

## 5. PHASE P3 — INSTALL UNREAL ENGINE 5.8.2

This is the only prerequisite without a winget path; it rides the Epic
Games Launcher (free account):

1. Install the launcher:
   `https://store.epicgames.com/en-US/download` (default options).
   Sign in, or create a free Epic account (needs email + 2FA — if you are
   an agent driving this for a user, hand the login step to the human).
2. Launcher → **Unreal Engine → Library → the "+ arrow" next to the engine
   version list → install 5.8.x (latest 5.8.2)**. Default install location
   is `C:\Program Files\Epic Games\UE_5.8` — fine.
   Default components are enough (Core + "Starter content" NOT required;
   "Editor symbols for debugging" optional).
   Download is ~30–40 GB; expect 20–90 min depending on bandwidth.
3. When it finishes, verify:

```powershell
Test-Path "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
# expect: True
```

4. **If you install to a non-default root** (like the reference machine's
   `E:\Epic Games\UnrealEngine`), set it once for every future shell:

```powershell
setx UE_ROOT "E:\Epic Games\UnrealEngine"      # persists for future shells
$env:UE_ROOT = "E:\Epic Games\UnrealEngine"    # current shell
```

`ASTRAWILD.uproject` uses EngineAssociation `5.8`, so any 5.8.x registers.
If Windows asks "which editor to open .uproject with" and shows 5.8 → OK.
Do NOT use 5.3–5.7 for evidence runs (Setup_And_Play.bat tolerates them,
but the canon engine is 5.8.2).

---

## 6. PHASE P4 — CLONE THE REPOSITORY + STATIC GATES

**P4.1 Clone (private repo — credentials):** you need a GitHub identity with
read access to `banksaisuoy/astrawild-game` and a classic PAT with `repo`
scope (the LFS endpoint uses the same credential). Git Credential Manager
(ships with Git for Windows) stores it on first use:

```powershell
cd D:\            # or wherever you want the repo (drive with headroom)
git clone https://github.com/banksaisuoy/astrawild-game AstrawildGame
# credentials prompt appears once → username + PAT
cd AstrawildGame
```

**P4.2 Branch + LFS:**

```powershell
git lfs install
git checkout final-completion
git pull origin final-completion
git lfs pull
```

**P4.3 Verify LFS (expect exactly 586):**

```powershell
git lfs ls-files | Measure-Object -Line      # expect: Lines: 586
git lfs fetch --all                          # expect: no missing objects
# spot-check one binary really resolved (not a pointer file):
git show HEAD:Content/Vehicles/SM_Vehicle_DawnSkiff.uasset | Select-Object -First 3
# expect: binary garbage / UE package magic, not "version https://git-lfs..."
```

Remember rule 7 in §0: `git lfs fsck` WILL exit 1 with ~3,954 raw-pack
flags — that is the documented convention mismatch, not corruption. If
`git lfs ls-files` is empty or objects are missing, THAT is real: re-run
`git lfs install` + `git lfs pull origin`.

**P4.4 Run both static validators (they gate everything else):**

```powershell
python Scripts\validate_final_run.py
# expect final line: FINAL RUN VALIDATION: ALL CHECKS PASSED
# (133-test gate, census gates: 229 species / 204 rows / 22 quests /
#  13 NPCs / 189-manifest / 51 direct-mesh rows / Tier-B 36 ...)

& "C:\Program Files\Git\bin\bash.exe" Scripts/validate_repository.sh
# expect final line: ASTRAWILD repository validation passed (v2 ruleset). ...
```

If either validator fails on a fresh clone: capture the full output, commit
nothing, report the exact failing gate — the clone is broken, not the repo
gate (they pass at the authored tip).

**P4.5 Working-tree hygiene:** `git status --porcelain` must be clean
before the engine run. Generated dirs (`Binaries/`, `Intermediate/`,
`Saved/`, `DerivedDataCache/`) are git-ignored — they will appear after
builds; that is expected and must never be committed.

**P4.6 Run the preflight script (the machine-readiness gate):**

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\fresh_machine_preflight.ps1 -RepoRoot (Get-Location).Path
```

Every REQUIRED row must be PASS. Any FAIL prints the playbook section that
fixes it. Loop P1–P4.6 until green. **Do not start P5 with a FAIL row.**

---

## 7. PHASE P5 — GENERATE + BUILD (compile gate: queue V-1..V-3)

**P5.1 Generate IDE files (optional but harmless):**

```powershell
& (Join-Path $env:UE_ROOT 'Engine\Build\BatchFiles\GenerateProjectFiles.bat') -project="<repo>\ASTRAWILD.uproject" -game -engine
```

**P5.2 Build the editor target** — the wrapper scripts honor environment
overrides since v9.6 (`UE_ROOT`, `ASTRAWILD_UPROJECT`, `ASTRAWILD_ARCHIVE`,
`ASTRAWILD_AUTOMATION_OUTPUT`); with no env vars they keep the exact legacy
defaults, so both layouts work:

```powershell
cd <repo>
$env:UE_ROOT = "C:\Program Files\Epic Games\UE_5.8"          # your engine root
.\Build.ps1
```

Equivalent direct form (if you prefer no wrapper):

```powershell
& "$env:UE_ROOT\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe" `
    ASTRAWILDEditor Win64 Development `
    -Project="<repo>\ASTRAWILD.uproject" -WaitMutex -FromMsBuild -NoUBA
```

**PASS = exit 0, 0 errors.** First full build of ~193 C++ files takes
10–40 min. Record the tail of the output (the compiler's last ~50 lines) —
that is your V-2 evidence.

**On failure:**

- **UBT ExitCode 6 / FZ-A1 class** (seen once at a pre-final SHA): capture
  `Engine\Intermediate\Build\BuildHistory` + the UBT log under
  `$env:UE_ROOT\Engine\Programs\UnrealBuildTool\Log.txt` BEFORE any retry.
  One retry is allowed; if it recurs, stop and report with the logs.
- **UHT errors (45-header codegen)**: prefer the minimal mechanical fix
  (queue §1 note); do not restructure.
- **LNK2001 (historical C-2 class)**: usually a missing `.cpp` for a new
  header — check `Scripts/validate_repository.sh` header/impl rule first.
- If the editor binary already exists (`Binaries\Win64\UnrealEditor-ASTRAWILD.exe`),
  P5 is still required once from a clean `Intermediate` state for honest evidence.

---

## 8. PHASE P6 — ASSET IMPORT + SHOWCASE (queue V2-29 + V2-36)

The one-click Windows path (this exact .bat is the product's front door):

```powershell
cd <repo>
.\Setup_And_Play.bat
```

What it does (in order): locates UE (5.8 → 5.3, or `%UE_ROOT%`) →
re-verifies the 109-asset catalog (`Scripts/fetch_free_assets.py`) →
launches `UnrealEditor.exe` with
`-ExecutePythonScript="Content/Python/AwPipeline/run_overhaul.py"` →
`run_overhaul.py` runs `import_all.py` (imports the 109 real CC0 meshes +
textures/audio, renames clips to the runtime AM_ convention, builds PBR
ore-node emissive materials, adds Muzzle/Weapon_R sockets, and counts
meshes + clips as direct-binding coverage) then `build_showcase_map.py`
(builds `/Game/Maps/L_Showcase_ArtOverhaul`: PlayerStart + armor podium +
hero row + 16-base grid + Tier-B grid + boss arena + weapon rack + vehicle
pad + node garden) and leaves the editor open on that map.

**PASS = the report file** `Saved\AwPipelineReport\import_report.json`
shows `"total_missing": 0` AND `"errors": []` — **and the missing count
includes every AM_ clip path** (direct-binding evidence: species → real
mesh + real clips). Then PIE in the showcase level (press Play in the open
editor): the player pawn spawns in the real Tier-3 Exosuit mesh, display
rows play real idle clips, ore nodes glow. Capture the report JSON + one
screenshot. That closes V2-36 and the import half of V2-29.

**Then the Sci-Fantasy base import (V2-35, same window):** in the editor's
Python console (or a second editor launch with
`-run=pythonscript -script="Content/Python/AwPipeline/import_echo_bases.py" -stdout -unattended`):

```
py "Content/Python/AwPipeline/import_echo_bases.py"
```

PASS = `Saved\AwPipelineReport\echo_base_report.json` shows
`"total_missing": 0` AND `"errors": []` (material coverage counts — a
failed M_SciFi_* master is an ERROR, not a warning).

**Manual step you must not skip (V2-32 prerequisite):** open
`Content/ASTRAWILD/Maps/MainMap.umap`, select the Landscape actor, and set
its **Landscape Material** slot to `/Game/Materials/M_Landscape_SciFiFrontier`
(the import pass builds the material but does NOT assign it). If the slot
still points at `M_Master_Surface` or is unset, terrain renders wrong.

**On failure:** a specific file that will not import = record file + error
in `Docs/BUILD_STATUS.md`, skip it, continue (queue §20b rule). A report
with `total_missing > 0` usually means the LFS pull missed sources —
re-check P4.3 before touching the pipeline.

---

## 9. PHASE P7 — PIE BOOT (queue V-4)

Open the editor (or it is already open), then open
`Content/ASTRAWILD/Maps/MainMap.umap` and press **Play**.

**Expected within ~30 s:** terrain tiles + Dawnstead camp (workbench,
campfire, rest point, 2 skiffs) + village; HUD shows `Day 1 08:00`,
weather Clear, `Research: 0 RP`, quest tracker "First Light: Wood 0/10".

**The census log line (rule 9):** open the Output Log and find
`Content library registered (live census)` — it must read:

```
78 items, 58 recipes, 229 Echo species, 26 buildings, 17 technologies,
22 quests (17 MQ + 5 post-game DCP-1), 11 loot tables, 13 NPCs (11 + DCP-4
Vess/Ione), 8 weapon profiles, 10 resource nodes, 8 work sites,
16 world events, 17 POIs, 12 biomes, 13 dialogue trees (11 + DCP-4), 3 robots.
```

Numbers are counted live from the registry — if any number differs, the log
wins; record the delta and treat it as a P1 finding (fix-forward or report).

Capture: screenshot of the camp + the log tail. Also run one full loop of
the 23-stage golden path here or in P10 (the stages are enumerated in
`Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` §2, V-6..V-28).

---

## 10. PHASE P8 — AUTOMATION TESTS (queue V-5)

```powershell
cd <repo>
$env:UE_ROOT = "C:\Program Files\Epic Games\UE_5.8"     # your engine root
.\Test.ps1
# writes Automation_Output.txt (or $env:ASTRAWILD_AUTOMATION_OUTPUT)
```

**PASS = `Result={Success}` count == 133 and `Result={Fail}` count == 0.**
The exact number is pinned by the static validator (rule 4) — read it from
`Automation_Output.txt`, never from memory. Contracts to watch:
`ASTRAWILD.Inventory.TransactionSafety`, `ASTRAWILD.Save.SchemaV5Ending`,
`ASTRAWILD.Quest.FinalRunChain`, `ASTRAWILD.Dialogue.EndingChoice`,
`ASTRAWILD.SCI_FANTASY.MutationSystem`, `ASTRAWILD.DCP1.PostGameQuests`
(the DCP pack adds tests 127–133: post-game quests, NG+, ending cinematic,
Act 3 NPCs, journal detail, gamepad chord, mesh coverage).

On FAIL: the failing test's name + log excerpt goes into
`Docs/BUILD_STATUS.md`; world-free contract failures are source bugs —
capture and report to GLM unless the fix is a trivial mechanical one.

---

## 11. PHASE P9 — FULL GAMEPLAY VERIFICATION (the "เล่นได้ครบทั้งเกม" phase)

This is the phase that proves the game plays like a real game. It is ONE
continuous PIE session (~30–45 min) plus feature spot-checks. The full
23-stage golden path (V-6..V-28) lives in the queue §2 — walk it in order:
NEW GAME → explore → survive → find Echo → scan → combat → capture →
inventory → gather → BUILD BASE → power → assign Echo → automation →
research → advanced tech → DUNGEON → BOSS → reward → return → SAVE → quit →
LOAD → verify.

**PIE shortcut for the story tail:**
`AW.FastForward Quest_TheDrownedSovereign` (type in the console with ~)
completes the chain through MQ-16, then walk the MQ-17 homecoming marker
at Dawnstead and talk to Warden Maren to pick an ending.

**The v9.5 DCP feature matrix (verify each — these are the deferred items
the user personally re-opened):**

| Feature | How to verify |
|---|---|
| Ending cinematics | pick Ending A → letterbox bars + fade + 3 staged shots + title card "THE DAWN THAT STAYS" (~18.7 s, skippable) plays before the post-game banner |
| Post-game quests | after Ending A: talk to Maren/Wren/Tam/Nima/Kael → one-time offer dialogues appear; accept → 5 quests (PostVigil/PostFieldNotes/PostGlassTrade/PostDeepRecords/PostLongWatch) become trackable |
| NG+ | ESC → pause → **Start New Game Plus** (visible only post-game) → confirm → world resets BUT attributes/journal/affinities/top-3-bond Echoes carry; hostiles hit ~10% harder; research earns ~15% more |
| Vess & Ione | two new NPCs in Dawnstead (storm-scholar + relic trader bodies, real survivor meshes); Vess's 7-node lore tree unlocks as Act 3 MQs complete; Ione trades Maelstrom Glass once |
| Toast sound | any notification toast (world event, discovery, capture) plays A_UI_Confirm |
| Journal detail | P (Field Journal) → click a species row → detail panel (stats/habits/habitat — knowledge-gated: unseen fields show as locked) → Back |
| Gamepad smart-cast | with a controller: LB + X fires the smart-cast (the Y keyboard path still works) |
| Direct-mesh bodies | spot-check Wavecrest, Voltmaw, Voidwing, Verdantbloom, Undertowray (DCP-7 boss-spare binds) render their real skeletal meshes |

Also verify the PCR screens (P/L/M/U), the GDP/DP playtest additions
(HANDOFF "GDP Playtest Additions" list + DP-4..DP-9 items), and the
mutation visuals (V2-35: two bestiary species from different themes —
energy-body glow vs stone matte, attachments, persistent element VFX).

**Controls reference** (full set: `Docs/ASTRAWILD_PLAYER_RULES.md` +
HANDOFF §13): WASD move · mouse look · Space jump · Shift sprint · Q dodge ·
LMB attack · F heavy · RMB block/aim · E interact · C party command ·
R feed · G smart-consume · X equip-best · V scan (hold) · B build mode ·
Tab inventory · K research · H drone · J robot · F5 save · F9 load ·
P journal · L roster · M map · U hunt board · T party abilities ·
Y smart-cast · Esc pause.

Evidence per feature: a short clip or screenshot + one log line where
applicable. File them under `Docs/ENGINE_LOGS/raw/` (see P14).

---

## 12. PHASE P10 — SAVE/LOAD STRESS (queue V-25..V-28 + §20 step 6)

```powershell
.\Test_RealSaveLoad.ps1
```

(Env-adaptive since v9.6: `ASTRAWILD_PACKAGED_EXE` + `ASTRAWILD_REPO`
override the legacy `E:\` defaults.)

Minimum manual protocol (3 round-trips): create state (build a base, assign
Echoes, deploy drone) → F5 → quit to desktop → relaunch → F9 → verify:
buildings/party/equipment/research/quest/journal/zone discovery **plus work
assignments + site output + battery charge + rest state**; automation
resumes without re-assign. The old-save migration check (V-29): if any v1–v4
save exists, load it once and watch for the migration log line; a corrupted
file (flip one byte) must be REFUSED with a checksum error while the game
continues.

---

## 13. PHASE P11 — PACKAGE + STANDALONE RUN (queue V-41/V-42 + §20 step 7-8)

```powershell
$env:UE_ROOT = "C:\Program Files\Epic Games\UE_5.8"     # your engine root
$env:ASTRAWILD_ARCHIVE = "D:\Astrawild_Packaged"        # where you want the exe
.\Build_Package.ps1
```

**PASS = RunUAT exit 0.** Then launch the packaged game:

```powershell
& "D:\Astrawild_Packaged\Windows\ASTRAWILD.exe" -log
```

Expected: loading screen → MainMap → the same census log line as P7 →
input works → play the first loop steps (move/gather/craft/capture).
If the package fails at Entry-map policy (V-41's open question), capture
the full UAT log — do not retry blind.

---

## 14. PHASE P12 — LAN 4-PLAYER (queue §22 — the PART 22 contract)

1 host + 3 clients (separate machines or processes) on one LAN.
HOST: launch → ESC → LAN CO-OP → **Host LAN Game**. CLIENTS ×3: ESC →
LAN CO-OP → **Find + Join LAN Game** (beacon discovery ~1–2 s) or Direct
Connect by IP. Windows Firewall: allow `ASTRAWILD.exe` /
`UnrealEditor-ASTRAWILD.exe` inbound on Private networks when prompted.

Then walk the 17-row table in `Docs/ASTRAWILD_FINAL_BUILD_HANDOFF.md` §22
(world visible to clients → movement → interact routing → combat →
gather/craft/build → capture → party/abilities → mounts/skiff → vendor →
dialogue → quests/research → dungeons/bosses → host save → reconnect →
late join → authority sanity → session flow honesty). Capture raw logs
(host + one client minimum) named with the SHA.

---

## 15. PHASE P13 — EVIDENCE, STATUS, CLOSING (§20 steps 9-10)

1. Collect artifacts under `Docs/ENGINE_LOGS/raw/` using
   `<NAME>_<short-sha>.<ext>` (BUILD_1699d58.log, AUTOMATION_1699d58.log,
   RUNTIME_1699d58.log, import_report.json copy, screenshots/clips).
2. Update `Docs/BUILD_STATUS.md` playtest table: every queue row you
   exercised gets PASS/FAIL + evidence filename (honest per row).
3. Update `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` row statuses.
4. Update `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` §1 snapshot + §5 change
   log (ENGINE-RUN-1 → state per evidence; V2-29..V2-36 rows).
5. Commit + push to `final-completion` (task ID `FMP-ENG` or your agent
   convention + SHA + state + next). Never force-push.
6. When P5–P12 all pass: per HANDOFF §20, ASTRAWILD is GAME-COMPLETE
   (source-complete + engine-verified + packaged) — record the final SHA +
   log manifest in `Docs/ASTRAWILD_FINAL_READINESS_REPORT.md` §J.
   The merge to `main` (POST-ACCEPT-1) still belongs to GLM + the user.

---

## 16. TROUBLESHOOTING MATRIX

| Symptom | Diagnosis | Fix |
|---|---|---|
| `winget` missing | App Installer not registered | https://aka.ms/getwinget or use direct installer links in each phase |
| `git lfs version` fails | Git installed without LFS | re-run installer w/ LFS, or `winget install Git.Git.LFS`; then `git lfs install` |
| Clone stalls at LFS objects | PAT lacks LFS access | create classic PAT with `repo` scope; re-auth in Credential Manager |
| `python` not found after install | PATH not refreshed | open a NEW shell; or use the full path; re-run installer with "Add to PATH" |
| vswhere returns empty | workload missing | VS Installer → Modify → "Game development with C++" + MSVC v143 component |
| `UnrealEditor.exe not found` in Setup_And_Play | engine at non-standard root | `setx UE_ROOT "<root>"` then re-run the .bat |
| UBT ExitCode 6 | FZ-A1 class (seen once pre-final) | capture BuildHistory + UBT log FIRST, one retry, then stop + report |
| UHT rejects a header | codegen pattern | minimal mechanical fix only (queue §1 note); never restructure |
| Import report `total_missing > 0` | LFS sources incomplete | re-check P4.3 (`git lfs pull origin`); only then treat as engine finding |
| Terrain renders flat/wrong | Landscape material unset | P6 manual step: assign M_Landscape_SciFiFrontier on MainMap |
| PIE black screen / shader crash | DDC cold or RHI | wait for shader compile; retry; `-d3d11` only as a diagnostic, evidence on default RHI |
| Test count != 133 | repo/version drift | read the count from `Scripts/validate_final_run.py` gate — it pins the truth; do not "fix" the count |
| `git lfs fsck` exit 1 (~3,954 flags) | documented convention mismatch | NOTHING — do not rewrite history (rule 7) |
| Disk fills mid-build | intermediates + DDC | point DDC elsewhere (`-DDC=<path>` or shared DDC env); clean `Intermediate/` between evidence builds (full rebuild then required) |
| LAN clients cannot see session | firewall blocks beacon | allow the game binaries on Private networks; verify same subnet |
| Editor asks to rebuild modules on .uproject open | binaries stale vs source | say Yes; or run P5 first (preferred for evidence) |

---

## APPENDIX A — EXPECTED NUMBERS (single lookup, read from the repo at authoring)

| Metric | Expected |
|---|---|
| Branch / integration | `final-completion` |
| Automation contracts | 133 (validator gate pins it) |
| Census | 229 species · 204 bestiary rows · 12 zones · 22 quests (17 MQ + 5 post-game) · 13 NPCs/trees · 17 techs · 58 recipes · 26 buildings · 78 items · 11 loot tables · 8 weapons · 10 resource nodes · 8 work sites · 16 world events · 17 POIs · 3 robots |
| ArtSource manifest | 189 entries / 189 present / 0 pending (109 real meshes ← 109 unique CC0 sources) |
| Direct-mesh species coverage | 51 / 229 |
| LFS pointers | 586 (all objects on disk after `git lfs pull`) |
| Content packages | 416 .uasset/.umap with UE magic |
| Engine | UE 5.8.2, target `ASTRAWILDEditor Win64 Development` |
| Golden path stages | 23 (queue §2, V-6..V-28) |
| LAN acceptance rows | 17 (HANDOFF §22) |

## APPENDIX B — ONE-PAGE COMMAND SPINE

```powershell
# 0 readiness            powershell -ExecutionPolicy Bypass -File Scripts\fresh_machine_preflight.ps1 -RepoRoot .
# 1 tools                winget install --id Git.Git -e; winget install --id Python.Python.3.12 -e
# 2 VS                   winget install --id Microsoft.VisualStudio.2022.Community -e --override "--quiet --wait --add Microsoft.VisualStudio.Workload.NativeGame --includeRecommended"
# 3 UE 5.8.2             Epic Games Launcher -> Library -> install 5.8.2 -> setx UE_ROOT "<root>"
# 4 repo                 git clone https://github.com/banksaisuoy/astrawild-game AstrawildGame; cd AstrawildGame
#                        git lfs install; git checkout final-completion; git pull origin final-completion; git lfs pull
# 4 gates                python Scripts\validate_final_run.py; & "C:\Program Files\Git\bin\bash.exe" Scripts/validate_repository.sh
# 5 build                .\Build.ps1                                       # exit 0
# 6 import+showcase      .\Setup_And_Play.bat                             # import_report.json total_missing==0
# 6b echo bases          (editor python) py "Content/Python/AwPipeline/import_echo_bases.py"   # echo_base_report.json
# 6c landscape material  MainMap -> Landscape actor -> M_Landscape_SciFiFrontier (manual)
# 7 PIE boot             open Content/ASTRAWILD/Maps/MainMap.umap -> Play -> census log line
# 8 tests                .\Test.ps1                                        # 133/133 Result={Success}
# 9 golden path          queue section 2 V-6..V-28 + AW.FastForward Quest_TheDrownedSovereign
# 10 save/load           .\Test_RealSaveLoad.ps1  (+ manual 3-cycle protocol)
# 11 package             .\Build_Package.ps1 -> <archive>\Windows\ASTRAWILD.exe -log
# 12 LAN                 HOST: Host LAN Game / CLIENTS x3: Find + Join (17-row table, HANDOFF section 22)
# 13 evidence            Docs/ENGINE_LOGS/raw/<NAME>_<sha>.log + BUILD_STATUS + queue + LIVE_STATE -> commit + push
```

## APPENDIX C — ENVIRONMENT VARIABLE CONTRACT (v9.6)

The wrapper scripts resolve paths in this order (first hit wins; absent
env vars keep the exact legacy `E:\` defaults so the reference machine's
documented commands still work verbatim):

| Variable | Used by | Meaning |
|---|---|---|
| `UE_ROOT` | Build.ps1, Test.ps1, Build_Package.ps1, Verify_Runtime.ps1, Setup_And_Play.bat | Unreal engine install root (contains `Engine\Binaries\...`) |
| `ASTRAWILD_UPROJECT` | Build.ps1, Test.ps1, Verify_Runtime.ps1 | full path to ASTRAWILD.uproject |
| `ASTRAWILD_REPO` | Test_RealSaveLoad.ps1, Evidence_Playtest.ps1, Test_V30_Replication.ps1, Test_PlayableInput.ps1, preflight | repo working-tree root |
| `ASTRAWILD_ARCHIVE` | Build_Package.ps1 | packaged-output archive directory |
| `ASTRAWILD_PACKAGED_EXE` | Test_RealSaveLoad.ps1, Evidence_Playtest.ps1, Test_V30_Replication.ps1 | path to the packaged ASTRAWILD.exe |
| `ASTRAWILD_AUTOMATION_OUTPUT` | Test.ps1 | automation log output path |

Every adapted script prints the resolved paths it is about to use — read
them before letting it run, and never let a legacy `E:\` default silently
point at a nonexistent drive on your machine.
