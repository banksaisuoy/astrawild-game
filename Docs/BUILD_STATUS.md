# ASTRAWILD — Build Status

## Status

- Overall: `PARTIAL` — full vertical-slice foundation implemented in C++ (**source-complete, never compiled**)
- Last updated: 2026-08-30
- Branch: `main` (latest commits: content wave 2 + UMG crafting hooks, then content wave 3 + docs sync)
- Latest change: **CODE_DEFAULT wave 3** — equipment progression (weapon + shield slots, armory recipes,
  equip-best on X), 2 camp NPCs (Warden Maren, Trader Tam), loot tables (dungeon boss + vendor starter),
  equipment save persistence — plus a full docs sync (this round, Task 2-b)
- Codebase: **86 C++ files (42 `.cpp` + 44 `.h`), 13,398 LOC** in `Source/AstrawildCore` (single module)

## Environment

- Unreal Engine: **Not run in sandbox environments; target is 5.8** (EngineAssociation in `ASTRAWILD.uproject`)
- Compiler: Not run in sandbox environments (Linux sandbox has no MSVC/UE toolchain); target build is Windows + VS2022
- OS: Repository validation run in Ubuntu sandbox; target build is Windows
- CPU/GPU/RAM/Storage: Not measured

## Compile

- Target: `ASTRAWILDEditor Win64 Development` — pending Antigravity (user machine)
- Result: `NOT_RUN` (unchanged — sandbox has no UE5; honest status per Definition of Done)
- Errors: Not measured; Unreal Editor unavailable in sandbox environments
- Warnings: Not measured
- Build duration: Not measured
- Validation steps for the target machine: `Docs/ASTRAWILD_TEST_PLAN.md` §4

Static repository validation passed with `Scripts/validate_repository.sh`.

## Changes in this round (2026-08-30 — content wave 3: equipment progression, NPCs, loot tables + docs sync)

### Content expansion (CODE_DEFAULT wave 3)

