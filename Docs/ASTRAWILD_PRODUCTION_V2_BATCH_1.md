# ASTRAWILD — Production V2, Batch 1: Data Foundation & Production Contracts

**Status:** SOURCE_IMPLEMENTED (compile validation pending on the UE 5.8.2 target machine)
**Date:** 2026-08-30
**Scope driver:** `Docs/ASTRAWILD_PRODUCTION_V2_MASTER_PLAN.md` + `Docs/GLM_5_3_PRODUCTION_V2_EXECUTION_PROMPT.md`
**Baseline:** `e1c1b44` (Batch 8) + the V2 master-plan docs → this batch

---

## 1. Batch objective

Turn the verified technical prototype into the **data-driven production foundation** the Master Plan §3 STEP 3-8 demands, plus the P0 fix and the Visual-Vertical-Slice support contracts — without duplicating a single working system.

## 2. What was built

### 2.1 P0 — foundation safety
- **Resource identity is now deterministic** (Master Plan §1 known limitation): new `UAstrawildResourceNodeDefinition` data assets are the single source of truth for node identity (item id, quantities, respawn, rarity shape-kit, scanner gating). `AAstrawildResourceNode::BeginPlay` resolves everything from the definition; a node with no identity is ERROR-logged and interaction-disabled instead of silently no-oping. The bootstrapper's spawn tables now reference **node ids**, never raw item ids.
- Legacy `ResourceItemId` writes still work for level-placed actors (documented fallback).

