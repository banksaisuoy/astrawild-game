# ASTRAWILD — AUTOMATION TEST INVENTORY

**Suite**: 111 world-free contract tests · `Source/AstrawildCore/Private/AstrawildAutomationTests.cpp`
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
systems-completion layer, the FCR regressions (#100-102) pin that run's
DDA/ability-kit/IV fixes (validator/fallback/spoilage/sanity/mount/combos/
difficulty/crops/schedules/turrets/genetics/perf), the DP-3 contract (#103)
pins the party element resonance, the DP-4 contract (#104) pins the
player skill loadout (bind validation + bound-only smart-cast + the
empty-loadout legacy fallback), the DP-5 contract (#105) pins the
per-boss special sets (canonical id resolution, unknown-id fallback,
zero-regression default tuning, pairwise-distinct data), and the DP-6 contract
(#106) pins the base depth (8-site work-type coverage with two named design
exceptions, 17/17 research branch assignments, and the field-consumable
production→progression loop: item verbs, recipe mirrors, site output wiring),
and the DP-7 contract (#107) pins the world depth (per-zone hazard identity
with pure consumption helpers, the 7 previously-bare zones each anchoring an
event built from the existing effect vocabulary, and the 4 scanner-gated
secret POIs), and the DP-8 contract (#108) pins the NPC depth (the affinity
gate evaluation: default-0 never gates, fail-closed without a talking NPC,
the inclusive 25/50/75 tier boundaries, flag-pair AND semantics, and the four
evolved trees' gated replies pinned through the live registry census —
NPC/dialogue counts stay 11/11: depth, not clones), and the DP-9 contract
(#109) pins the dungeon depth (theme resolution per dungeon id — the 3
canonical dungeons resolve 3 distinct themes with pairwise-distinct shell
tints/proportions/walls/hazards and ArtPack-resolvable dressing vocabulary;
the resonance-pillar sequence verbs — correct order advances to completion,
wrong order resets, window expiry resets; the unthemed default stays the
legacy shell).

**Count reconciliation (Final Completion Run Phase 0, binding)**: the single
authoritative test count is **111** — derived from `AutomationTests.cpp`
(`IMPLEMENT_SIMPLE_AUTOMATION_TEST` count), enforced by the static validator
EXACT gate (`Automation tests == 111`), and listed in this inventory (rows 1-111).
Historical counts (57/63/67/72/84/99/102/103/104/105/106/107/108) describe earlier commits only and appear
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
Filter: ASTRAWILD.                                          (all 111)
Expected: 111 pass / 0 fail / 0 skip. Any failure → capture the raw log and file AG-6.
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

## DP-3 depth contracts (103, landed in the DP-3 batch)

| # | Test | Covers |
|---|------|--------|
| 103 | ASTRAWILD.DP3.Resonance.PairResolution | DP-3: 15-pair element resonance — themed rows resolve symmetrically, None/same never resonate, three-element parties resolve the canon-dominant pair, every row carries exactly one modest bonus axis in the 8-12% band |

## DP-4 depth contracts (104, landed this run)

| # | Test | Covers |
|---|------|--------|
| 104 | ASTRAWILD.DP4.SkillLoadout | DP-4: 3-slot player skill loadout — slot bounds / locked-skill / duplicate / None bind validation, rebind replaces the occupant, clear is a safe no-op out of bounds, bound-only smart-cast (unbound unlocked skills suppressed), empty-loadout legacy fallback picks among ALL unlocked skills, save v5 round-trip + pre-DP-4 payload resets to all-empty |

## DP-5 depth contracts (105, landed in the DP-5 batch)

| # | Test | Covers |
|---|------|--------|
| 105 | ASTRAWILD.DP5.BossSpecialSets | DP-5: per-boss special sets — the four canonical defeat ids resolve to four distinct sets, unknown/None ids fail closed to the default set, the default set is the byte-exact legacy tuning (7s/1 bolt/1 blast/350cm/1 hazard/6dps/Gloomfang), no two sets share a tuning bundle, and every set stays inside the sane combat band (cooldown 4-10s, 1-4 bolts, 1-3 blasts, 250-500cm, 1-4 hazards, 4-10dps, set summon species) |

## DP-6 depth contracts (106, landed in the DP-6 batch)

| # | Test | Covers |
|---|------|--------|
| 106 | ASTRAWILD.DP6.BaseDepth | DP-6: base depth — registry-backed world-free census (ownerless `BuildDefaults`): 8 work sites registered with unique ids / resolvable outputs+inputs / placed zones, work-type coverage pins the 8 covered types (Gathering/Farming/Mining/Cooking/Transport/ResearchAssist/PowerGeneration/Defense) with Crafting Assistance + Construction named as the two by-design exceptions, all 17 techs resolve with their audited research branch (10 legacy + 7 production rows pinned), Field Ration carries a timed non-damaging stamina-regen status + food value, Pulse Tonic grants capture-focus seconds + heal/water with no status payload, fresh status effects default to zero regen (additive shape), and the loop closes end-to-end (recipes output the consumables; the Tidebreaker depot consumes kitchen meat + farm berries and outputs Field Rations; the Verdant lab outputs Pulse Tonics) |

## DP-7 depth contracts (107, landed in the DP-7 batch)

| # | Test | Covers |
|---|------|--------|
| 107 | ASTRAWILD.DP7.WorldDepth | DP-7: world depth — per-zone hazard identity from the pure static zone table (all 12 zones carry an explicit hazard row; the thermal offset + stamina-regen penalty helpers mirror the enum for Cold/Heat/AshLung; hazard-free zones stay fully neutral with zero pressure; Dawn Fields stays gentle by design; Frostveil reads colder than Dawn Fields — the layering contract the survival tick consumes; Hollow Approach is the ash-lung identity), the 7 previously-bare zones each anchor at least one of the 16 registered world events (live registry census, ownerless `BuildDefaults`; every new event pins its zone, balance band, cooldown, day-gate, no night-gate, and payload resolution — species boost ids, bonus node ids, loot tables), and the 17-POI census with 6 scanner-gated secrets (the 4 new DP-7 caches + 2 legacy signal sources all gated, SignalSource-typed, high-threat-zone-placed, with real loot + research rewards) |

## DP-8 depth contracts (108, landed in the DP-8 batch)

| # | Test | Covers |
|---|------|--------|
| 108 | ASTRAWILD.DP8.AffinityDialogue | DP-8: NPC depth — the affinity-gated dialogue evolution gate: the pure resolver (threshold 0 never gates — the fresh additive default keeps every pre-DP-8 tree byte-identical; below threshold fails; the threshold is inclusive so the 25/50/75 tier boundaries resolve exactly on Acquaintance/Friend/Confidant), component evaluation fail-closed (a gated reply hides when no talking NPC can be resolved, exactly like the quest conditions; a default-0 reply stays visible in the same state), the live talking-NPC path (a world-free NPC at 24 misses the 50 gate, at 50 passes, and clearing the NPC fails the gate closed again), AND semantics with the flag conditions, the four evolved trees pinned through the ownerless `BuildDefaults` registry (Tam Friend-50 supply line, Rowan Confidant-75 old doors, Nima Friend-50 rare goods, Sela Acquaintance-25 patrol chart — each gated reply one-time via forbidden flag and paying real consequences: research points / shop bridge / item grants, existing verbs only), and the census pin that NPC/dialogue counts stay 11/11 (depth, not clones) |

## DP-9 depth contracts (109, landed this run)

| # | Test | Covers |
|---|------|--------|
| 109 | ASTRAWILD.DP9.DungeonIdentity | DP-9: dungeon depth — theme resolution per dungeon id (the 3 canonical dungeon ids resolve 3 DISTINCT themes; unknown/empty ids fail closed to None so identity never breaks a dungeon), the theme profile table the rooms consume (pairwise-distinct shell tints; Underlight tighter than the Vault which reads wider than the Eye; Eye monolith walls taller than the Underlight's oppressive slabs; only the Eye pulses its accent light; the per-dungeon hazard identity is exactly AshLung/Waterlogged/EnergyPulse with mild bands — ash lung ≤ 8/s, waterlogged 0.5-1.0×, pulses 5s+ cadence / ≤ 6 dps / dissipating within the cadence; dressing vocabulary resolves through the EXISTING ArtPack biome/node tables with sane budgets; only the Vault carries the flooded-floor accent; only the Eye carries the ancient-tech node accent; the unthemed default stays the legacy shell — no hazard, no walls, no light, unscaled footprint), and the resonance-pillar sequence verbs (3 pillars, 20-90s window; correct order advances → advances → completes; skipping ahead / re-attuning / out-of-range / degenerate / stale inputs reset; window expiry is inclusive at the boundary and a zero window never expires; the room-hazard status ids are stable) |

## LCP-2 LAN co-op contracts (110-111, landed in the LCP-2 batch)

| # | Test | Covers |
|---|------|--------|
| 110 | ASTRAWILD.LCP2.ClientWorldPolicy | LCP-2: client world build policy — the cosmetic-build truth table (authority + standalone + listen-host build in BeginPlay; remote clients build from the replicated seed; dedicated-server proxies never build — out of scope per MASTER_CONTROL §1b), the client weather-visibility mapper equals the profile table for all six states (one source of truth: the same static the server atmosphere pass uses), and cosmetic-stream determinism (same seed -> identical landmark draw sequence, different seed -> different sequence, no hidden global state) |
| 111 | ASTRAWILD.LCP2.DressingGate | LCP-2: client dressing gate — expected replicated world actor count for the exclusion-bubble sources (villages + dungeon generators + portals + POI markers + skiffs; POI count from the registry, negative clamps), the gate timeout contract (positive, >= 10s for slow LAN joins), and the shared node-depleted predicate (infinite nodes never deplete; finite nodes deplete at quantity <= 0 — the server harvest path and the client OnRep mirror use the same pure function so they can never drift) |
