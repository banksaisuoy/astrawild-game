# ASTRAWILD — Asset Manifest

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-30** (wave 9 sync — Batch 5 ecosystem/tech content + Batch 6 dungeon/boss content; previously synced at wave 6)

Every piece of CODE_DEFAULT content in the game, with its replacement requirement. This is the
**REPLACE_BEFORE_RELEASE checklist** for the art/data pass (see `ASTRAWILD_ASSET_PIPELINE.md` for the
override mechanism: register a data asset with the **same id**).

Totals: **28 items · 18 recipes · 10 Echo species · 13 buildings · 10 technologies · 7 quests ·
2 loot tables · 2 NPCs** — all `Status = CODE_DEFAULT`, all `Replacement = REQUIRED` (data asset +
art where noted).

> **Count note:** the original directive and the early ContentLibrary log line said "10 items", but
> `BuildItems()` registered **12 item definitions** (verified by counting `RegisterItem` calls); the
> content wave 2 expansion (2026-08-30) added 4 more (16 total), wave 3 added 3 equipment items (19),
> wave 5 Batch 3 added 3 armor items (22 — previously missing from this manifest, added by the wave-6
> sync), and wave 6 Batch 4 adds the Dawn Shard currency (23). The log string now matches the code.
> (Same class of off-by-N as the "16 actions" log — see Assumptions #21.)

---

## 1. Items (28)

| Item | Id | Category | Weight | Stack | Key values | Art requirement |
|---|---|---|---|---|---|---|
| Dawnwood | `Item_Wood` | Material | 0.5 | 200 | — | Icon |
| Fieldstone | `Item_Stone` | Material | 0.8 | 200 | — | Icon |
| Sunfiber | `Item_Fiber` | Material | 0.1 | 200 | — | Icon |
| Dawnwood Plank | `Item_WoodPlank` | Material | 1.0 | 100 | — | Icon |
| Dawn Crystal Shard | `Item_CrystalShard` | Material | 0.3 | 100 | — | Icon |
| Glimmer Berry | `Item_Berry` | Consumable | 0.2 | 50 | Food 15, Water 5, EchoFeed 6; **vendor price 2** | Icon |
| Raw Echo Meat | `Item_RawMeat` | Consumable | 0.7 | 30 | Food 8, EchoFeed 5 | Icon |
| Seared Meat | `Item_CookedMeat` | Consumable | 0.6 | 30 | Food 30 | Icon |
| Dew Flask | `Item_WaterFlask` | Consumable | 0.9 | 20 | Water 40; **vendor price 2** | Icon |
| Sunfiber Bandage | `Item_Bandage` | Consumable | 0.2 | 30 | Heal 40; **vendor price 3** | Icon |
| Echo Resonator | `Item_Resonator` | CreatureItem | 0.4 | 20 | consumed per capture attempt; **vendor price 6** | Icon + (optionally) world model |
| Ancient Core | `Item_AncientCore` | QuestItem | 1.0 | 10 | quest reward (Dawn Guard) | Icon |
| Dawnbloom Petal | `Item_Dawnbloom` | Material | 0.1 | 100 | Sprigling loot; salve ingredient | Icon |
| Ember Ash | `Item_EmberAsh` | Material | 0.2 | 50 | Emberfang loot | Icon |
| Echo Feed Mix | `Item_FeedMix` | Consumable | 0.3 | 40 | Food 5, EchoFeed 14 (husbandry) | Icon |
| Dawnbloom Salve | `Item_HerbalSalve` | Consumable | 0.25 | 20 | Heal 70; **vendor price 4** | Icon |
| Dawnwood Club | `Item_DawnwoodClub` | Equipment | 2.5 | 1 | AttackPower +6 (wave 3) | Icon + (optionally) world model |
| Stonehide Shield | `Item_StonehideShield` | Equipment | 4.0 | 1 | BlockMitigation 0.65 (wave 3) | Icon + (optionally) world model |
| Dawn Crystal Blade | `Item_CrystalBlade` | Equipment | 3.0 | 1 | AttackPower +14 (wave 3) | Icon + (optionally) world model |
| Fiberweave Vest | `Item_FiberWeaveVest` | Equipment | 3.0 | 1 | ArmorRating 20 → ≈16.7 % reduction (wave 5 — Batch 3) | Icon + (optionally) world model |
| Emberhide Jacket | `Item_EmberhideJacket` | Equipment | 5.0 | 1 | ArmorRating 45 → ≈31.0 % reduction (wave 5 — Batch 3) | Icon + (optionally) world model |
| Crystalplate Cuirass | `Item_CrystalplateCuirass` | Equipment | 8.0 | 1 | ArmorRating 80 → ≈44.4 % reduction (wave 5 — Batch 3) | Icon + (optionally) world model |
| Dawn Shard | `Item_DawnShard` | Material | 0.1 | 200 | **Vendor currency** (wave 6 — Batch 4); `VendorPrice 0` — cannot be bought/sold with itself | Icon |
| Frostbloom | `Item_Frostbloom` | Material | 0.2 | 50 | Rimefang loot; **vendor price 3** (wave 8 — Batch 5) | Icon |
| Volt Core | `Item_VoltCore` | Material | 0.4 | 30 | Voltmaw loot; **vendor price 4** (wave 8 — Batch 5) | Icon |
| Hearth Broth | `Item_WarmBroth` | Consumable | 0.6 | 20 | Food 18, Water 12, Heal 10; **vendor price 3** (wave 8 — Batch 5) | Icon |
| Dawnfield Fertilizer | `Item_Fertilizer` | Material | 0.3 | 50 | Agriculture crafted good; **vendor price 2** (wave 8 — Batch 5) | Icon |
| Ancient Resonator | `Item_AncientResonator` | Equipment | 3.0 | 1 | AttackPower +18, **Light element** (the Underlight Warden's counter); **vendor price 8** (wave 9 — Batch 6) | Icon + (optionally) world model |

---

## 2. Recipes (18)

| Recipe | Id | Inputs | Output | Time | Tech | Station | Replacement |
|---|---|---|---|---|---|---|---|
| Echo Resonator | `Recipe_Resonator` | Stone ×2, Fiber ×1 | Resonator ×1 | 3 s | — | none | Data asset (values only) |
| Sunfiber Bandage | `Recipe_Bandage` | Fiber ×2 | Bandage ×1 | 2 s | — | none | Data asset |
| Dawnwood Plank | `Recipe_WoodPlank` | Wood ×2 | Plank ×1 | 2 s | — | `Station_Workbench` | Data asset |
| Seared Meat | `Recipe_CookedMeat` | Raw Meat ×1 | Cooked Meat ×1 | 5 s | `Tech_Cooking` | `Station_Campfire` | Data asset |
| Dew Flask | `Recipe_WaterFlask` | Fiber ×2, Crystal Shard ×1 | Flask ×1 | 4 s | — | `Station_Workbench` | Data asset |
| Echo Feed Mix | `Recipe_FeedMix` | Berry ×2, Fiber ×1 | Feed Mix ×1 | 4 s | `Tech_Husbandry` | `Station_Campfire` | Data asset |
| Dawnbloom Salve | `Recipe_HerbalSalve` | Dawnbloom ×2, Fiber ×1 | Salve ×1 | 4 s | `Tech_Husbandry` | `Station_Workbench` | Data asset |
| Dawnwood Club | `Recipe_DawnwoodClub` | Wood ×3, Fiber ×1 | Club ×1 | 3 s | — | `Station_Workbench` | Data asset |
| Stonehide Shield | `Recipe_StonehideShield` | Stone ×3, Wood ×2, Fiber ×1 | Shield ×1 | 5 s | `Tech_Armory` | `Station_Workbench` | Data asset |
| Dawn Crystal Blade | `Recipe_CrystalBlade` | Crystal Shard ×2, Wood Plank ×2, Ember Ash ×1 | Blade ×1 | 8 s | `Tech_Armory` | `Station_Workbench` | Data asset |
| Fiberweave Vest | `Recipe_FiberWeaveVest` | Fiber ×4, Wood ×1 | Vest ×1 | 4 s | `Tech_Armory` | `Station_Workbench` | Data asset (wave 5 — Batch 3) |
| Emberhide Jacket | `Recipe_EmberhideJacket` | Ember Ash ×2, Wood Plank ×2, Fiber ×2 | Jacket ×1 | 6 s | `Tech_Armory` | `Station_Workbench` | Data asset (wave 5 — Batch 3) |
| Crystalplate Cuirass | `Recipe_CrystalplateCuirass` | Crystal Shard ×3, Wood Plank ×3, Ember Ash ×2 | Cuirass ×1 | 9 s | `Tech_Armory` | `Station_Workbench` | Data asset (wave 5 — Batch 3) |
| Bulk Dawnwood Planks | `Recipe_PlankBatch` | Wood ×10 | Plank ×6 | 6 s | `Tech_Mechanics` | `Station_Workbench` | Data asset (wave 8 — Batch 5) |
| Resonator Batch | `Recipe_ResonatorBatch` | Crystal Shard ×2, Volt Core ×1, Fiber ×2 | Resonator ×3 | 8 s | `Tech_Mechanics` | `Station_Workbench` | Data asset (wave 8 — Batch 5) |
| Hearth Broth | `Recipe_WarmBroth` | Raw Meat ×1, Frostbloom ×1, Berry ×1 | Hearth Broth ×2 | 5 s | `Tech_Thermal` | `Station_Campfire` | Data asset (wave 8 — Batch 5) |
| Dawnfield Fertilizer | `Recipe_Fertilizer` | Dawnbloom ×2, Fiber ×1, Raw Meat ×1 | Fertilizer ×3 | 4 s | `Tech_Agriculture` | `Station_Campfire` | Data asset (wave 8 — Batch 5) |
| Ancient Resonator | `Recipe_AncientResonator` | Ancient Core ×1, Crystal Shard ×2, Resonator ×1 | Ancient Resonator ×1 | 10 s | `Tech_AncientResonance` | `Station_Workbench` | Data asset (wave 9 — Batch 6) |

---

## 3. Echo Species (10)

Stats table in `ASTRAWILD_CREATURE_SYSTEM.md` §2. Art: **skeletal mesh + icon per species** (weakness/
element should read visually). Replacement = `UAstrawildEchoDefinition` data asset with the same id +
mesh/icon wiring.

| Species | Id | Placeholder visual | Visual identity goal |
|---|---|---|---|
| Lumewisp | `Echo_Lumewisp` | Engine sphere ×0.8 | small floating light mote |
| Stonehide | `Echo_Stonehide` | Engine sphere ×0.8 | stocky rocky quadruped |
| Voltling | `Echo_Voltling` | Engine sphere ×0.8 | crackling nocturnal energy critter |
| Duskmoth | `Echo_Duskmoth` | Engine sphere ×0.8 | shy dusk moth |
| Gloomfang | `Echo_Gloomfang` | Engine sphere ×0.8 | night-stalker predator silhouette |
| Sprigling | `Echo_Sprigling` | Engine sphere ×0.8 | herding flora lamb with petal mane |
| Emberfang | `Echo_Emberfang` | Engine sphere ×0.8 | crepuscular ember predator, ash-trail glow |
| Rimefang | `Echo_Rimefang` | Engine sphere ×0.8 | frost hostile, Gloomfang's cold rival (wave 8 — Batch 5) |
| Voltmaw | `Echo_Voltmaw` | Engine sphere ×0.8 | pulse glass-cannon, fastest + highest ATK (wave 8 — Batch 5) |
| Auroraling | `Echo_Auroraling` | Engine sphere ×0.8 | Ancient-rare light spirit, one per world (wave 8 — Batch 5) |

---

## 4. Buildings (13)

Full stat table in `ASTRAWILD_BUILDING_SYSTEM.md` §5. Replacement = `UAstrawildBuildingDefinition` data
asset + **mesh** (category silhouettes are scaled cubes today).

| Building | Id | Placeholder | Mesh goal |
|---|---|---|---|
| Foundation | `Building_Foundation` | cube 2×2×0.2 | wood platform module |
| Wall | `Building_Wall` | cube 2×0.2×1.5 | wood wall module |
| Workbench | `Building_Workbench` | cube 1.2×1.2×1 | workbench prop |
| Campfire | `Building_Campfire` | cube 1.2×1.2×1 | campfire prop (+fire VFX) |
| Echo Dynamo | `Building_Generator` | cube 0.9×0.9×1.4 | resonant generator |
| Charge Cell | `Building_Battery` | cube 0.9×0.9×1.4 | battery unit |
| Dawn Lamp | `Building_LampPost` | cube 1.2×1.2×1 | lamp post (+light) |
| Farm Plot | `Building_FarmPlot` | cube 1.2×1.2×1 | tilled soil plot |
| Research Desk | `Building_ResearchDesk` | cube 1.2×1.2×1 | research desk prop |
| Echo Feed Trough | `Building_FeedTrough` | cube 1.2×1.2×1 | feed trough prop (husbandry) |
| Sawmill | `Building_Sawmill` | cube 1.4×1.4×1.2 | sawmill prop — bulk planks (wave 8 — Batch 5) |
| Hearth Coil | `Building_Heater` | cube 1.2×1.2×1 | heater prop — warmth auras (wave 8 — Batch 5) |
| Dawn Composter | `Building_Composter` | cube 1.4×1.4×1 | composter prop — fertilizer (wave 8 — Batch 5) |

---

## 5. Technologies (10)

| Tech | Id | Cost | Prereqs | Unlocks | Replacement |
|---|---|---|---|---|---|
| Basic Crafting | `Tech_BasicCrafting` | 0 | — | — (root) | Data asset |
| Cooking | `Tech_Cooking` | 5 | Basic Crafting | `Recipe_CookedMeat` | Data asset + tree icon |
| Electrical Foundations | `Tech_Electrical` | 15 | Basic Crafting | Generator, Battery, Lamp | Data asset + tree icon |
| Advanced Energy | `Tech_AdvancedEnergy` | 30 | Electrical | — (future) | Data asset + tree icon |
| Echo Husbandry | `Tech_Husbandry` | 10 | Cooking | Feed Mix + Salve recipes, Feed Trough | Data asset + tree icon |
| Armory | `Tech_Armory` | 8 | Basic Crafting | Stonehide Shield + Dawn Crystal Blade recipes (wave 3) | Data asset + tree icon |
| Mechanical Workshop | `Tech_Mechanics` | 12 | Basic Crafting | Bulk Planks + Resonator Batch recipes, Sawmill (wave 8 — Batch 5) | Data asset + tree icon |
| Thermal Engineering | `Tech_Thermal` | 18 | Electrical | Hearth Broth recipe, Hearth Coil (wave 8 — Batch 5) | Data asset + tree icon |
| Agriculture | `Tech_Agriculture` | 20 | Echo Husbandry | Fertilizer recipe, Dawn Composter (wave 8 — Batch 5) | Data asset + tree icon |
| Ancient Resonance | `Tech_AncientResonance` | 25 | Advanced Energy | Ancient Resonator recipe (wave 9 — Batch 6) — **force-unlocked by the Hollow Underlight** | Data asset + tree icon |

---

## 6. Quests (7)

| Quest | Id | Objectives | Rewards | Replacement |
|---|---|---|---|---|
| First Light | `Quest_FirstLight` | 10 Wood, 5 Stone | 2 Resonators, 5 RP | Data asset (text polish) |
| A Friend in the Fields | `Quest_FirstEcho` | Observe Lumewisp, Capture Lumewisp | 10 Berries, 10 RP | Data asset |
| Homeground | `Quest_Homeground` | Foundation, Workbench | 10 RP | Data asset |
| The Spark | `Quest_Spark` | Tech_Electrical, Generator | 15 RP | Data asset |
| Dawn Guard | `Quest_DawnGuard` | Defeat 3 Gloomfangs | Ancient Core, **Dawn Shard ×5 (Batch 4)**, 20 RP | Data asset |
| Shepherd's Dawn | `Quest_ShepherdsDawn` | Tech_Husbandry, Capture Sprigling, Feed Trough, 3 Feed Mix | 5 Feed Mix, 2 Salve, 20 RP | Data asset |
| The Hollow Underlight | `Quest_HollowUnderlight` | **ReachLocation** Hollow Underlight, DefeatCreature Underlight Warden (wave 9 — Batch 6) | 2 Salve, 5 Dawn Shards, 15 RP | Data asset |

---

## 7. Loot Tables (2) — wave 3, extended wave 6

Registered through `UAstrawildItemRegistrySubsystem::RegisterLootTable`; resolved by
`FindLootTable`. Granted by `AAstrawildDungeonRoomActor::GrantClearReward` when a room template
carries `ClearLootTableId` (all guaranteed drops + one bonus roll when `FRand() < BonusRollChance`).
Since Batch 4, a vendor's `ShopLootTableId` table ALSO doubles as its shop stock list (prices live on
each item's `VendorPrice`).

| Loot table | Id | Guaranteed drops | BonusRollChance | Used by | Replacement |
|---|---|---|---|---|---|
| Dungeon Boss | `Loot_DungeonBoss` | Ancient Core ×1, Dawn Crystal Shard ×2, Ember Ash ×2, **Dawn Shard ×3 (Batch 4)** | 0.75 | Hollow Underlight boss room (`ClearLootTableId`) | Data asset (drop tuning) |
| Vendor Starter | `Loot_VendorStarter` | Glimmer Berry ×3, Dew Flask ×1, Sunfiber Bandage ×2, **Dawnbloom Salve ×1 + Echo Resonator ×1 (Batch 4)** | 0.0 | Trader Tam shop stock (`ShopLootTableId`) — never depletes in v1 | Data asset |

> **Bonus roll:** the extra roll picks one stack uniformly from `GuaranteedDrops` and grants it a
> second time. `BonusRollChance = 0` disables it.

---

## 8. NPCs (2) — wave 3; vendor flow live wave 6

Registered through `RegisterNPC`; resolved by `FindNPCDefinition`. Both are spawned at the starting
camp by `AAstrawildWorldBootstrapper::SpawnPointsOfInterest` (±`CampRadius * 0.7` = ±630 cm south
side, z = 100).

| NPC | Id | Hooks | Spawn (camp) | Replacement |
|---|---|---|---|---|
| Warden Maren | `NPC_WardenMaren` | offers `Quest_FirstLight` | (630, −630, 100) | Data asset (dialogue/schedule) + real mesh |
| Trader Tam | `NPC_VendorTam` | `ShopLootTableId = Loot_VendorStarter` + `CurrencyItemId = Item_DawnShard` (live vendor since Batch 4) | (−630, −630, 100) | Data asset + real mesh |

> NPC bodies still use the engine capsule placeholder (`AAstrawildNPCCharacter`, section 9). The
> vendor **transaction flow is LIVE since Batch 4 (`c16fecd`)** — `TryPurchase`/`TrySell`
> (server-authoritative, 450 cm trade range); Interact lists wares + prices + Dawn Shard balance as a
> HUD toast; console `AW.BuyItem`/`AW.SellItem`. The **shop UMG screen** (visual shop UI) and
> NPC dialogue/schedule screens remain NOT IMPLEMENTED (future round).

### 8.1 Vendor economy quick reference (Batch 4 — M-11)

- **Currency:** Dawn Shard (`Item_DawnShard`, 0.1 kg, stack 200, `VendorPrice 0` — cannot be bought
  or sold with itself). Sources: Hollow Underlight boss loot (×3), Dawn Guard quest reward (×5),
  prototype starter kit (×10).
- **Wares at Trader Tam:** Glimmer Berry 2 · Dew Flask 2 · Sunfiber Bandage 3 · Dawnbloom Salve 4 ·
  Echo Resonator 6 (prices are each item's `VendorPrice` in Dawn Shards).
- **Sell rule:** half the buy price floored at 1 per unit (`ComputeVendorSellValue`); unpriced items
  (junk) and the currency itself are not sellable — no arbitrage loop by construction.
- **Transaction API:** `AAstrawildNPCCharacter::TryPurchase/TrySell` — server-authoritative
  (`GetLocalRole() == ROLE_Authority`), quantity 1–99, validates vendor → 450 cm range → registry →
  price > 0 → ware membership → funds → weight BEFORE transferring anything (no partial
  transactions); returns `EAstrawildVendorResult` (7 values).
- **Cheat usage:** `AW.BuyItem Item_Bandage 2` / `AW.SellItem Item_Berry 5` — route through the same
  API via `FindNearestVendor` (600 cm search, skips non-vendor NPCs — REVIEW-4 L-1 fix).
- **Honest status:** VERIFIED AT SOURCE (`c16fecd` — compile pending on target machine); shop stock
  never depletes in v1; shop UMG screen pending a future round.

---

## 9. World & System Placeholders (not registry content, same policy)

| Placeholder | Used by | Tag |
|---|---|---|
| Player mesh (engine cylinder) | PlayerCharacter | PLACEHOLDER / REPLACE_BEFORE_RELEASE |
| Echo mesh (engine sphere) | EchoCharacter (all species) | PLACEHOLDER / REPLACE_BEFORE_RELEASE |
| NPC mesh (engine capsule) | NPCCharacter | PLACEHOLDER / REPLACE_BEFORE_RELEASE |
| Resource node (cube), rest point (cylinder), crafting station (cylinder), work site (cube) | respective actors | PLACEHOLDER / REPLACE_BEFORE_RELEASE |
| Boss mesh (engine cone ×2.4) | EchoBossCharacter — Underlight Warden | PLACEHOLDER / REPLACE_BEFORE_RELEASE |
| Dungeon room shells (cube floor plates) | DungeonRoomActor | PLACEHOLDER — walls arrive with the asset pass |
| Dungeon gate (cylinder pillars + cube crossbar) | DungeonGateActor (wave 9 — Batch 6) | PLACEHOLDER — resonance-arch meshes |
| Dungeon portal (squashed cylinder pad) | DungeonPortalActor (wave 9 — Batch 6) | PLACEHOLDER — portal frame + VFX |
| Ground plane (engine plane ×80) + spawned lighting rig | WorldBootstrapper | PLACEHOLDER — replaced by a real Dawn Fields map (M9) |
| HUD Roboto engine font | HudWidget | acceptable for slice; themed font later |
| Interaction prompts (2 Thai NSLOCTEXT strings from v1) | ResourceNode / RestPoint | localize to LOCTEXT tables with the UI pass |

---

## 10. Not Yet Content (reserved, no CODE_DEFAULT entries)

- Ability definitions (`AbilityIds` on Echo definitions) — array exists, empty.
- Biomes beyond Dawn Fields — tags exist (`Biome.*`), no content.