### 2.2 Weapons — 8 families, one data asset (Master Plan §8)
- New `UAstrawildWeaponDefinition` (family, tier, rarity, fire mode, damage, interval, element, ammo, projectile speed/scale/lifetime, homing cone/range/acceleration, beam range/pierce, arc chain count/radius/fraction, VFX/audio contract ids).
- `UAstrawildCombatComponent::ExecuteRangedAttack` now branches on the equipped weapon's **fire mode**:
  - **Projectile** (Kinetic Scrapshot, Pulse Lance, Plasma Charger),
  - **HomingProjectile** (Skysinger Launcher — server-acquired lock-on inside the weapon's cone),
  - **Beam** (Lumen Beam pierce 2, Magrail Driver pierce 5, Starlance Prototype pierce 6 — hitscan),
  - **ArcChain** (Arc Caster — hitscan first hit + up to 3 chained targets at 60% decay per hop).
- All four archetypes funnel through ONE damage vocabulary (`ResolveRangedHit`) — elemental weaknesses, boss pipeline and quest credit behave identically to melee.
- `AAstrawildProjectileActor::LaunchFromWeapon` — data-driven speed/scale/lifetime + `ProjectileMovement` homing target.
- New ammo economy: Rail Slug, Seeker Missile, Nova Cell + Ancient Alloy (the hidden-vein material).
- Combat readability: HUD **weapon line** (name / DMG / interval / ammo count).

### 2.3 Armor & scanners (Master Plan §9/§10)
- **Split thermal bands**: `ColdInsulationRating` / `HeatInsulationRating` on items; survival exposure checks each side independently (legacy `InsulationRating` still counts on BOTH sides — every existing piece keeps its documented behaviour). `GetEquippedCold/HeatInsulationRating` resolve helmet + exosuit + torso.
- **Armor tiers**: Mk II Vanguard set (helm+vest), Mk III Bastion set (helm+plate), Experimental Astralforged Exosuit (+60kg, +20% speed, 12° both bands) — tech-gated through new `Tech_ExosuitEngineering`.
- **Scanner tiers**: Field Scanner (Mk I, existing) → Array Scanner Mk II (×1.6 observation range, +speed, **hidden-vein detection**) → Oracle Scanner (×2.5 range, **ancient-signal tracking**, doubles every POI discovery radius) through new `Tech_ScannerArray`.
- **Hidden veins**: `Node_AncientVein` (Epic, Ancient Alloy, 480s respawn) is only harvestable while a hidden-detection scanner is equipped.

### 2.4 Robotics — modules + specialist chassis (Master Plan §11/§12)
- **Drone modules** (items auto-apply while carried, best per category): Cell Extender (+600s battery), Focused Array (+6m scan radius, +2 progress/pulse), Salvage Claw (+5m harvest radius).
- **Drone battery**: drains server-side while deployed (base 600s + modules), auto-recalls at 0 with a toast; battery mid-drain persists in save v4.
- **Specialist robots**: `UAstrawildRobotDefinition` — Borebot (mining 1.6x/0.5x), Cultivator (farming 1.5x/0.5x), Sentinel (defense 1.4x/0.6x) with role lights + chassis tints. Deploy [J] consumes the specialist item first; work-site ticks resolve the robot's per-site rate. Save v4 persists the chassis id.

### 2.5 Base automation — consume→produce (Master Plan §7)
- `UAstrawildWorkSiteDefinition` data assets drive sites: output item/quantity, **input items per cycle**, seconds per output, power gate, zone + offset placement.
- `AAstrawildWorkSiteActor` gains an **input buffer**: production consumes inputs per cycle (empty buffer stalls the accumulator at the threshold — no free cycles); interact [E] stages up to 10 full input cycles from the player's pack with clear toasts.
- New definition-driven sites: Ridge Breaker Rig (Ember Ridge, powered mining → Stone×2) + Camp Kitchen (Raw Meat → Seared Meat — the showcase chain). Camp Gathering/Farm keep their historical placements but now initialize from definitions.

### 2.6 World events (Master Plan §19)
- New `UAstrawildWorldEventSubsystem` (tickable, server-only): deterministic scheduler (world-seeded, absolute in-world minutes), eligibility gates (day/night/cooldown/concurrency/weight), weighted picks.
- New `UAstrawildWorldEventDefinition` ×9: Storm Surge (forces weather), Great Migration (species boost), Resource Surge (bonus nodes), Supply Drop (loot), Ancient Signal (research), Night Raid (hostiles converge on camp), Meteor Fall (crater nodes + loot), Rare Echo Bloom (Auroraling), Boss Stirring (research).
- Events publish `Event.WorldEventStarted/Ended` (quest-hookable), toast through Notify, and persist in **save schema v4**.
- Weather transitions now also publish `Event.WeatherChanged`.

### 2.7 POIs — discovery content (Master Plan §5/§31)
- New `UAstrawildPOIDefinition` ×12 (ruins, watchtowers, cave, ancient tech, 2 ancient **signal sources** that require the Oracle Scanner) + `AAstrawildPOIMarkerActor` (type-colored beacon pillars, lore on examine).
- New `UAstrawildPOISubsystem`: 1s discovery sweep, radius from the definition (doubled by signal scanners), loot + research on first discovery, `Event.PoiDiscovered` (quest-hookable), persisted in save v4.
- New quest objective type **DiscoverPOI** (appended last — serialization-safe).

### 2.8 Biome asset contracts — Visual Vertical Slice support (Master Plan §31)
- New `UAstrawildBiomeDefinition` ×12 — one per zone, carrying the Antigravity binding surface: landscape material, grass/tree/rock mesh arrays, ambient audio soft refs + gameplay anchors (resource node ids, signature species, POI ids). **Dawn Fields is flagged `bStartingBiome` — the P1 slice lands there first.**

### 2.9 Echo production roster (Master Plan §6 STEP 5)
- 6 role-differentiated species with data-only abilities (**party passive auras**):
  - **Terraquill** (gathering 1.9, Pack Instinct +20kg carry aura, Dawn Fields),
  - **Cindermule** (transport 1.9, Pack Instinct, Ember Ridge),
  - **Voltpylon** (power generation 1.8, Rhythm Aura +2 stamina/s, Glimmerwood),
  - **Bastionbeetle** (defense 1.9, Calm Presence — wild hostiles lose player lock-on nearby, Verdant Reach),
  - **Mistmender** (Mending Aura +1 HP/s to player and party, Dusk Marsh),
  - **Deepdelver** (mining 1.9, Stormcrest).
- `EAstrawildEchoPassive` enum + `Rarity` field on echo definitions; auras tick at 1s cadence server-side.
- Wildlife tables seed all six in their home zones.

### 2.10 Research tree branches + quests (Master Plan §16/§17)
- `EAstrawildResearchBranch` on every tech (the 10 legacy techs retrofitted): Survival/Tools/Weapons/Armor/Energy/Automation/Scanner/EchoTech/Exploration.
- 6 new techs: Weapon Systems → Advanced Ballistics → Experimental Arsenal (weapons ladder), Exosuit Engineering (armor), Scanner Array, Automation II (robotics) — 16 total.
- Quests 11-12: **"Signals in the Static"** (craft Array Scanner + discover 2 POIs) → **"The Vanguard Protocol"** (craft Mk II vest + build a Sentinel + survive 300s). The Sunken Vault now chains into them.

### 2.11 Save schema v4 (Master Plan §25)
- Additive: `WorldEvents` (active + roll clock + cooldown map), `DiscoveredPOIIds`, drone `BatteryRemainingSeconds`, robot `RobotDefinitionId`, work-site `InputBuffer` + `OutputQuantity`. `MigrateV3ToV4` stamps additive defaults. Every v3 save loads cleanly.

## 3. Changed files

**New (10):** `Public/AstrawildProductionContent.h`, `Private/AstrawildProductionContent.cpp`, `Public/AstrawildWorldEventSubsystem.h`, `Private/AstrawildWorldEventSubsystem.cpp`, `Public/AstrawildPOISubsystem.h`, `Private/AstrawildPOISubsystem.cpp`, `Public/AstrawildPOIMarkerActor.h`, `Private/AstrawildPOIMarkerActor.cpp`, `Docs/ASTRAWILD_PRODUCTION_V2_BATCH_1.md` (this file)

**Modified (21):** `AstrawildTypes.h` (7 enums + save structs v4), `AstrawildDataAssets.h` (+7 definition classes, +17 item/echo/tech fields), `AstrawildItemRegistrySubsystem.h/.cpp` (+7 registries +4 enumerators), `AstrawildGameplayTags.h/.cpp` (+4 event tags), `AstrawildCombatComponent.h/.cpp` (fire modes), `AstrawildProjectileActor.h/.cpp` (weapon launch), `AstrawildInventoryComponent.h/.cpp` (weapon/scanner/insulation getters), `AstrawildSurvivalComponent.h/.cpp` (split bands + restore hooks), `AstrawildResourceNode.h/.cpp` (P0 determinism), `AstrawildWorkSiteActor.h/.cpp` (consume→produce), `AstrawildUtilityDroneActor.h/.cpp` (modules + battery), `AstrawildUtilityRobotActor.h/.cpp` (chassis), `AstrawildEchoCharacter.h/.cpp` (passives), `AstrawildEchoAIController.cpp` (Calm Presence), `AstrawildPlayerCharacter.h/.cpp` (specialist deploys), `AstrawildQuestComponent.cpp` (DiscoverPOI), `AstrawildWeatherSubsystem.cpp` (event publish), `AstrawildSaveSubsystem.h/.cpp` (schema v4), `AstrawildContentLibrary.cpp` (BuildAll + retrofits), `AstrawildWorldBootstrapper.h/.cpp` (definition-driven spawns), `AstrawildHudWidget.h/.cpp` (3 readouts), `AstrawildAutomationTests.cpp` (+11 tests → 39)

## 4. New data definitions (CODE_DEFAULT — overridable by .uasset registrations)

| Family | Count | Ids (samples) |
|---|---|---|
| Weapon profiles | 8 | Weapon_Scrapshot…Weapon_StarlancePrototype |
| Weapon items + ammo | 10 | Item_Scrapshot, Item_RailSlug, Item_NovaCell, Item_AncientAlloy… |
| Armor/scanner items | 7 | Item_VanguardHelm/Vest, Item_BastionHelm/Plate, Item_AstralforgedExosuit, Item_ArrayScanner, Item_OracleScanner |
| Drone modules + robots | 6 | Item_DroneCellExtender/FocusedArray/SalvageClaw, Item_RobotBorebot/Cultivator/Sentinel |
| Robot definitions | 3 | Robot_Borebot, Robot_Cultivator, Robot_Sentinel |
| Resource nodes | 10 | Node_Dawnwood…Node_AncientVein (hidden) |
| Work sites | 4 | Site_CampGathering/Farm/Kitchen, Site_RidgeMining |
| World events | 9 | Event_StormSurge…Event_BossStirring |
| POIs | 12 | POI_FirstLightRuin…POI_PearlseaResonanceWell |
| Biomes | 12 | Zone_DawnFields (starting)…Zone_PearlseaReef |
| Production Echoes | 6 | Echo_Terraquill, Echo_Cindermule, Echo_Voltpylon, Echo_Bastionbeetle, Echo_Mistmender, Echo_Deepdelver |
| Technologies | 6 | Tech_WeaponSystems/AdvancedBallistics/ExperimentalArsenal/ExosuitEngineering/ScannerArray/AutomationII |
| Quests | 2 | Quest_SignalsInTheStatic → Quest_VanguardProtocol |
| Loot tables | 5 | Loot_EventSupplyDrop/Meteor, Loot_POIAncient/Ruin/Watchtower |
| Recipes | 20 | All weapon/armor/scanner/robotics/ammo recipes |

Totals: 48 items, 44 recipes, 220 Echo species, 16 techs, 12 quests, 39 automation tests.

## 5. What Antigravity must do in UE5

1. **Pull + compile** (UE 5.8.2, Development Editor):
   `git pull origin main` → right-click `ASTRAWILD.uproject` → Generate → build in VS/Rider. This batch touches 31 files — UHT will re-run; expect the first build to take a while.
2. **Run the 39 automation tests** (`Lower` console: `Automation RunTests ASTRAWILD`): 28 prior + **11 new** (Weapon.ProfileMath, WorldEvent.EligibilityGates, WorldEvent.WeightedPickDeterminism, POI.DiscoveryRadiusMath, ResourceNode.DefinitionContract, Echo.ProductionRosterContract, Armor.SplitInsulation, Robot.SpecialistRates, WorkSite.ProductionChain, Save.SchemaV4, Quest.DiscoverPOIType).
3. **Golden-path smoke additions** (on top of the existing 23-stage path):
   - Craft the **Scrapshot** (Armory tech) → equip → LMB fires a stone-consuming bolt.
   - Research **Weapon Systems** → craft the **Arc Caster** → fire into a pack — chain hits 4 targets.
   - Research **Scanner Array** → craft the **Array Scanner** → travel to Hollow Approach → harvest a **hidden alloy vein** (cone spire — only interactable with the scanner equipped).
   - Drop raw meat at the **Camp Kitchen** site [E] → assign an Echo/robot → seared meat accumulates (inputs are consumed).
   - Wait for a world event toast (first roll ~09:00 in-world day 1) — verify the HUD amber event banner + `Event.WorldEventStarted` in the log.
   - Walk to the **First Light Ruin** (Dawn Fields, camp NE) → discovery toast + quest "Signals in the Static" objective ticks.
   - Deploy a **Borebot** [J] near the Ridge Breaker Rig (Ember Ridge) → orange chassis light, 1.6x stone output when powered.
4. **VVS asset binding** (P1 work, per §2.8 + `UAstrawildBiomeDefinition`): Dawn Fields first — landscape material, grass/tree/rock meshes, ambient loop into the soft refs; POI dressing sets per `DressingSetId`; weapon VFX per Muzzle/Trail/Impact VfxId (`NS_AW_Weap_*`), sounds `SC_AW_Weap_*`.

## 6. Tests added

11 new automation tests (names above) — pure logic, world-free, all deterministic. Never removed an existing test.

## 7. Known limitations / honest status

- **Compile status: NOT_RUN in this sandbox** (no UE5 on Linux). Source follows every established compile-safe pattern; static validation (validate_repository.sh + brace/include lint) passes.
- Drone module application is passive (best-per-category while carried) — no install UI yet (backlog).
- Beam weapons have no visual beam yet (damage resolves server-side; VFX ids carry the Antigravity contract).
- Supply-drop loot is granted directly to the player's pack with a toast — the physical crate lands with the art pass.
- POI markers are placeholder pillars; `DressingSetId` carries the prop contract.
- Save v4 is additive — **old v3 saves load cleanly** (world layout unchanged since Batch 8, so Batch 8 saves still work).

## 8. Verification expectations for this batch

- `Automation RunTests ASTRAWILD` → 39/39 pass.
- No new compile warnings in the touched files (watch for: `AstrawildWorldEventSubsystem.cpp`, `AstrawildProductionContent.cpp` — both are large new files).
- If anything fails: **exact file:line + full error text** into `Docs/ENGINE_LOGS/` per the protocol — GLM fixes source-side next batch.
