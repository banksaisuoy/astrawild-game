# ASTRAWILD — PRODUCTION CHECKLIST V2

This file is the machine-readable human-readable acceptance checklist for AI agents. Never mark an item complete from documentation alone. Use `[x]` only when implementation exists and has been verified.

> **Evidence source of truth (final production run, commit `249eec7`):** file:line
> evidence lives in `Docs/ASTRAWILD_UE5_PRODUCTION_AUDIT.md` (batches 1–7) and
> `Docs/ASTRAWILD_GL53_SOURCE_AUDIT.md` (final run). `[~]` marks implementation that
> exists in source but is NOT engine-verified (the project has never been compiled —
> sandbox has no UE). Nothing in this file is `[x]`-verified yet.

Legend:
- `[ ]` Not started
- `[~]` In progress / partial
- `[x]` Implemented and verified
- `[!]` Blocked / failed

## P0 Audit
- [x] Repository audit completed
- [!] Existing UE5 project builds — never compiled; all static-review compile blockers (C-1, C-1b, C-2 + final-run C-1..C-12) fixed in source; engine build pending (verification queue §1)
- [x] Existing systems classified KEEP/REFACTOR/REPLACE/REMOVE/ADD
- [x] Architecture gaps documented
- [x] Technical debt documented
- [!] Current baseline build recorded — baseline = "does not compile" until proven otherwise on the target machine

## P1 Foundation
- [~] UE5 version locked/documented (5.8, both targets, uproject)
- [~] C++ architecture established (single module AstrawildCore, 114 files)
- [~] Gameplay subsystems established (world + game-instance subsystems)
- [~] Logging categories established (LogAstrawild*)
- [~] Gameplay Tags taxonomy established (82 native tags)
- [~] Data Asset conventions established (item/recipe/echo/building/tech/quest/loot/NPC)
- [~] Stable ID strategy established (FGuid instances + FName definition ids + site ids)
- [~] Enhanced Input architecture established (runtime-built IMC/actions, KB/M + gamepad)
- [~] Automated test foundation established (25 tests)

## P2 Player
- [~] Third-person character
- [~] Camera (spring arm + follow camera)
- [~] Movement (walk/sprint/block-slow/stagger-zero/status-slow + exosuit bonus)
- [~] Sprint (stamina drain + exhaustion)
- [~] Jump
- [~] Dodge (i-frames)
- [~] Interaction (trace + IAstrawildInteractable)
- [~] Controller support — final run: full gamepad companion IMC (sticks/face/shoulders/D-pad/select/start)
- [ ] Input remapping UI
- [ ] Traversal foundation
- [ ] Animation architecture (placeholder visuals; asset pass deferred)

## P3 Survival
- [~] Health
- [~] Stamina (+ exosuit regen bonus)
- [~] Hunger
- [~] Thirst
- [~] Temperature (+ insulation band from helm/exosuit — final run)
- [~] Status effects (elemental vocabulary shared across player/Echo/boss)
- [~] Food
- [~] Medicine (bandage/salve/broth)
- [~] Death
- [~] Respawn (input restoration)

## P3 Inventory / Equipment
- [~] Item Definition
- [~] Item Instance
- [~] Stack
- [~] Weight (+ exosuit carry bonus — final run)
- [~] Equipment — 6 slots: weapon/shield/torso/helmet/exosuit/scanner (final run)
- [~] Inventory UI — TAB screen with stacks/weight/loadout/use/equip (final run)
- [ ] Drop/pickup world items
- [~] Transfer (collect output / vendor trade)
- [~] Persistence (all six slots, schema v3)
- [~] Duplication tests (checksum + weight gates; engine-run pending)

## P4 Combat
- [~] Damage framework
- [~] Melee (light/heavy, arc sweep)
- [~] Ranged — Pulse Lance projectile path, ammo-gated (final run)
- [~] Dodge/parry architecture
- [~] Hit reaction
- [~] Stagger (player + creature)
- [~] Death
- [~] Status effects
- [~] Element system (7 elements, weakness/resist vocabulary)
- [~] First hostile enemy (4 hostile species + spawner)

## P5 First Complete Echo
- [~] Species Definition
- [~] Instance state
- [~] AI Controller
- [~] Navigation — runtime navmesh via navigation invokers (Batch 1 C-3); runtime-unverified
- [~] Perception
- [~] Behavior
- [~] Personality
- [~] Needs
- [~] Combat
- [~] Capture
- [~] Ownership
- [~] Follow
- [~] Commands
- [~] Relationship (trust + bond)
- [~] Utility role — work sites interactable; robot co-workers (final run)
- [~] Save/Load — party respawn + work assignments re-link (schema v3)
- [ ] Edge-case tests

