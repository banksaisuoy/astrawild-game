# ASTRAWILD — Production Roadmap V2

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-30** (wave 3 sync — STEP 28 content waves progressing; dungeon round reflected)
**Context:** audit `ASTRAWILD_UE5_ARCHITECTURE_AUDIT.md` §G defined milestones M0–M7; this document maps the
directive's 28 production steps and the audit milestones onto the code that actually exists on `main`
(commits `3872c7e` → latest).

Legend: **DONE (code)** = implemented in C++, compile pending · **PARTIAL** = core done, gaps listed ·
**PENDING (user machine)** = requires the target Windows/UE toolchain · **PLANNED** = future round.

---

## 1. The 28 Directive Steps — Status

| # | Step | Status | Evidence / Notes |
|---|---|---|---|
| 01 | Repository Audit | **DONE** | `ASTRAWILD_UE5_ARCHITECTURE_AUDIT.md` (M0, commit `b0378b5`) |
| 02 | Build Environment Validation | **PENDING (user machine)** | Sandbox has no UE/MSVC; compile validation steps in `ASTRAWILD_TEST_PLAN.md` §4 |
| 03 | Architecture Consolidation | **DONE (code)** | Architecture V2 doc + audit §F; single-module subsystem/folder architecture |
| 04 | Module Restructure | **DONE (code)** — as a *folder* restructure | Single runtime module `AstrawildCore` + Public/Private groups (deliberate; split deferred post-compile — Assumptions #2) |
| 05 | Core Framework Hardening | **DONE (code)** | 8 log categories, 77 native gameplay tags, types v2, 8 data-asset definition classes, module deps (+AIModule/Nav/UMG) |
| 06 | Player Controller & Camera | **DONE (code)** | PlayerCharacter (spring arm 360 cm, orient-to-movement, sprint/jump), PlayerController, runtime IMC |
| 07 | Survival Systems | **DONE (code)** | SurvivalComponent (rates in Survival doc); death→respawn 5 s |
| 08 | Echo AI Core | **DONE (code)** | EchoAIController: sight perception + 16-state machine, personality modulation |
| 09 | Echo AI Behavior Expansion | **PARTIAL** | 9 executors wired (incl. `ExecuteSocialize` herd behavior — wave 2 — and predator stalking); Eat/Alert/ReturnHome/Injured/Dead executors + BT assets PLANNED; T-2 think-rate fix in code (verify at playtest) |
| 10 | Capture System Rework | **DONE (code)** | Multi-step pipeline: observe (journal +15 %) → track (+5 %) → weaken/trust → roll; Resonator cost |
| 11 | Inventory 2.0 | **DONE (code)** | Stacks + weight gate 120 kg + equipment slots (weapon + shield — wave 3); inventory UI PLANNED |
| 12 | Crafting System | **DONE (code)** | Timed queue, station/tech/ingredient gates; UMG contract implemented (`UAstrawildCraftingScreenWidget`), screen assets PLANNED |
| 13 | Base Building | **DONE (code)** | B/N/LMB placement, 200 cm grid, ghost, server validation + refund; structural snapping PLANNED |
| 14 | Power Grid | **DONE (code)** | Gen/draw/battery, 2 s re-solve, priority brownout shedding |
| 15 | Creature Work Assignment | **DONE (code)** | WorkSiteActor + command path; player-facing output pickup PLANNED |
| 16 | Research / Tech Tree | **DONE (code)** | 6 techs (incl. Armory — wave 3), prereqs, points from journal+quests; tree UI PLANNED |
| 17 | Quest System | **DONE (code)** | Event-driven, 6-quest First Dawn chain; only ReachLocation/SurviveTime lack publishers (T-1 ObserveEcho fixed in code) |
| 18 | Ecosystem Simulation | **DONE (code)** | LOD tiers 0–3, population tracking, predator-prey chains + herding; dynamic spawning PLANNED |
| 19 | Weather System | **DONE (code)** | 8 states, weighted 90-world-min transitions, profiles |
| 20 | Day/Night Cycle | **DONE (code)** | 1 s = 1 world min, 24-min day, sun arc, activity windows |
| 21 | Procedural Dawn Fields | **DONE (code)** | WorldBootstrapper: 160 m arena, 26 nodes, wild species rotation, camp + 2 NPCs (wave 3) |
| 22 | Dungeon & Boss Encounter | **DONE (code)** | Hollow Underlight: deterministic linear chain, room templates + encounters, 3-phase Echo boss, clear-reward loot table (wave 3) — commit `a0634f6` |
| 23 | Save Schema v2 | **DONE (code)** | Schema v2 + FNV-1a + v1 migration + autosave 300 s; additive equipment fields stay v2 (wave 3) |
| 24 | Vertical Slice Integration | **DONE (code)** — pending playtest | Full loop playable in code; §50 checklist awaits compile |
| 25 | Multiplayer Architecture Prep | **DONE (code)** | Authority guards, 25 replicated props, 5 server RPCs; co-op playtest PLANNED (Multiplayer doc §5) |
| 26 | Debug & Dev Tools | **DONE (code)** | CheatManager (13 exec functions — wave 3 adds `AW.EquipItem`), interaction debug draw flag, 8 log categories |
| 27 | Performance Optimization | **PARTIAL** | LOD + tick budgets designed (Performance doc); profiling + hotspot fixes PENDING (user machine) |
| 28 | Content Expansion | **PARTIAL** | **CODE_DEFAULT waves 1–3 delivered in code** (wave 1 foundation content; wave 2 husbandry economy 7 species/16 items/7 recipes/5 techs/6 quests; **wave 3 equipment progression + armory tech + 2 loot tables + 2 camp NPCs — 19 items / 10 recipes / 7 species / 10 buildings / 6 techs / 6 quests**); real assets, second biome, NPC dialogue still blocked on compile + art pass (M9–M11) |

**Tally:** 24 DONE (code) · 3 PARTIAL · 1 PENDING (user machine) · 0 PLANNED — of which **all** "DONE (code)"
items carry the shared caveat: *never compiled* (sandbox constraint, Assumptions #1).

---

## 2. Audit Milestones (M0–M7)

| Milestone | Scope | Status |
|---|---|---|
| M0 — Audit | Repository audit document | **COMPLETE** |
| M1 — Foundation | Log tags, gameplay tags, types v2, module deps | **COMPLETE (code)** — commit `3872c7e` |
| M2 — World | GameState, time, weather, event bus, bootstrapper | **COMPLETE (code)** — commit `a25404c` |
| M3 — Player | Survival, combat v2, HUD, death/respawn, PC + quests | **COMPLETE (code)** — commit `4ecaf4d` |
| M4 — Echo | Needs/personality/growth/AI/commands/roster/work | **COMPLETE (code)** — commit `4ecaf4d` |
| M5 — Systems | Capture, journal, inventory v2, content lib, crafting, building, power, research, quests, save v2, cheats | **COMPLETE (code)** — commit `4ecaf4d` |
| M6 — Tests + docs | 9 automation tests + this docs suite + validate script v2 | **COMPLETE (code/docs)** — commit `7775668` + later rounds |
| M7 — (User machine) | Compile + map/IMC real assets + playtest | **PENDING — the single blocking milestone** |

---

## 3. Next Phases (M7 → M12, proposed)

| Phase | Content | Exit criteria |
|---|---|---|
| **M7 — Compile & First Playable** (user machine) | Generate project files, build `ASTRAWILDEditor Win64 Development`, run the 9 tests, run the §50 17-step flow, verify the 6 known-issue fixes (Test Plan §5) | Compile PASS, tests 9/9, first-playable flow complete, `BUILD_STATUS.md` filled with real results |
| **M8 — Polish the Slice** | Verify T-1…T-6 fixes (all in code, none run yet), remaining M8 polish: ghost validity tinting, HUD temp label (T-3), playtest balance pass (capture chances, decay rates, equipment numbers) | Re-run §50 clean; Insights baseline captured (Performance doc §6) |
| **M9 — Real Assets 1** | 7 Echo meshes + icons, 19+ item icons + equipment world models, real Dawn Fields map replacing the bootstrapper arena (keep bootstrapper as fallback), optional IMC assets | Zero placeholder Echoes/items; PIE works from the real map |
| **M10 — Co-op Foundation** | Multiplayer work list items 2–8 (Multiplayer doc §5): 2–4 player listen-server sessions, quest replication, per-player save v3, server-routed interaction | 4-player session with no dupe/desync in the core loop (master plan gate) |
| **M11 — Content Alpha** | Biome 2 (Luminous Rainforest), dungeon content beyond Hollow Underlight, tech tree expansion (AdvancedEnergy consumers), vendor purchase flow + NPC schedules/dialogue, loot-table content growth | Master plan "Content Alpha" gate |
| **M12 — Beta prep** | Performance regression gates, accessibility pass (remap, colorblind), save migration hardening, 3-slot backups | Master plan "Beta" gate |

---

## 4. Immediate Next Actions (ordered)

1. **User machine: compile** (Test Plan §4) — everything else is blocked behind it.
2. File compile findings into `BUILD_STATUS.md`; fix-forward on `main`.
3. Run automation suite + §50 manual flow; capture first Insights baseline.
4. Close T-1…T-6 (Test Plan §5).
5. Begin M9 asset pass using `ASTRAWILD_ASSET_MANIFEST.md` as the checklist.
