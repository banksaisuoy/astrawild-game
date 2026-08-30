# ASTRAWILD — Test Plan

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine) — 12 automation tests are
written; **none have been executed** (no UE toolchain in the sandbox). Execution happens on the target
Windows machine.**
**Date: 2026-08-30** (wave 6 sync — counts refreshed through Batch 4: +2 armor/status tests in Batch 3, +1 vendor economy test in Batch 4; 5 of 12 now call production statics)
**Primary sources:** `AstrawildAutomationTests.cpp`, `Scripts/validate_repository.sh`,
directive §39 (automation) + §50 (first-playable flow)

---

## 1. Automation Tests (12)

All tests are `IMPLEMENT_SIMPLE_AUTOMATION_TEST` under the `ASTRAWILD.` namespace,
flags `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter`, compiled behind
`WITH_DEV_AUTOMATION_TESTS` (stripped from Shipping). They are **world-free logic tests** — safe to run
from the Session Frontend or CLI without a map.

| # | Test name | What it verifies (from the code) |
|---|---|---|
| 1 | `ASTRAWILD.Inventory.AddRemove` | Stack validity contract: `FAstrawildItemStack().IsValid()` is false for zero quantity; a `Item_Wood ×5` stack is valid |
| 2 | `ASTRAWILD.Survival.DamageAndDeath` | Vitals struct invariants: health starts 100, stamina fraction math (50/100 = 0.5), player starts alive (`bIsDead = false`) |
| 3 | `ASTRAWILD.Capture.DesignRuleBounds` | Species template sanity: `CaptureDifficulty` and `CaptureResilience` live in 0..1 on a constructed `UAstrawildEchoDefinition`; documents the defeated→0-chance rule |
| 4 | `ASTRAWILD.Combat.MitigationMath` | Block math with a 0.65 mitigation input on 100 damage passes exactly 35 (the input now corresponds to the Stonehide Shield tier); weakness multiplier ×1.5 (20 → 30) |
| 5 | `ASTRAWILD.Save.ChecksumDeterminism` | FNV-1a checksum: deterministic for identical (schema, timestamp); differs across schema versions; non-zero |
| 6 | `ASTRAWILD.Quest.ObjectiveProgress` | Objective completion math: 7/10 incomplete, 10/10 complete, over-progress (12/10) still complete |
| 7 | `ASTRAWILD.Echo.PersonalityModifiers` | Personality contract surface: enum covers all 10 archetypes (`Social == 9`); Timid ≠ Brave (flee-ordering invariant hook) |
| 8 | `ASTRAWILD.Power.BrownoutMath` | Grid math: draw 9 > generation 8 → brownout; shedding the lowest-priority consumer (4.0) restores draw 5 ≤ 8 |
| 9 | `ASTRAWILD.Equipment.ProgressionMath` | Wave 3 equipment contract: unarmed light stays 25; Dawnwood Club light 25+6=31; Dawn Crystal Blade heavy 60+14=74; unarmed block (0.45) passes 55 % of 100; shielded block (0.65) passes 35 %; shield strictly improves block |
| 10 | `ASTRAWILD.Equipment.ArmorMath` | Batch 3 armor contract — calls the REAL production static `UAstrawildCombatComponent::ComputeArmorFraction`: tier ratings 20/45/80 → 16.7/31.0/44.4 % reduction with diminishing-returns ordering; 1,000,000-rating clamps to `ArmorMaxFraction 0.6`; K=0 degenerate case → 0; block+cuirass ≈ 19.4 of a 100 hit |
| 11 | `ASTRAWILD.Combat.StatusEffectFactory` | Batch 3 status contract — calls the REAL production factory `UAstrawildCombatComponent::MakeElementalStatusEffect`: Ember→Burn (4 s, DPS 2+5 %×40), Frost→Chill (3 s, ×0.5), Flora→Poison (6 s, 2 DPS), Pulse→Shock (0.8 s, ×0.3); None/Light/Ash → no status |
| 12 | `ASTRAWILD.Economy.VendorSellValue` | Batch 4 vendor contract — calls the REAL production static `AAstrawildNPCCharacter::ComputeVendorSellValue`: price 2→1, 3→1 (floor), 4→2, 6→3; unpriced (0) → 0 (junk not sellable); sell value strictly below buy price for every priced ware (no arbitrage) |