## P6 Echo Platform
- [~] Data-driven species creation (10 species, definition-driven)
- [~] Stats
- [~] Traits (personality archetypes)
- [~] Abilities (work affinities + elements)
- [~] Habitat (biome ids per zone)
- [~] Diet (preferred foods feed capture/trust)
- [~] Time behavior (activity patterns)
- [~] Weather behavior (preferred weather)
- [~] Work roles (11 work types)
- [ ] Additional Echo content pipeline (asset pass)

## P7 Crafting
- [~] Recipe definition (26 recipes)
- [~] Ingredients
- [~] Stations (workbench/campfire/desk)
- [~] Technology requirements (tech gating real)
- [~] Crafting UI — inventory/research screens live (final run); dedicated crafting screen remains the Abstract widget + station quick-craft
- [~] Output validation — collection path weight-safe; craft output validation still ignores AddItem result (gap H-11)
- [~] Save/load compatibility

## P7 Base Building
- [~] Foundation
- [~] Wall
- [~] Workstation (workbench/campfire/research desk/sawmill/heater)
- [~] Farm (trough/composter)
- [~] Power (generator/battery/lamp)
- [ ] Floor
- [ ] Roof
- [ ] Door
- [ ] Storage
- [~] Placement preview (ghost + validity)
- [~] Snap (200 cm grid)
- [~] Collision validation
- [~] Delete — dismantle + refund (Batch 2)
- [ ] Repair
- [~] Save/load — health + switch + power-state + charge (schema v3)
- [~] Ownership foundation (OwnerPlayerId records)

## P8 Power / Automation
- [~] Generator (Echo Dynamo)
- [~] Battery (Charge Cell)
- [~] Power network (proximity grid, 2 s resolve)
- [~] Consumers
- [~] Power priority (brownout shedding)
- [~] Power failure
- [ ] Energy UI (HUD readout only)
- [~] First automation loop — work-site interact + robots (final run)
- [~] Persistence — grid StoredEnergy restored on load (H-6 closed in final run, schema v3); per-building power state (Batch 2)

## P9 Advanced Technology
- [~] Technology tree — 10 nodes, every era used; research screen (final run) restores player choice
- [~] Research system — points/prereqs/unlock logic + full UI screen (final run)
- [~] Armor framework (rating → diminishing-returns mitigation)
- [~] Helmet slot — Resonance Helm (armor + insulation, final run)
- [~] Chest slot — 3-tier torso progression (fiberweave/emberhide/crystalplate)
- [ ] Arms slot
- [ ] Legs slot
- [ ] Boots slot
- [ ] Core/backpack module
- [~] Exosuit framework — Dawnstrider (insulation/stamina/carry/speed, final run)
- [~] Shield module — Stonehide Shield
- [~] Thermal module — insulation band (final run)
- [~] Scanner module — Field Scanner, hold-to-scan (final run)
- [~] Energy capacitor — Pulse Cells (laser ammunition economy, final run)
- [ ] Traversal module
- [~] Laser weapon — Pulse Lance (projectile path, final run)
- [ ] Plasma/advanced energy weapon
- [ ] Missile/guided projectile framework
- [ ] Weapon heat/energy model where appropriate
- [ ] Weapon mod framework
- [~] Utility drone framework — AAstrawildUtilityDroneActor (final run)
- [~] One complete drone — Item_UtilityDrone, deploy/recall/save (final run)
- [~] Utility robot framework — AAstrawildUtilityRobotActor (final run)
- [~] One complete robot — Item_UtilityRobot, mans sites, save-restore (final run)
- [~] Technology integrates with exploration — scanner/journal + drone research feed (final run)

## P10 World
- [ ] World Partition (explicitly deferred — runtime PMC tiles instead; documented)
- [~] Day/night
- [~] Weather
- [~] Biome definition — six zones (Batch 7)
- [~] Resource spawning — per-zone tables (Batch 7)
- [~] Creature population — spawner + idempotent population counting (final-run C-9: capture/defeat/destroy each release exactly once)
- [~] Ecosystem tiers (distance-based simulation tiers)
- [ ] Migration foundation
- [ ] Dynamic events
- [ ] Streaming/HLOD baseline

