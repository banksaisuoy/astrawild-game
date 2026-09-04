# ASTRAWILD — AUTOMATION TEST INVENTORY

**Suite**: 102 world-free contract tests · `Source/AstrawildCore/Private/AstrawildAutomationTests.cpp`
**Flags**: `EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter` (guarded by `#if WITH_DEV_AUTOMATION_TESTS`)
**Status**: IMPLEMENTED (ENGINE-UNVERIFIED — the suite compiles with the module and runs on the Antigravity machine at AG-3; this sandbox has no UE5/MSVC, so the run itself is pending)

Every test follows the house rules: **no world, no spawned actors** (pure structs,
CDO reads and static resolvers only — the one exception class reads the
`AAstrawildSkiffActor` CDO, never a spawned actor), deterministic inputs, and
one behavioral contract per domain. The Final Run additions (#62-67) pin the
Act 3 story spine end-to-end at the data level; the final-audit regressions
(#68-72) pin the audit's P0/P1 fixes with behavioral contracts (real matcher
paths, real import paths); the GDP pack (#73-84) pins the depth systems
(abilities/locomotion/attributes/affinity); the SCP pack (#85-99) pins the
systems-completion layer, and the FCR regressions (#100-102) pin this run's
DDA/ability-kit/IV fixes (validator/fallback/spoilage/sanity/mount/combos/
difficulty/crops/schedules/turrets/genetics/perf).

**Count reconciliation (Final Completion Run Phase 0, binding)**: the single
authoritative test count is **103** — derived from `AutomationTests.cpp`
(`IMPLEMENT_SIMPLE_AUTOMATION_TEST` count), enforced by the static validator
EXACT gate (`Automation tests == 103`), and listed in this inventory (rows 1-103).
Historical counts (57/63/67/72/84/99) describe earlier commits only and appear
nowhere as current-state claims.

## Inventory (1-57: baseline hardening suite, landed c65d734)

| # | Test | Contract |
| :-- | :--- | :--- |
| 1 | ASTRAWILD.Inventory.AddRemove | stack add/remove/weight basics |
| 2 | ASTRAWILD.Survival.DamageAndDeath | damage pipeline + death threshold |
| 3 | ASTRAWILD.Capture.DesignRuleBounds | capture chance bounds |
| 4 | ASTRAWILD.Combat.MitigationMath | armor mitigation formula |
| 5 | ASTRAWILD.Save.ChecksumDeterminism | FNV-1a header checksum determinism |
| 6 | ASTRAWILD.Quest.ObjectiveProgress | objective progress accumulation |
| 7 | ASTRAWILD.Echo.PersonalityModifiers | personality stat modifiers |
| 8 | ASTRAWILD.Power.BrownoutMath | power grid brownout thresholds |
| 9 | ASTRAWILD.Equipment.ProgressionMath | equipment stat progression |
| 10 | ASTRAWILD.Equipment.ArmorMath | armor slot math |
| 11 | ASTRAWILD.Combat.StatusEffectFactory | element→status vocabulary |
| 12 | ASTRAWILD.Economy.VendorSellValue | vendor sell pricing |
| 13 | ASTRAWILD.Dungeon.BossElementalMultiplier | boss weakness ×1.5 / resist ×0.80 (unified in FINAL-AUDIT-B) |
| 14 | ASTRAWILD.Dungeon.BossPhaseThresholds | phase thresholds 0.66/0.33 |
| 15 | ASTRAWILD.Dungeon.BossAttackDamage | phase/enrage damage scaling |
| 16 | ASTRAWILD.Zones.TableIntegrity | 12-zone table integrity |
| 17 | ASTRAWILD.Zones.LookupCorrectness | zone lookup by XY |
| 18 | ASTRAWILD.Zones.BlendPartitionOfUnity | zone blend weights partition of unity |
| 19 | ASTRAWILD.Terrain.HeightDeterministic | height field determinism |
| 20 | ASTRAWILD.Terrain.SeamContinuity | tile seam continuity |
| 21 | ASTRAWILD.Equipment.SlotRouting | 6 equipment slot routing |
| 22 | ASTRAWILD.Survival.InsulationBand | insulation temperature bands |
| 23 | ASTRAWILD.Quest.ObjectiveTypes | objective type serialization vocabulary |
| 24 | ASTRAWILD.Save.SchemaV3 | v3 additive migration contract |
| 25 | ASTRAWILD.Dungeon.BossSpecialsMath | special attack cooldown/damage math |
| 26 | ASTRAWILD.Bestiary.TableIntegrity | 204-species bestiary integrity |
| 27 | ASTRAWILD.Zones.SeaClassification | sea zone classification |
| 28 | ASTRAWILD.Skiff.FlightMath | skiff velocity clamps |
| 29 | ASTRAWILD.Craft.OutputGuard | crafting output guards |
| 30 | ASTRAWILD.Weapon.ProfileMath | weapon profile math |
| 31 | ASTRAWILD.WorldEvent.EligibilityGates | world event eligibility |
| 32 | ASTRAWILD.WorldEvent.WeightedPickDeterminism | weighted pick determinism |
| 33 | ASTRAWILD.POI.DiscoveryRadiusMath | POI discovery radius (scanner ×2) |
| 34 | ASTRAWILD.ResourceNode.DefinitionContract | node identity is concrete |
| 35 | ASTRAWILD.Echo.ProductionRosterContract | production roster archetypes |
| 36 | ASTRAWILD.Armor.SplitInsulation | split cold/heat insulation |
| 37 | ASTRAWILD.Robot.SpecialistRates | robot chassis specialist rates |
| 38 | ASTRAWILD.WorkSite.ProductionChain | work-site consume→produce chain |
| 39 | ASTRAWILD.Save.SchemaV4 | v4 additive migration contract |
| 40 | ASTRAWILD.Quest.DiscoverPOIType | DiscoverPOI objective wiring |
| 41 | ASTRAWILD.BiomeDressing.ZoneProfiles | biome dressing profiles |
| 42 | ASTRAWILD.BiomeDressing.PointRejection | scatter point rejection |
| 43 | ASTRAWILD.BiomeDressing.DeterministicScatter | deterministic scatter |
| 44 | ASTRAWILD.Atmosphere.DayRamp | day/night atmosphere ramp |
| 45 | ASTRAWILD.Vfx.Palette | element tint palette |
| 46 | ASTRAWILD.Vfx.BeamMath | beam VFX math |
| 47 | ASTRAWILD.Vfx.ArcJitter | arc jitter determinism |
| 48 | ASTRAWILD.Vfx.RingGeometry | ring VFX geometry |
| 49 | ASTRAWILD.Dialogue.TreeContract | node/goto resolution + ambiguity rules |
| 50 | ASTRAWILD.Dialogue.ChoiceConditions | AND condition filtering |
| 51 | ASTRAWILD.Dialogue.Consequences | consequence order + hard-fail rules |
| 52 | ASTRAWILD.Echo.EvolutionGates | evolution level/bond gates |
| 53 | ASTRAWILD.Weapon.AssetBindingContract | weapon art binding round-trip |
| 54 | ASTRAWILD.ArtPack.BindingContract | ArtPack soft path contract |
| 55 | ASTRAWILD.Input.CoreLoopRoster | core input action roster |
| 56 | ASTRAWILD.Input.RuntimeActionContract | runtime action contract |
| 57 | ASTRAWILD.Asset.SurvivorFallbackChain | survivor mesh fallback chain |

## Inventory (58-61: Final Run BATCH-1 hardening, landed 61c45e6)

| # | Test | Contract |
| :--- | :--- | :--- |
| 58 | ASTRAWILD.Inventory.TransactionSafety | negative-qty mint exploit + atomic consume (FR-1) |
| 59 | ASTRAWILD.Save.ConsistencyContracts | schema refusal + day cap + NaN guards (FR-2) |
| 60 | ASTRAWILD.Quest.ImportSafety | import sanitize: dedupe + single-active (FR-3) |
| 61 | ASTRAWILD.Echo.RosterImportSafety | roster import sanitize: guid/species collapse (FR-4) |

## Inventory (62-67: Final Run BATCH-2 Act 3 spine, landed 93ee929)

| # | Test | Contract |
| :-- | :--- | :--- |
| 62 | ASTRAWILD.Quest.FinalRunChain | MQ-13..17 chain: links resolve, one terminus, reward policy (FR-5/FR-12) |
| 63 | ASTRAWILD.Dialogue.EndingChoice | ending id vocabulary maps to exactly two states; TriggerEndingId default-off (FR-6) |
| 64 | ASTRAWILD.Save.SchemaV5Ending | schema 5, ending round-trip, corrupt-value clamp (FR-6) |
| 65 | ASTRAWILD.Echo.FinalRunBosses | boss display-name roster; Sovereign 2000 HP + Dawn Light weakness + phase bands (FR-11) |
| 66 | ASTRAWILD.Tech.SkiffEngineering | tech/recipe gate: one unlock, three materials, Maelstrom Glass anchor (FR-5/FR-8) |
| 67 | ASTRAWILD.Skiff.CeilingGate | ceiling resolver 12000/16000; Eye Gate at 15000 sits between them (FR-8) |

## Inventory (68-72: FINAL-AUDIT regressions, landed a5aa74d)

| # | Test | Contract |
| :-- | :-- | :-- |
| 68 | ASTRAWILD.Quest.OneShotBackFill | G-1/G-3: discovered-POI + one-shot-boss objectives back-fill from world history; partial counts cap; CollectItem never back-filled; completed objectives skipped |
| 69 | ASTRAWILD.Quest.DefeatCountImportSafety | G-3: crafted saves cannot mint defeat credit (id-less dropped, negatives dropped, 999 cap) |
| 70 | ASTRAWILD.Quest.DismantleIsNotPlacement | F-03: drives the REAL objective matcher — BuildingPlaced Amount=-1 (dismantle) never advances placement; +1 does |
| 71 | ASTRAWILD.Research.ImportSafety | M-3: duplicate tech ids collapse, id-less entries drop, negative RP clamps to zero |
| 72 | ASTRAWILD.Save.FinalAuditContracts | M-2/H-2: echo health additive field (legacy 0 sentinel + carry + negative gate) + robot chassis round-trip shape |

## Run instructions (Antigravity, AG-3)

```
UE_5.8\Engine\Build\BatchFiles\RunUAT.bat BuildGraph ...   (or the Editor automation UI)
Filter: ASTRAWILD.                                          (all 102)
Expected: 102 pass / 0 fail / 0 skip. Any failure → capture the raw log and file AG-6.
```

## Gameplay Depth Pack contracts (73-84)

| # | Test | Covers |
|---|------|--------|
| 73 | ASTRAWILD.Ability.LibraryIntegrity | 44 unique templates, bounds, names, payload sanity |
| 74 | ASTRAWILD.Ability.DerivedLoadout | Every element x role combo derives >= 4 abilities incl. offense; deterministic |
| 75 | ASTRAWILD.Ability.CombatPick | Tactical ladder: hurt -> heal, healthy -> offense, cooldown/range gating |
| 76 | ASTRAWILD.Ability.SpeciesLoadout | Authored ids lead, derived fill, no duplicates |
| 77 | ASTRAWILD.Locomotion.Derivation | Avian/floating fly; aquatic/sea-zone swim; flight outranks water; else land |
| 78 | ASTRAWILD.Attributes.XPCurve | Level thresholds, overflow carry, cap 10 + residue clear, negative XP rejected |
| 79 | ASTRAWILD.Attributes.BonusFormulas | All seven bonus formulas at fresh/max, masterwork 15% gate |
| 80 | ASTRAWILD.Attributes.SkillUnlock | Milestone table, cooldown table, smart-cast ladder + cooldown blocking |
| 81 | ASTRAWILD.Attributes.SaveRoundTrip | Clean round-trip, corrupt import repairs (clamp/dedup), empty = fresh |
| 82 | ASTRAWILD.NPC.AffinityTiers | 0/25/50/75 boundaries, discount ladder, stable id fallback |
| 83 | ASTRAWILD.NPC.AffinitySave | Payload round-trip + save field defaults empty |
| 84 | ASTRAWILD.Ability.EngineContracts | End-to-end echo contracts: knowability gates, cooldown queries, combat picks |

## Systems Completion Pack contracts (85-99, landed SCP-1..6)

| # | Test | Covers |
|---|------|--------|
| 85 | ASTRAWILD.SCP.DataValidator.StaticTables | Bestiary + ability + element-chain integrity (SCP-1) |
| 86 | ASTRAWILD.SCP.ErrorReporter.RingBuffer | Diagnostic trail capacity + formatting (SCP-1) |
| 87 | ASTRAWILD.SCP.AssetFallback.ShapePaths | Engine basic-shape fallback map (SCP-1) |
| 88 | ASTRAWILD.SCP.Spoilage.Math | Stack aging + Ice Box preservation + conversion (SCP-1) |
| 89 | ASTRAWILD.SCP.Durability.Contracts | Wear constants + legacy-inert definition fields (SCP-1) |
| 90 | ASTRAWILD.SCP.Sanity.MathAndIllness | Sanity rates + illness risk bands + modifiers (SCP-2) |
| 91 | ASTRAWILD.SCP.BaseTerminal.LevelAndGarrison | Territory 3500 + levels + garrison caps (SCP-2) |
| 92 | ASTRAWILD.SCP.Mount.SpeciesAndSpeed | Rideable gates + 1.25x speed + socket contract (SCP-3) |
| 93 | ASTRAWILD.SCP.Combo.ReactionTable | 12 dual-tech reactions + Steam Explosion contract (SCP-4) |
| 94 | ASTRAWILD.SCP.DDA.SkillBands | Difficulty bands + multipliers (SCP-4) |
| 95 | ASTRAWILD.SCP.Crop.GrowthMath | Water/fertilizer/season + state ladder (SCP-5) |
| 96 | ASTRAWILD.SCP.NPC.ScheduleAnchors | Professions + hours + rain shelter + service gating (SCP-5) |
| 97 | ASTRAWILD.SCP.Turret.RangeAndPolicy | Range/cadence/damage + party-safe targeting (SCP-5) |
| 98 | ASTRAWILD.SCP.Genetics.Inheritance | Trait effects + deterministic rolls + IVs (SCP-6) |
| 99 | ASTRAWILD.SCP.Perf.TierLadder | Scalability ladder + hysteresis policy (SCP-6) |

## FCR regression contracts (100-102, landed this run)

| # | Test | Covers |
|---|------|--------|
| 100 | ASTRAWILD.FCR.DDA.PartyLossDirection | M-d8: party echo losses pull the band DOWN (2 losses -> Struggling); weight = half a player death |
| 101 | ASTRAWILD.FCR.Ability.FullElementKits | M-a8: every element derives its full 4-ability kit — 6 entries incl. the previously dead templates |
| 102 | ASTRAWILD.FCR.Genetics.IVConsumptionBounds | H-d5: IV multiplier bounds (0 neutral / 31 = +31%, negatives & oversized clamp) — the IV layer is live |

## DP-3 depth contracts (103, landed this run)

| # | Test | Covers |
|---|------|--------|
| 103 | ASTRAWILD.DP3.Resonance.PairResolution | DP-3: 15-pair element resonance — themed rows resolve symmetrically, None/same never resonate, three-element parties resolve the canon-dominant pair, every row carries exactly one modest bonus axis in the 8-12% band |