Run on the target machine: **Tools → Session Frontend → Automation** (filter `ASTRAWILD`), or:
`UnrealEditor-Cmd.exe ASTRAWILD.uproject -ExecCmds="Automation RunTests ASTRAWILD" -unattended -nopause -testexit="Automation Test Queue Empty"`

**Honest scope note:** these tests validate struct/enum math and pure functions. World-dependent behavior
(AI states, capture rolls, replication) needs the manual + PIE passes below and future functional
tests (World-based `IMPLEMENT_SIMPLE_AUTOMATION_TEST` with a test world — PLANNED).

---

## 2. Manual Playtest Checklist — First Playable Flow (directive §50)

Run in PIE after a successful editor compile. Each step must pass before the next; record results in
`BUILD_STATUS.md` → Playtest.

| # | Step | Expected result | Systems exercised |
|---|---|---|---|
| 1 | **Launch** — open the project in UE 5.8 editor, press Play | No errors in Output Log beyond warnings; Dawn Fields visible (lit ground plane, sky) | Bootstrapper, lighting rig |
| 2 | **Menu** — (deferred) | Direct-to-game by design; note it: real main menu is M7 work | — |
| 3 | **Spawn** — player capsule spawns at the fallback PlayerStart near origin | Player standing on ground, camera behind | GameMode, PlayerStart fallback |
| 4 | **Move** — WASD + mouse look, Shift sprint, Space jump | Movement responsive; sprint blocked when stamina < 5 % | Runtime IMC, movement |
| 5 | **Explore** — walk toward a resource node | Interaction prompt appears when aimed at node; day clock advances (1 real sec = 1 world min); HUD vitals tick down slowly | HUD, TimeSubsystem, Survival |
| 6 | **Find Echo** — approach a wild Lumewisp (sphere) | Curious Echo may approach/investigate you; others wander | Echo AI, perception |
| 7 | **Fight / Observe** — LMB light, F heavy, Q dodge, RMB block on the Echo; or just keep it in view | Damage applies (Echo flees at low HP per personality); journal observation % climbs while in view; capture chance % updates on HUD | Combat, Journal, HUD |
| 8 | **Capture** — E on the Echo (needs Resonator; feed berries with R first for trust) | Chance-based capture; on success Echo joins party and follows; `AW.CaptureAll` as fallback for testing | Capture pipeline, Roster |
| 9 | **Inventory** — (stopgap) verify via `AW.GiveItem` + quest pickups | Items land; weight gate visible through logs | Inventory |
| 10 | **Gather** — E on resource nodes ×N | Wood/Stone/Fiber added; nodes deplete (3 harvests) and respawn | ResourceNode, Inventory, quests (First Light progress) |
| 11 | **Craft** — walk to the workbench (0, +900) and press E | First craftable station recipe crafts (timed queue visible in log); Resonator/Bandage craft anywhere via component API | Crafting, stations |
| 12 | **Build** — B, aim, N to rotate, LMB to confirm | Preview ghost follows snapped grid; building spawns; materials consumed | Building placement |
| 13 | **Assign** — capture an Echo, C-cycle to Work near a work site (or `AW.SpawnEcho` + site assign) | Echo walks to the site and production accumulates (`StoredOutput` via log) | Work sites, commands |
| 14 | **Save** — F5 (and/or wait 300 s for autosave) | Save file written (ASTRAWILD_Main / ASTRAWILD_Auto); log line confirms counts | Save v2 |
| 15 | **Quit** — stop PIE, close editor (optional full restart) | Clean shutdown, no crash on exit | — |
| 16 | **Reload** — relaunch PIE, F9 | World restores: time/day/weather, player position, inventory, buildings, roster, quests, journal | Save/Load orchestration |
| 17 | **Continue** — verify quest tracker resumes mid-chain; kill time to next weather change | Quest progress persisted; weather transitions on cadence | Quests, Weather |

**Full-loop acceptance:** a new player completes steps 1–17 and finishes at least the First Light +
First Echo quests unaided. That is the vertical-slice bar for this round.

---

## 3. Cheat-Assisted Test Matrix