## P11 Dawn Fields Vertical Slice
- [~] Starting area
- [~] 3–5 Echo (10 species)
- [~] Hostile creatures
- [~] Resources
- [~] Base location
- [~] Crafting
- [~] Research
- [~] Advanced technology unlock
- [~] Dungeon — gated progression + save persistence (Batch 6)
- [~] Boss — 3 phases + telegraphs + weak point + hazards + HP bar (Batch 6 + final run)
- [~] Main quest slice — 8 quests; every objective type implemented incl. SurviveTime + VisitZone (final run)
- [~] Weather
- [~] Day/night
- [~] Save/load — schema v3 full coverage (vitals/party/buildings/power/research/quests/journal/dungeon/zones/work-sites/robots/drones/rest-points)
- [~] Full gameplay loop — code-reachable end-to-end: 23/23 loop stages implemented (final run); compile + runtime + save round-trip pending on target machine

## P12 NPC / Story
- [~] NPC framework (2 NPCs, interact + vendor)
- [~] Dialogue (interaction prompts + notifications)
- [ ] Schedule
- [~] Faction foundation (Wild/Player/Hostile tags)
- [~] Main quest chain (8-quest chain to "The Vale Beyond")
- [ ] Side quest framework
- [~] World state (world save block)
- [~] Lore discovery (field journal)

## P13 Multiplayer
- [~] Server authority rules (documented SP/listen-server policy; mutations authority-gated)
- [~] Player replication (controller/pawn/components replicated)
- [~] Creature replication
- [~] Inventory authority (server + replicated slots)
- [~] Capture authority (server RPC)
- [~] Building authority (server placement)
- [~] Quest authority (controller component, server events)
- [ ] Shared world state
- [ ] Save authority
- [ ] Join/leave handling
- [ ] 1–4 player co-op test
- Remote-client shop/dismantle RPC layer deferred (H-12, documented)

## P14 Content Expansion
- [~] The Shattered Vale — 6 zones delivered (Batch 7); the old zone list (Luminous Rainforest/Salt Plains/Azure Snowline/Veldara Ruins) is superseded by the six-zone world
- [ ] Additional Echo species (asset/content pass)
- [ ] Additional dungeons (DungeonId infra ready)
- [ ] Additional bosses (boss framework hardened, second boss is content)

## P15 Optimization
- [ ] CPU profiling
- [ ] GPU profiling
- [ ] RAM profiling
- [ ] VRAM profiling
- [ ] AI profiling
- [ ] Network profiling
- [ ] Streaming profiling
- [ ] Save/load profiling
- [~] Tick audit (per-frame ticks catalogued + throttled; see GL53 audit)
- [~] AI simulation tier validation (tier sweep implemented)
- [ ] 60 FPS target tested on defined baseline hardware

## P16 QA / Release Foundation
- [!] Automated tests pass — 25 written; ALL NOT_RUN (engine required)
- [ ] Runtime smoke test passes
- [ ] Save/reload test passes (3× round-trip queued: V-25..V-28)
- [ ] Regression test passes
- [ ] Duplication tests pass
- [ ] Crash tests pass
- [ ] Multiplayer regression passes
- [~] Controller test — gamepad mapping implemented (final run); engine verification V-31
- [ ] Accessibility settings tested
- [ ] Packaging test passes (V-41/V-42 queued)
- [~] Placeholder asset audit complete (14 REPLACE_BEFORE_RELEASE markers catalogued)
- [~] License/asset manifest complete (ThirdPartyLicenses.md + ASSET_MANIFEST)
- [ ] Definition of Done satisfied

## GLOBAL QUALITY GATE
- [!] No Critical issues — C-1..C-12 (all static-review finds across every audit pass) fixed in source; engine verification pending on target machine
- [!] No unresolved High issues affecting core loop — closed: H-1/H-2/H-3/H-5 (Batch 1), B2-A/B2-B/B2-C (Batch 2), M-7 (Batch 6), H-6 grid battery + H-9 population counting + H-10 objective types + H-13 gamepad (final run). Still open: H-11 (craft output validation ignores AddItem result) + H-12 (remote-client RPC layer); both documented, neither blocks the SP/listen-server loop
- [!] Build passes — never compiled (verification queue §1)
- [ ] Game launches — unverified
- [~] Core loop playable — 23/23 loop stages implemented end-to-end in source (final run); compile + runtime + save round-trip pending on target machine
- [~] Save/load reliable — schema v3 full-coverage matrix (see GL53 audit §3); round-trip engine tests queued
- [~] Documentation matches implementation — final-run reports (GL53 audit / verification queue / build readiness / milestone) + this checklist synchronized
- [~] Git working tree clean after commit — final-run commits pushed at round close