| Content | Entries |
|---|---|
| Items 16 → **19** | `Item_DawnwoodClub` (Equipment, ATK +6, 2.5 kg), `Item_StonehideShield` (Equipment, BlockMitigation 0.65, 4.0 kg), `Item_CrystalBlade` (Equipment, ATK +14, 3.0 kg) — all stack 1 |
| Recipes 7 → **10** | `Recipe_DawnwoodClub` (3 Wood + 1 Fiber, 3 s, no tech), `Recipe_StonehideShield` (3 Stone + 2 Wood + 1 Fiber, 5 s, `Tech_Armory`), `Recipe_CrystalBlade` (2 Crystal Shard + 2 Plank + 1 Ember Ash, 8 s, `Tech_Armory`) — all workbench |
| Technologies 5 → **6** | `Tech_Armory` (8 RP, Primitive, prereq `Tech_BasicCrafting`; unlocks shield + blade recipes) |
| Loot tables 0 → **2** | `Loot_DungeonBoss` (Ancient Core ×1 + Crystal Shard ×2 + Ember Ash ×2, bonus roll 0.75 — wired to the Hollow Underlight boss room), `Loot_VendorStarter` (Berry ×3 + Dew Flask ×1 + Bandage ×2, no bonus roll — Trader Tam's stock hook) |
| NPCs 0 → **2** | `NPC_WardenMaren` (offers `Quest_FirstLight`; spawned at camp (630, −630, 100)), `NPC_VendorTam` (`ShopLootTableId = Loot_VendorStarter`; spawned at (−630, −630, 100)) |

### Systems (wave 3 code changes — already implemented by the lead, verified by this round)

| Area | Change |
|---|---|
| Registry | `+RegisterLootTable/FindLootTable`, `+RegisterNPC/FindNPCDefinition` (new `LootTables` + `NPCDefinitions` maps on `UAstrawildItemRegistrySubsystem`) |
| Inventory | Two equipment slots: `EquippedItemId` (weapon) + **`EquippedShieldItemId`** (both replicated); `EquipItem` auto-routes by stat (AttackPower > 0 → weapon, BlockMitigation > 0 → shield); `+OnEquipmentChanged` delegate; `+GetEquippedWeaponAttackPower/GetEquippedShieldMitigation` (BlueprintPure) |
| Combat | `BlockMitigation` renamed **`UnarmedBlockMitigation`** (default 0.65 → **0.45**); `+GetEffectiveBlockMitigation()` (shield replaces unarmed baseline, clamped 0..0.8); `+GetEquippedWeaponAttackPower()`; `+GetOutgoingAttackDamage(bHeavy)` = base + weapon flat ATK (used by `ExecuteAttack`) |
| Save | v2 payload + `EquippedWeaponId` + `EquippedShieldId` (additive FNames, `NAME_None` defaults — **schema stays v2**, old saves load fine); load re-equips only when `HasItem` passes |
| Dungeons | `GrantClearReward` grants `Template.ClearLootTableId` to the first player (guaranteed drops + one bonus roll); boss room template sets `ClearLootTableId = Loot_DungeonBoss` |
| Input | **X** = equip-best (strongest owned weapon + shield) — 17 actions / 17 keys; log line fixed to "17 actions" |
| Cheats | `+AW.EquipItem <ItemId>` — **13 commands** (warns when the item is missing or not equipment) |
| HUD | `+EquipmentText` right-bottom readout (anchor 0.98/0.90, amber, 300×20, font 14): `Weapon: <name> (+N) | Shield: <name>` — 12 widgets total |
| Tests | `+ASTRAWILD.Equipment.ProgressionMath` — **9 automation tests** (club light 25+6=31, blade heavy 60+14=74, unarmed block 55 %, shielded 35 %) |

### Docs sync (this round — Task 2-b, docs only)

Updated 13 docs to match the wave 3 code (every value re-verified against source):
`ASTRAWILD_ASSET_MANIFEST` (19 items/10 recipes/6 techs + loot-table & NPC sections) ·
`ASTRAWILD_INPUT_REFERENCE` (17 keys → 17 actions, X row, `AW.EquipItem`, 13 commands) ·
`ASTRAWILD_COMBAT_SYSTEM` (§2.3 equipment integration, §4 block rework) ·
`ASTRAWILD_SAVE_SYSTEM` (v2 payload + additive-no-bump decision) ·
`ASTRAWILD_MULTIPLAYER` (25 replicated props / 9 classes — corrected a stale 20/7 count that missed
the dungeon round's 4 props) · `ASTRAWILD_UI_ARCHITECTURE` (EquipmentText, 12 widgets, 17 actions) ·
`ASTRAWILD_TEST_PLAN` (9 tests + T-1..T-6 fix-status re-check) · `ASTRAWILD_RESEARCH_SYSTEM` (6-node
tree, quest totals) · `ASTRAWILD_GAMEPLAY_SYSTEMS` (30-row system inventory) ·
`ASTRAWILD_CRAFTING_SYSTEM` (10 recipes) · `ASTRAWILD_QUEST_SYSTEM` (quest 6 + camp NPCs + ObserveEcho
wiring fix status) · `BUILD_STATUS` (this file) · `ASTRAWILD_PRODUCTION_ROADMAP_V2` (STEP 28 note).

## Changes in the previous round (2026-08-30 — content wave 2 + UMG crafting hooks)

### Content expansion (CODE_DEFAULT wave 2 — husbandry economy)

| Content | Entries |
|---|---|
| Items 12 → **16** | `Item_Dawnbloom`, `Item_EmberAsh`, `Item_FeedMix`, `Item_HerbalSalve` |
| Recipes 5 → **7** | `Recipe_FeedMix` (campfire), `Recipe_HerbalSalve` (workbench) |
| Echo species 5 → **7** | `Echo_Sprigling` (Flora support, Social, herding, Farming 1.7, loot: Dawnbloom), `Echo_Emberfang` (new Ember element, crepuscular predator, loot: Ember Ash) |
| Buildings 9 → **10** | `Building_FeedTrough` (Farm, Tech_Husbandry) |
| Technologies 4 → **5** | `Tech_Husbandry` (10 RP, prereq Cooking) |
| Quests 5 → **6** | `Quest_ShepherdsDawn` chained after Dawn Guard |
| Elements | new `Ember` element on `EAstrawildElementType` (additive) |
| Ecosystem | Emberfang→Sprigling/Voltling + Gloomfang→Sprigling chains; Sprigling herding |
| World spawn | wild rotation 4 species; hostiles alternate Gloomfang/Emberfang |

### UMG crafting screen contract (formerly "future UMG contract")

- `UAstrawildCraftingScreenWidget` (Abstract, Blueprintable): base class that binds the owning pawn's
  crafting component and forwards everything to `BP_OnRecipesAvailable/BP_OnCraftStarted/BP_OnCraftProgress/
  BP_OnCraftCompleted/BP_OnCraftCancelled` events — UMG assets stay pure view code.
- `UAstrawildCraftingComponent` additions: `OnCraftStarted`/`OnCraftCancelled` delegates,
  `ServerRequestCraft`/`ServerRequestCancelCraft` Server RPCs (client-safe), `CancelActiveCraft()` with
  ingredient refund, `GetCraftingProgress()`, `GetCraftTimeRemaining()`, `GetTechUnlockedRecipes()`,
  `GetNearbyStationIds()`.

## Changes in the previous round (2026-08-29, DOCS-1 — docs suite)

The C++ for all systems below landed in commits `3872c7e`→`7775668`; this round adds the complete
documentation suite (23 new files in `Docs/`, see "New systems documented" table) and this status refresh.
No `Source/` files were modified by DOCS-1.

### New systems (implemented in C++ this foundation round, compile pending)

| System | Key classes | Doc |
|---|---|---|
| Logging (8 categories) | `AstrawildLog` | — |
| Native gameplay tags (77) | `AstrawildGameplayTags` | `ASTRAWILD_GAMEPLAY_TAGS.md` |
| Types v2 + 8 data-asset definition classes | `AstrawildTypes`, `AstrawildDataAssets` | `ASTRAWILD_UE5_ARCHITECTURE_V2.md` |
| Replicated world state | `AstrawildGameState` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Day/night (24-min day) | `AstrawildTimeSubsystem` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Weather (8 states, weighted) | `AstrawildWeatherSubsystem` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Event bus | `AstrawildEventBusSubsystem` | `ASTRAWILD_UE5_ARCHITECTURE_V2.md` |
| Ecosystem LOD + population | `AstrawildEcosystemSubsystem` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Procedural Dawn Fields (zero-asset arena) | `AstrawildWorldBootstrapper` | `ASTRAWILD_WORLD_SYSTEM.md` |
| Survival vitals + status effects + respawn | `AstrawildSurvivalComponent` | `ASTRAWILD_SURVIVAL_SYSTEM.md` |
| Action combat (light/heavy/dodge/block/elemental) | `AstrawildCombatComponent` | `ASTRAWILD_COMBAT_SYSTEM.md` |
| Echo v2 (needs/personality/growth/commands) | `AstrawildEchoCharacter` | `ASTRAWILD_CREATURE_SYSTEM.md` |
| Echo AI (perception + 16-state machine) | `AstrawildEchoAIController` | `ASTRAWILD_AI_ARCHITECTURE.md` |
| Capture pipeline + field journal | `AstrawildCaptureComponent`, `AstrawildJournalSubsystem` | `ASTRAWILD_CREATURE_SYSTEM.md` |
| Echo roster/party (max 3) | `AstrawildEchoRosterSubsystem` | `ASTRAWILD_CREATURE_SYSTEM.md` |
| Echo work sites | `AstrawildWorkSiteActor` | `ASTRAWILD_CREATURE_SYSTEM.md` |
| Inventory v2 (weight 120 kg, equipment slots — weapon + shield) | `AstrawildInventoryComponent` | `ASTRAWILD_GAMEPLAY_SYSTEMS.md` |
| Item registry + CODE_DEFAULT content library | `AstrawildItemRegistrySubsystem`, `AstrawildContentLibrary` | `ASTRAWILD_ASSET_PIPELINE.md` |
| Timed crafting + stations | `AstrawildCraftingComponent`, `AstrawildCraftingStationActor` | `ASTRAWILD_CRAFTING_SYSTEM.md` |
| Building placement + actors + power grid | `AstrawildBuildingComponent`, `AstrawildBuildingActor`, `AstrawildPowerSubsystem` | `ASTRAWILD_BUILDING_SYSTEM.md` |
| Research / tech tree | `AstrawildResearchSubsystem` | `ASTRAWILD_RESEARCH_SYSTEM.md` |
| Quests (event-driven, 6-quest chain) | `AstrawildQuestComponent` | `ASTRAWILD_QUEST_SYSTEM.md` |
| Save schema v2 (checksum, migration, autosave) | `AstrawildSaveSubsystem` | `ASTRAWILD_SAVE_SYSTEM.md` |
| Pure-C++ HUD + runtime Enhanced Input | `AstrawildHudWidget`, `AstrawildPlayerCharacter` | `ASTRAWILD_UI_ARCHITECTURE.md` |
| Cheat manager (13 commands) | `AstrawildCheatManager` | `ASTRAWILD_INPUT_REFERENCE.md` |
| NPC base (architecture-ready) | `AstrawildNPCCharacter` | `ASTRAWILD_QUEST_SYSTEM.md` |
| Automation tests (9) | `AstrawildAutomationTests.cpp` | `ASTRAWILD_TEST_PLAN.md` |
| Multiplayer authority/replication (25 props, 5 RPCs) | across classes | `ASTRAWILD_MULTIPLAYER.md` |

### Docs created this round (DOCS-1)

`ASTRAWILD_UE5_ARCHITECTURE_V2` · `ASTRAWILD_GAMEPLAY_SYSTEMS` · `ASTRAWILD_CREATURE_SYSTEM` ·
`ASTRAWILD_AI_ARCHITECTURE` · `ASTRAWILD_COMBAT_SYSTEM` · `ASTRAWILD_SURVIVAL_SYSTEM` ·
`ASTRAWILD_BUILDING_SYSTEM` · `ASTRAWILD_CRAFTING_SYSTEM` · `ASTRAWILD_RESEARCH_SYSTEM` ·
`ASTRAWILD_WORLD_SYSTEM` · `ASTRAWILD_QUEST_SYSTEM` · `ASTRAWILD_SAVE_SYSTEM` · `ASTRAWILD_MULTIPLAYER` ·
`ASTRAWILD_UI_ARCHITECTURE` · `ASTRAWILD_GAMEPLAY_TAGS` · `ASTRAWILD_PERFORMANCE` ·
`ASTRAWILD_TEST_PLAN` · `ASTRAWILD_ASSET_PIPELINE` · `ASTRAWILD_PRODUCTION_ROADMAP_V2` ·
`ASTRAWILD_DEFINITION_OF_DONE` · `ASTRAWILD_ASSUMPTIONS` · `ASTRAWILD_ASSET_MANIFEST` ·
`ASTRAWILD_INPUT_REFERENCE`

## Unreal assets created by Antigravity

| Asset | Path | Status | Notes |
|---|---|---|---|
| Prototype map | — | NOT_CREATED | Not required for PIE: `AstrawildWorldBootstrapper` builds a playable zero-asset arena |
| Input assets (IMC/IA) | — | NOT_CREATED | Optional: runtime input defaults are built in code |
| Echo/Item/Building data assets | — | NOT_CREATED | CODE_DEFAULT content registered in memory; replacement plan in `ASTRAWILD_ASSET_PIPELINE.md` |
| UI | — | NOT_CREATED | Pure-C++ HUD requires none |
| Echo meshes / icons | — | NOT_CREATED | Engine basic-shape placeholders; checklist in `ASTRAWILD_ASSET_MANIFEST.md` |

## Playtest

| Test | Result | Notes |
|---|---|---|
| Open project | NOT_RUN | Awaiting target-machine compile (Test Plan §4) |
| Compile Development Editor | NOT_RUN | **Blocking step for everything below** |
| Automation suite (9 tests) | NOT_RUN | Run via Session Frontend, filter `ASTRAWILD` |
| Player movement/camera | NOT_RUN | Manual flow step 4 |
| Interaction | NOT_RUN | Step 5 |
| Harvest resource | NOT_RUN | Step 10 |
| Capture Echo | NOT_RUN | Step 8 |
| Craft recipe | NOT_RUN | Step 11 (station interact) |
| Place building | NOT_RUN | Step 12 |
| Activate rest point | NOT_RUN | Step 17 area |
| Save snapshot (F5) | NOT_RUN | Steps 14–16 |
| Load snapshot (F9) | NOT_RUN | Step 16 |
| Full first-playable flow (17 steps) | NOT_RUN | `ASTRAWILD_TEST_PLAN.md` §2 |

## Known issues

| Severity | Issue | File/asset | Reproduction | Owner/next action |
|---|---|---|---|---|
| **Blocker** | Repository has never been compiled | `Source/AstrawildCore/` | Any build attempt | Antigravity: Test Plan §4, then fix-forward |
| High | T-1 ObserveEcho quest wiring | `AstrawildQuestComponent.cpp` / `AstrawildJournalSubsystem.cpp` | Start Quest_FirstEcho and observe a Lumewisp | **Fix in code** (journal publishes `Event.EchoObserved` at the 25 % scan milestone) — verify at playtest |
| High | T-2 AI think loop / LOD interval | `AstrawildEchoAIController.cpp:Think` | Spawn 10+ Echoes, profile | **Fix in code** (`SetTimer` with the LOD interval) — verify via Insights |
| Medium | T-4 cold/heat damage reachability | `AstrawildWeatherSubsystem` / Survival | `AW.SetWeather cold` and wait | **Fix in code** (Cold −17 °C offset → felt 3 °C < 4 °C threshold) — verify at playtest |
| Medium | T-5 consume keybind | `AstrawildPlayerCharacter.cpp` | Have berries, press **G** | **Fix in code** (G = `SmartConsume`) — verify at playtest |
| Medium | T-6 journal per-frame iteration | `AstrawildJournalSubsystem.cpp` | Insights capture | **Fix in code** (throttled observation sweep) — verify via Insights |
| Low | T-3 HUD weather label hard-codes 20 °C | `AstrawildHudWidget.cpp` | Look at HUD | Cosmetic fix (still present) |
| Low | Log-line count drift | PlayerCharacter.cpp / ContentLibrary.cpp | Read logs | Resolved for now: log says "17 actions" / "19 items, … 2 loot tables, 2 NPCs" and matches the code |
| Low | NPC vendor purchase logic | `AstrawildNPCCharacter` | Talk to Trader Tam | `ShopLootTableId` is a definition-level hook only — purchase flow NOT IMPLEMENTED (future round) |

## Handoff to Antigravity

The C++ core (single module `AstrawildCore`, **~13.4k LOC, 86 source files**), the zero-asset playability
layer (procedural world + runtime input + C++ HUD), save schema v2 (with wave 3 equipment persistence),
the CODE_DEFAULT content set (19 items / 10 recipes / 7 species / 10 buildings / 6 techs / 6 quests /
2 loot tables / 2 NPCs), the documentation suite, the test plan, and the asset manifest/replacement
pipeline are all in the repository. Antigravity must: **pull, generate project files, compile
`ASTRAWILDEditor Win64 Development`, run the 9 automation tests, execute the 17-step first-playable
checklist, and fill this report with real results.** Do not mark `COMPLETE` until Compile, the automation
suite, the core-loop Playtest, and Save/Load have all passed (see `Docs/ASTRAWILD_DEFINITION_OF_DONE.md`).