| Scenario | Cheats |
|---|---|
| Stress Echo AI / tier LOD | `AW.SpawnEcho Echo_Lumewisp` ×10, walk away 80 m+ |
| Night/hostile behavior | `AW.SetTime 22 0` (Gloomfangs aggro), `AW.SetTime 8 0` (day) |
| Weather capture bonuses & temperature | `AW.SetWeather fog` / `heat` / `cold` |
| Combat survivability | `AW.God`, `AW.HealAll` |
| Tech/quest fast-path | `AW.ResearchPoints 100`, `AW.UnlockTech Tech_Electrical` |
| Equipment progression | `AW.GiveItem Item_CrystalBlade 1`, `AW.EquipItem Item_CrystalBlade` (then X to equip-best; check the HUD equipment readout + block damage change) |
| Vendor economy (Batch 4) | Stand within 6 m of Trader Tam: E (Interact — wares + prices + Dawn Shard balance toast), `AW.BuyItem Item_Bandage 2`, `AW.SellItem Item_Berry 5`; verify funds/weight refusals (NotEnoughCurrency / TooHeavy) and that the currency itself is not sellable |
| Save edge cases | `AW.SaveNow`, `AW.LoadNow`, `AW.CaptureAll`, `AW.TeleportForward 5000` |

---

## 4. Compile Validation Steps (user's Windows machine — REQUIRED FIRST)

The repository has **never been compiled**. Do this before any test above:

1. **Prereqs:** Windows 10/11, Visual Studio 2022 (Desktop development with C++ + Game development with
   C++ workloads), Unreal Engine **5.8** (launcher or source) installed.
2. **Clone/pull** `banksaisuoy/astrawild-game` @ `main` (this round's commits through
   `7775668`), or use the existing Antigravity workspace copy at the project path.
3. **Right-click `ASTRAWILD.uproject` → Generate Visual Studio project files.** (If the context menu is
   missing: run `<UE5.8>\Engine\Build\BatchFiles\Build.bat -projectfiles`… or UnrealVersionSelector
   associate.)
4. Open `ASTRAWILD.sln`, select **`ASTRAWILDEditor Win64 Development`**, **Build** (Ctrl+Shift+B).
   Expected duration: a few minutes for one module; engine must already be installed.
5. **Fix-first policy:** any compile errors are logged as blocking issues in `BUILD_STATUS.md` — fix before
   proceeding (the code is compile-conservative, but UE API drift between 5.x minor versions is possible).
6. Launch `ASTRAWILDEditor` → the project opens with an empty content tree (expected) → **PIE works
   without a map** via the bootstrapper (§2 step 1).
7. Run the automation suite (§1 command) — 9/9 expected green.
8. Fill `BUILD_STATUS.md`: Compile = PASS (with duration/warnings), Playtest rows with real results.
   **Do not mark COMPLETE before steps 1–17 of §2 pass.**
9. Optional static validation re-run (sandbox): `Scripts/validate_repository.sh`.

---

## 5. Known Issues to Verify During Playtest

Status re-checked against code on 2026-08-30: T-1/T-2/T-4/T-5/T-6 have **fixes in code** (applied with
the lead's foundation round) but none are verified — nothing has been compiled or run yet. Verify each
during the first playtest; T-3 is still unfixed.

| ID | Issue | Where | Status / verification |
|---|---|---|---|
| T-1 | Quest 2 "Observe a Lumewisp" progression (no `Event.*` publisher for ObserveEcho) | QuestComponent / JournalSubsystem | **Fixed in code** — journal publishes `Event.EchoObserved` at the 25 % first-scan milestone; verify Quest_FirstEcho completes both objectives |
| T-2 | AI think loop reschedules per frame (LOD interval computed but not applied) | EchoAIController::Think | **Fixed in code** — `SetTimer(ThinkIntervalSeconds)`; verify with Insights at 10+ Echoes |
| T-3 | Weather temperature label hard-codes 20 °C on the HUD | HudWidget | **Still present** (`WeatherText` prints a literal `20.0f`); cosmetic |
| T-4 | Cold/heat damage unreachable with default weather offsets | Survival + Weather | **Fixed in code** — Cold profile offset −17 (felt 3 °C < 4 °C threshold), Heat +20 (felt 40 °C ≥ 36 °C); verify with `AW.SetWeather cold` / `heat` |
| T-5 | Player "consume food/drink" action has no keybind | Survival / input | **Fixed in code** — **G** = `SmartConsume` (`AWD_Consume`); verify with berries/flask in inventory |
| T-6 | Journal per-frame actor iteration | JournalSubsystem | **Fixed in code** — throttled observation sweep; verify with Insights |
