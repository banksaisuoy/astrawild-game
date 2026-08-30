# ASTRAWILD — Build Status

## Status

- Overall: `PARTIAL` — full vertical-slice foundation implemented in C++ (**source-complete, never compiled**)
- Last updated: 2026-08-30 (Wave 6 — Batch 4: Survival feel + vendor economy)
- Branch: `main` (latest: `c16fecd` — Batch 4 survival feel + vendor economy; preceded by `5dd69cd` wave-5 docs sync / `021f93a` Batch 3 source)
- Latest change: **Wave 6 Batch 4** — four work items closed in source: (M-2a) sprint stamina
  drain (`SprintStaminaDrainPerSecond = 7` tunable + moving-only drain + `OnSprintExhausted`
  broadcast — ≈14 s sprint from full stamina), (M-2b) the block movement penalty is now LIVE
  (`BlockSpeedMultiplier = 0.45` was dead code — nothing listened to `OnBlockingChanged`; now bound
  to `RefreshMovementSpeed`), (L-2) thirst decay 0.14 → **0.0833/s** (~12 → ~20 min, matches the
  documented design goal), and (M-11) the full vendor economy — `EAstrawildVendorResult` enum +
  `VendorPrice`/`CurrencyItemId` fields + server-authoritative `TryPurchase`/`TrySell` on
  `AAstrawildNPCCharacter` (450 cm trade range, no partial transactions) + Dawn Shard currency +
  5 wares at Trader Tam + `AW.BuyItem`/`AW.SellItem` cheats + `ASTRAWILD.Economy.VendorSellValue`
  test. REVIEW-4 verdict: CLEAN (no HIGH/MEDIUM); its L-1 vendor-filter cheat fix applied at commit.
- Codebase: **88 C++ files (43 `.cpp` + 45 `.h`), ~15,990 LOC** in `Source/AstrawildCore` (single module)

## Environment

- Unreal Engine: **Not run in sandbox environments; target is 5.8** (EngineAssociation in `ASTRAWILD.uproject`)
- Compiler: Not run in sandbox environments (Linux sandbox has no MSVC/UE toolchain); target build is Windows + VS2022
- OS: Repository validation run in Ubuntu sandbox; target build is Windows
- CPU/GPU/RAM/Storage: Not measured

## Compile

- Target: `ASTRAWILDEditor Win64 Development` — pending Antigravity (user machine)
- Result: `NOT_RUN` (unchanged — sandbox has no UE5; honest status per Definition of Done)
- Errors: **2 latent compile errors found by the audit and fixed in source in the Batch-1 round** —
  (1) `AstrawildCraftingStationActor.cpp:52` TPair-iteration over `TArray` (C-1);
  (2) `MakeRuntimeAction` declared 3 required params while all 17 call sites pass 2 (C-1b).
  Both await engine compile confirmation; more latent errors may surface. Batch-3 changes were
  read-only reviewed (REVIEW-3) — HIGH RISK: one **pre-existing** compile blocker caught and fixed
  in `021f93a` (missing `#include "AstrawildInventoryComponent.h"` in `AstrawildHudWidget.cpp` —
  the file calls inventory-component members while only forward-declaring the type; 3 of 5 offending
  accesses pre-date Batch 3 from wave 3 `2eeedf8`); MEDIUM RISK: two runtime bugs caught and fixed
  inline (M-1 Echo stagger expiry never restored `MaxWalkSpeed` — permanent creature freeze after
  any heavy hit; M-2 `FullRestore`/`SetStatsForRestore` cleared statuses without broadcasting
  `OnStatusEffectRemoved` — stale half-speed after rest/load); LOW RISK: see audit §23. Batch-4
  changes were read-only reviewed (REVIEW-4) — verdict **CLEAN**: no HIGH compile blockers and no
  MEDIUM runtime bugs in the 12-file diff; 5 LOW notes (see the REVIEW-4 table below — note L-1,
  the missing vendor filter in `FindNearestVendor`, was applied by the lead at commit time,
  `CheatManager.cpp:72-77`).
- Warnings: Not measured
- Build duration: Not measured
- Validation steps for the target machine: `Docs/ASTRAWILD_TEST_PLAN.md` §4

Static repository validation passed with `Scripts/validate_repository.sh`.

## Changes in this round (2026-08-30 — Wave 6 Batch 4: Sprint stamina drain + Live block penalty + Thirst rate fix + Vendor economy)

### Commits

| Commit | Type | Subject |
|---|---|---|
| `c16fecd` | feat(batch-4) | Survival feel + vendor economy — sprint stamina drain, live block penalty, thirst doc-alignment, Trader Tam purchase/sell flow with Dawn Shard currency (12 files, +584/−7) |

### Repository totals (verified with shell commands on `c16fecd`)

| Metric | Value | Command |
|---|---|---|
| C++ source files | **88** (43 `.cpp` + 45 `.h`) — unchanged from Batch 3 | `find Source -name '*.cpp' -o -name '*.h' \| wc -l` |
| C++ LOC | **15,990** | `find Source -name '*.cpp' -o -name '*.h' \| xargs wc -l \| tail -1` |
| Docs (`*.md`) | **49** — unchanged | `ls Docs/*.md \| wc -l` |
| DOREPLIFETIME props | **28 across 9 classes** — unchanged from Batch 3 (no new replicated props in Batch 4; the sprint-drain flag is deliberately server-side only) | `grep -c DOREPLIFETIME Source/AstrawildCore/Private/*.cpp` |
| Input actions | **19** — unchanged (no new keys in Batch 4) | `MakeRuntimeAction` call sites in `AstrawildPlayerCharacter.cpp::BuildRuntimeInputDefaults` |
| Cheat commands | **15** | `UFUNCTION(Exec)` methods on `UAstrawildCheatManager` (+`AW.BuyItem`/`AW.SellItem`) |
| Automation tests | **12** | `IMPLEMENT_SIMPLE_AUTOMATION_TEST` count (+`ASTRAWILD.Economy.VendorSellValue` — **5 of 12 now call production statics**) |
| Content totals | **23 items · 13 recipes · 7 species · 10 buildings · 6 techs · 6 quests · 2 loot tables · 2 NPCs** | `ContentLibrary.cpp:636` log line: "23 items, 13 recipes, 7 Echo species, 10 buildings, 6 technologies, 6 quests, 2 loot tables, 2 NPCs" |

> **LOC reconciliation note:** REVIEW-4 counted 15,984 LOC on the pre-commit working tree
> (5dd69cd + 12 modified files, +578/−7). The lead then applied REVIEW-4's L-1 vendor-filter fix
> (+6 lines in `CheatManager.cpp`) before committing, so `c16fecd` lands at +584/−7 and
> **15,990 LOC** (15,413 at `5dd69cd` + 584 − 7 = 15,990 — arithmetic confirms).

### Systems added / changed in Batch 4

| System | Class / symbol | Status | Files |
|---|---|---|---|
| Sprint stamina drain (M-2a) — tunable `SprintStaminaDrainPerSecond = 7.0` (100 stamina / 7 per s ≈ **14 s** sprint); `TickComponent` drains stamina INSTEAD of regenerating while the drain is armed AND the owner actually moves (regen 14/s would out-pace drain 7/s → free sprint); exhaustion at the floor clears the flag and broadcasts `OnSprintExhausted` **once** | `UAstrawildSurvivalComponent::SetSprintDrainActive(bool)` + private `bSprintDrainActive` (server-side only, NOT replicated) + `IsOwnerMoving()` (velocity² > 25² cm/s — walk 450/sprint 700 both qualify; holding sprint while standing still drains nothing) | NEW | `AstrawildSurvivalComponent.h/.cpp` |
| Sprint wiring (M-2a) — `StartSprint` arms the drain, `StopSprint` clears it; `OnSprintExhausted` handler drops `bSprinting` + clears the drain + `RefreshMovementSpeed()` (the existing >0.05 stamina-fraction gate keeps re-sprint suppressed until recovery); `OnPlayerDied` clears both so respawn (which `FullRestore`s stamina) never re-drains instantly | `AAstrawildPlayerCharacter::StartSprint/StopSprint/OnSprintExhausted/OnPlayerDied` (bindings at `cpp:141`, handlers at `cpp:434-459, 884-942`) | NEW | `AstrawildPlayerCharacter.h/.cpp` |
| Block movement penalty LIVE (M-2b) — `BlockSpeedMultiplier = 0.45` (`CombatComponent.h:83`) existed since the foundation but was **dead code**: `OnBlockingChanged` broadcast had no listener, so the penalty never applied (and never lifted). `BeginPlay` now binds `CombatComponent->OnBlockingChanged → OnBlockingChanged(bool) → RefreshMovementSpeed()`; the penalty multiplies the walk/sprint target inside `RefreshMovementSpeed` (`cpp:476-479`) | `AAstrawildPlayerCharacter::OnBlockingChanged` (`cpp:923-929`, binding at `cpp:150`) | NEW wiring (existing tunable) | `AstrawildPlayerCharacter.h/.cpp` (tunable in `AstrawildCombatComponent.h`) |
| Thirst decay alignment (L-2) — 0.14/s (~11.9 min) → **0.0833/s** (~20.0 min), matching the documented "~20 real minutes" design goal; header comment records the fix | `UAstrawildSurvivalComponent::ThirstDecayPerSecond` (`.h:51-56`) | CHANGED value | `AstrawildSurvivalComponent.h` |
| Vendor transaction API (M-11) — `EAstrawildVendorResult` UENUM (**7 values**: Success / NotAVendor / NotAWare / NotEnoughCurrency / TooHeavy / TooFarAway / InvalidRequest); `TryPurchase`/`TrySell` are **server-authoritative** (`GetLocalRole() == ROLE_Authority` gate) with constant `VendorTradeRangeCm = 450`; validation order (all BEFORE anything moves — no partial transactions): role → player/inventory/quantity 1–99 → vendor definition (ShopLootTableId + CurrencyItemId) → range → registry → item def + `VendorPrice > 0` → ware membership (shop loot table `GuaranteedDrops`) → funds (`HasItem`) → weight (`CanAddItem`) → then `RemoveItem` currency + `AddItemSilent` ware; sell = `ComputeVendorSellValue(price) = max(1, price/2)` per unit — junk (`VendorPrice 0`) and the currency itself are **not sellable** (no arbitrage loop by construction) | `AAstrawildNPCCharacter::TryPurchase/TrySell/ComputeVendorSellValue` (static BlueprintPure — unit-testable without a world) | NEW | `AstrawildNPCCharacter.h/.cpp`, `AstrawildTypes.h` |
| Economy data fields (M-11) — `VendorPrice` (int32, 0 = not tradeable) on `UAstrawildItemDefinition`; `CurrencyItemId` (FName, NAME_None = shop closed) on `UAstrawildNPCDefinition` | `AstrawildDataAssets.h:88-95, 435-441` | NEW fields | `AstrawildDataAssets.h` |
| Vendor session UX (M-11) — interacting with a vendor now lists wares + prices + the player's currency balance as a HUD toast ("Trader Tam's wares: Glimmer Berry [2] · … · You carry 10 Dawn Shard.") via `AAstrawildPlayerController::Notify`; no new widget — the shop **UMG screen remains a future round** | `AAstrawildNPCCharacter::Interact_Implementation` (`cpp:78-120`) | NEW | `AstrawildNPCCharacter.cpp` |
| Vendor cheats (M-11) — `AW.BuyItem <ItemId> [Qty]` / `AW.SellItem <ItemId> [Qty]` route through the same server-authoritative API a future shop UMG screen will use; `FindNearestVendor` scans within 600 cm **and skips non-vendor NPCs** (REVIEW-4 L-1 fix — Warden Maren can never shadow Tam); `VendorResultMessage` maps each result to an actionable HUD message | `UAstrawildCheatManager::BuyItem/SellItem` + file-local `FindNearestVendor`/`VendorResultMessage` (`cpp:57-110`) | NEW — **15 exec cheats total** | `AstrawildCheatManager.h/.cpp` |
| Economy content (M-11) — currency **Item_DawnShard** (Dawn Shard, Material, 0.1 kg, stack 200, `VendorPrice 0` — cannot be bought or sold with itself); Trader Tam wares: Glimmer Berry 2 / Dew Flask 2 / Sunfiber Bandage 3 / Dawnbloom Salve 4 / Echo Resonator 6; `Loot_VendorStarter` extended to 5 wares (+Salve ×1, +Resonator ×1); `Loot_DungeonBoss` + Dawn Shard ×3; `Quest_DawnGuard` reward + Dawn Shard ×5; prototype starter kit + Dawn Shard ×10 — **23 items total** | `AstrawildContentLibrary.cpp:124-179, 535, 586, 594`, `AstrawildPlayerCharacter.cpp:108-114` | NEW content | `AstrawildContentLibrary.cpp`, `AstrawildPlayerCharacter.cpp` |
| Vendor economy test — sell-value rule (half price floored at 1, junk = 0, strictly-below-buy-price for every priced ware); calls the REAL production static `ComputeVendorSellValue` | `FAstrawildVendorEconomyTest` = `ASTRAWILD.Economy.VendorSellValue` (`AutomationTests.cpp:308-332`) — **12 tests total** | NEW | `AstrawildAutomationTests.cpp` |

### REVIEW-4 findings (verdict CLEAN — read-only review of the pre-commit working tree at `5dd69cd`)

| Severity | Finding | Resolution in `c16fecd` |
|---|---|---|
| **HIGH / MEDIUM** | — none found — | Verdict CLEAN: all UHT signatures, delegate bindings, include closures, transaction atomicity and totals verified. Compile still must run on the UE 5.8 target machine |
| LOW (L-1) | `FindNearestVendor` ignored vendor status (nearest NPC could be Warden Maren → "not a vendor" while Tam is in range) and searched 600 cm vs the 450 cm trade range | **Applied at commit** — the loop now skips NPCs without `ShopLootTableId` + `CurrencyItemId` (`CheatManager.cpp:72-77`); the 600 cm search intentionally stays a grace margin wider than the 450 cm trade range |
| LOW (L-2) | NotEnoughCurrency cheat message names the currency | Committed `VendorResultMessage` is already currency-agnostic ("Not enough vendor currency for %d × %s.") — nothing hardcoded remains; genericize further only when a 2nd currency exists |
| LOW (L-3) | MP-scope note: `OnSprintExhausted`/`OnBlockingChanged` broadcast server-side only; `Stats`/`bIsBlocking` have no OnRep speed-refresh → remote clients keep a stale `MaxWalkSpeed` (rubber-band) until the next local refresh | **Accepted** (same class as REVIEW-3's accepted stagger note) — SP/listen-server target unaffected; folds into the H-12 MP RPC layer batch |
| LOW (L-4) | Doc drift outside the diff: SURVIVAL_SYSTEM.md:25 + PRODUCTION_AUDIT.md:75 still said thirst 0.14/s (~12 min) | **Fixed by this wave-6 docs sync** (both now read 0.0833/s ≈ 20 min) |
| LOW (L-5) | Pre-existing `AutomationTests.cpp:95` float `TestEqual` (`100×(1−0.65f) == 35.0f`) | Verified HARMLESS by IEEE-754 analysis (1.0f−0.65f exact via Sterbenz; ×100 lands 0.625 ULP above 35 → tie-rounds-to-even 35.0f) — no action |

### Not persisted / server-only (documented decisions)

- `bSprintDrainActive` is **server-side only** (not replicated) — it feeds the server stamina economy;
  the sprint SPEED itself is handled locally by `RefreshMovementSpeed` on each client.
- Vendor state is content, not world state: wares/stock/currency are registry definitions (shop stock =
  the `GuaranteedDrops` list of the NPC's `ShopLootTableId` — it never depletes in v1); only the player's
  owned items (including Dawn Shards in inventory stacks) persist through the existing save schema.
  Zero `Vendor`/`DawnShard` references in `SaveSubsystem` (grep-verified) — no schema bump.
- Dawn Shard keeps `VendorPrice = 0` by design: the currency can never be bought with itself or sold
  back — the only sources are the dungeon boss (×3), the Dawn Guard quest reward (×5) and the
  prototype starter kit (×10).

## Changes in the previous round (2026-08-30 — Wave 5 Batch 3: Status effects + Hit reactions (stagger) + Armor framework)

### Commits

| Commit | Type | Subject |
|---|---|---|
| `021f93a` | feat(batch-3) | Combat depth — status effects + hit reactions (stagger) + armor framework (22 files, +780/−15) |

### Repository totals (verified with shell commands on `021f93a`)

| Metric | Value | Command |
|---|---|---|
| C++ source files | **88** (43 `.cpp` + 45 `.h`) | `find Source -name '*.cpp' -o -name '*.h' \| wc -l` |
| C++ LOC | **15,413** | `find Source -name '*.cpp' -o -name '*.h' \| xargs wc -l \| tail -1` |
| Docs (`*.md`) | **49** | `ls Docs/*.md \| wc -l` |
| DOREPLIFETIME props | **28 across 9 classes** | `grep -c DOREPLIFETIME Source/AstrawildCore/Private/*.cpp` |
| Input actions | **19** | `MakeRuntimeAction` call sites in `AstrawildPlayerCharacter.cpp::BuildRuntimeInputDefaults` (no new keys in Batch 3) |
| Cheat commands | **13** | `UFUNCTION(Exec)` methods on `UAstrawildCheatManager` |
| Automation tests | **11** | `+ASTRAWILD.Equipment.ArmorMath` + `+ASTRAWILD.Combat.StatusEffectFactory` (both call production statics) |
| Content totals | **22 items · 13 recipes · 6 techs** | `ContentLibrary.cpp` log line: "22 items, 13 recipes, 7 Echo species, 10 buildings, 6 technologies, 6 quests, 2 loot tables, 2 NPCs" |

DOREPLIFETIME breakdown by class (verified by grep): `AAstrawildEchoCharacter` 7 (was 6 — `StatusEffects` added in Batch 3),
`UAstrawildInventoryComponent` 4 (was 3 — `EquippedArmorItemId` added in Batch 3), `AAstrawildBuildingActor` 4,
`AAstrawildGameState` 4, `AAstrawildEchoBossCharacter` 3, `UAstrawildCombatComponent` 2, `UAstrawildSurvivalComponent` 2,
`AAstrawildDungeonRoomActor` 1, `AAstrawildWorkSiteActor` 1.

### Systems added / changed in Batch 3

| System | Class / symbol | Status | Files |
|---|---|---|---|
| Element→status shared factory (static BlueprintPure): Ember→Burn (4 s DoT, DPS `2 + 5% × hit`), Frost→Chill (3 s, speed ×0.5), Flora→Poison (6 s, 2 DPS), Pulse→Shock (0.8 s, speed ×0.3); None/Light/Ash → nothing | `UAstrawildCombatComponent::MakeElementalStatusEffect(EAstrawildElementType, float SourceDamage)` (`CombatComponent.cpp:327-368`) | NEW | `AstrawildCombatComponent.h/.cpp` |
| Weapon element field + override — weapon `Element` overrides the `AttackElement` tunable when set; Dawn Crystal Blade = Pulse (tier-3 weapon Shocks) — **closes the weapon-element-override gap** | `UAstrawildItemDefinition::Element` + `UAstrawildCombatComponent::GetResolvedAttackElement()` | NEW field + CLOSED gap | `AstrawildDataAssets.h`, `AstrawildCombatComponent.h/.cpp`, `AstrawildContentLibrary.cpp` |
| Creature status container (replicated) + server tick: DoT on `CurrentHealth`, expiry, combined speed multiplier; **DoT defeats route through the FULL defeat pipeline (loot/events/quest credit)** | `AAstrawildEchoCharacter::StatusEffects` (`TArray<FAstrawildStatusEffect>`, `DOREPLIFETIME` line 63) + `AddStatusEffect`/`HasStatusEffect`/`GetStatusSpeedMultiplier` + private `ApplyStatusTicks` | NEW | `AstrawildEchoCharacter.h/.cpp` |
| Status application hooks — player weapon hits (element ≠ None) apply status to the creature; creature attacks apply the species element's status to the PLAYER | `EchoCharacter::ApplyElementalDamage` (after damage lands) + `EchoAIController::TryAttackTarget` player branch (`Survival->AddStatusEffect`) | NEW | `AstrawildEchoCharacter.cpp`, `AstrawildEchoAIController.cpp` |
| Player status speed integration + restore fix | `UAstrawildSurvivalComponent::GetStatusSpeedMultiplier()` (multiplicative) + `OnStatusEffectRemoved` delegate; `RefreshMovementSpeed` multiplies by the combined status slow; REVIEW-3 (M-2) fix — `FullRestore`/`SetStatsForRestore` broadcast removal before clearing (no stale slow after rest/load) | NEW + FIX INLINE | `AstrawildSurvivalComponent.h/.cpp`, `AstrawildPlayerCharacter.cpp` |
| Hit reactions — Echo stagger: server-only `ApplyStagger` (clamped ≤ 2 s) zeroes `MaxWalkSpeed` + `SetAIState(Staggered)`; heavy hits (≥ 20% of `GetMaxHealth()`) stagger 0.8 s; `EAstrawildEchoAIState::Staggered` appended AFTER `Dead` (serialization-safe); REVIEW-3 (M-1) fix — expiry explicitly restores `MaxWalkSpeed` | `AAstrawildEchoCharacter::ApplyStagger` / `StaggerRemainingSeconds` / `IsStaggered` (`EchoCharacter.cpp:319-331`, trigger at 386-389) | NEW + FIX INLINE | `AstrawildEchoCharacter.h/.cpp`, `AstrawildTypes.h` |
| AI stagger gate — `Think()` stops movement + skips Decide/Execute while staggered but STILL re-arms the think timer (naive early-return would kill the AI loop) | `AAstrawildEchoAIController::Think` (`EchoAIController.cpp:137-153`) | NEW | `AstrawildEchoAIController.cpp` |
| Player stagger — server-side countdown + `OnStaggerStateChanged` delegate; triggers: Echo hits ≥ 35 mitigated → 0.6 s; boss landed hits ALWAYS stagger 0.6 s; `RefreshMovementSpeed` zeroes while staggering | `UAstrawildCombatComponent::StaggerRemainingSeconds` / `ApplyStagger` / `IsStaggering` + `EchoBossCharacter::ExecuteAttack` | NEW | `AstrawildCombatComponent.h/.cpp`, `AstrawildEchoBossCharacter.cpp`, `AstrawildPlayerCharacter.cpp` |
| Armor framework — 3rd equipment slot `EquippedArmorItemId` (replicated, 28th replicated prop) + routing branch (before statless fallback) + `GetEquippedArmorRating` + `GetEquippedWeaponElement` + Unequip clears + additive `OnArmorChanged` delegate (2-param `OnEquipmentChanged` signature unchanged for BP stability) | `UAstrawildInventoryComponent` | NEW | `AstrawildInventoryComponent.h/.cpp` |
| Armor math — static pure `ComputeArmorFraction(Rating, K, MaxFraction) = Rating/(Rating+K)` clamped; tunables `ArmorConstantK=100` / `ArmorMaxFraction=0.6`; `GetMitigatedIncomingDamage` multiplies by `(1 − armor)` AFTER dodge/block | `UAstrawildCombatComponent::ComputeArmorFraction` / `GetEquippedArmorFraction` (`CombatComponent.cpp:387-398`) | NEW | `AstrawildCombatComponent.h/.cpp` |
| Armor save persistence — additive `EquippedArmorId` FName (no schema bump) + `HasItem`-guarded restore in the same block as weapon/shield | `UAstrawildSaveGame::EquippedArmorId` (`SaveSubsystem.h:56`, write `cpp:83`, restore `cpp:236-238`) | NEW | `AstrawildSaveSubsystem.h/.cpp` |
| Armor content — Fiberweave Vest (rating 20, 3.0 kg, 4 s craft), Emberhide Jacket (45, 5.0 kg, 6 s), Crystalplate Cuirass (80, 8.0 kg, 9 s) — all `Tech_Armory` + workbench; `Tech_Armory` now unlocks 5 recipes; items 19→**22**, recipes 10→**13** | `ContentLibrary.cpp:184-194, 250-260` | NEW | `AstrawildContentLibrary.cpp` |
| HUD equipment readout + armor segment; `EquipBest` also picks the best armor by `ArmorRating` | `AstrawildHudWidget.cpp:283-288`, `PlayerCharacter::EquipBest` (`cpp:730-765`) | UPDATE | `AstrawildHudWidget.cpp`, `AstrawildPlayerCharacter.cpp` |
| New gameplay tags | `TAG_Astrawild_Status_Chilled` / `_Shocked` / `_Staggered` + `TAG_Astrawild_State_Creature_Staggered` | NEW | `AstrawildGameplayTags.h/.cpp` |
| Tests — 2 REAL automation tests replacing tautological coverage (both call production statics); pre-existing float-unsafe `TestEqual` converted to tolerance-based `TestTrue` | `FAstrawildArmorMathTest` (ArmorMath, `AutomationTests.cpp:217`) + `FAstrawildStatusEffectFactoryTest` (StatusEffectFactory, `:261`) — **11 total** | NEW | `AstrawildAutomationTests.cpp` |

### REVIEW-3 findings (caught and fixed before commit `021f93a`)

| Severity | Finding | Fix that landed in `021f93a` |
|---|---|---|
| **HIGH (compile blocker, pre-existing since wave 3 `2eeedf8`)** | `AstrawildHudWidget.cpp` calls members on `UAstrawildInventoryComponent` (`EquippedItemId` / `GetEquippedWeaponAttackPower()` / `EquippedShieldItemId` — plus new Batch-3 `EquippedArmorItemId` / `GetEquippedArmorRating()`) while the type is only forward-declared in that TU — MSVC C2027/C2079 | Added `#include "AstrawildInventoryComponent.h"` at `AstrawildHudWidget.cpp:10` (REVIEW-1/REVIEW-2 missed it because the offending block predates both batches and the project has never been compiled in-sandbox) |
| **MEDIUM (runtime)** | Echo stagger expiry only ran `SetAIState(Idle)`; the Tick speed recompute is gated on the STATUS multiplier changing, which stagger does not touch — with no speed status active (default Ash element), `MaxWalkSpeed` stayed 0 permanently → every heavy hit ≥ 20% max HP permanently froze the creature | Expiry branch now explicitly restores `Movement->MaxWalkSpeed = CachedStats.MoveSpeed × GetStatusSpeedMultiplier()` (`EchoCharacter.cpp:106-119`, REVIEW-3 M-1 comment) |
| **MEDIUM (runtime)** | `SurvivalComponent::FullRestore` / `SetStatsForRestore` called `StatusEffects.Reset()` without broadcasting `OnStatusEffectRemoved` → a Chilled/Shocked player kept the stale halved speed after resting at a RestPoint, after QuickLoad, or the `AW.FullRestore` cheat | Both paths now broadcast removal per effect BEFORE `Reset()` (`SurvivalComponent.cpp:192-194, 221-225`, REVIEW-3 M-2 comments) |

### Not persisted (documented decision)

Status effects are **transient combat state** — they are deliberately NOT written to `UAstrawildSaveGame`
(grep-verified: zero `StatusEffect` references in `SaveSubsystem.h/.cpp`). A save/load or full restore
resets both player and creature statuses; the M-2 fix guarantees the removal broadcasts fire so no
stale slow survives the reset.

## Changes in the previous round (2026-08-30 — Wave 4 Batch 2: Hostile respawn + Building dismantle + Power persistence)

### Commits

| Commit | Type | Subject |
|---|---|---|
| `6f14520` | fix(bootstrapper) | Add missing `Engine/StaticMeshActor.h` include — REVIEW-1 compile blocker for `SpawnActor<AStaticMeshActor>` |
| `d5d23c2` | feat(batch-2) | Hostile respawn + building dismantle + power persistence |

### Repository totals (verified with shell commands on `d5d23c2`)

| Metric | Value | Command |
|---|---|---|
| C++ source files | **88** | `find Source -name '*.cpp' -o -name '*.h' \| wc -l` |
| C++ LOC | **14,648** | `find Source -name '*.cpp' -o -name '*.h' \| xargs wc -l \| tail -1` |
| Docs (`*.md`) | **49** | `ls Docs/*.md \| wc -l` |
| DOREPLIFETIME props | **26 across 9 classes** | `grep -c DOREPLIFETIME Source/AstrawildCore/Private/*.cpp` |
| Input actions | **19** | `MakeRuntimeAction` call sites in `AstrawildPlayerCharacter.cpp::BuildRuntimeInputDefaults` |
| Cheat commands | **13** | `UFUNCTION(Exec)` methods on `UAstrawildCheatManager` |
| Automation tests | **9** | `ASTRAWILD.Equipment.ProgressionMath` + 8 prior |

DOREPLIFETIME breakdown by class (verified by grep): `AAstrawildBuildingActor` 4 (was 3 — `bIsPowered` added in Batch 2), `AAstrawildEchoCharacter` 6, `AAstrawildGameState` 4, `AAstrawildEchoBossCharacter` 3, `UAstrawildInventoryComponent` 3, `UAstrawildCombatComponent` 2, `UAstrawildSurvivalComponent` 2, `AAstrawildDungeonRoomActor` 1, `AAstrawildWorkSiteActor` 1.

### Systems added / changed in Batch 2

| System | Class / symbol | Status | Files |
|---|---|---|---|
| Hostile respawn (server-only `UTickableWorldSubsystem`, 25 s sweep, ring-biased outward 30–100 % of `SpawnRadius=1800 cm`, `FRandomStream` seeded from `WorldSeed`) | `UAstrawildHostileSpawnerSubsystem` (Tick / `OnWorldBeginPlay` / `SpawnOneHostile`) | NEW | `Source/AstrawildCore/Public/AstrawildHostileSpawnerSubsystem.h`, `Source/AstrawildCore/Private/AstrawildHostileSpawnerSubsystem.cpp` |
| Building dismantle (server-authoritative, weight-safe — refuses if `CanAddItem` fails, refunds via `AddItemSilent` then `Destroy()`) | `UAstrawildBuildingComponent::DismantleBuilding(AActor*)` | NEW | `AstrawildBuildingComponent.h/.cpp` |
| Silent refund (structurally identical to `AddItem` minus the EventBus publish block — dismantling materials do NOT advance `CollectItem` quest objectives) | `UAstrawildInventoryComponent::AddItemSilent(FName, int32)` | NEW | `AstrawildInventoryComponent.h/.cpp` |
| Delete-building input action (mirrors `EquipBest` wiring; 5 m crosshair trace via `FollowCamera + LineTraceSingleByChannel ECC_Visibility`) | `AAstrawildPlayerCharacter::DeleteBuildingAction` (TObjectPtr<UInputAction>) bound to `EKeys::Z`, handler `DeleteBuilding` | NEW | `AstrawildPlayerCharacter.h/.cpp` |
| Building power-state field (replicated, BlueprintReadOnly) | `AAstrawildBuildingActor::bIsPowered` (UPROPERTY Replicated + `DOREPLIFETIME` line) | NEW field | `AstrawildBuildingActor.h/.cpp` |
| Save-data additive power flag (default `false` → old saves deserialize fine; re-resolved on first `ResolveGridNow()` call during `LoadWorld`) | `FAstrawildBuildingSaveData::bIsPowered` (Types.h:465) | NEW field | `AstrawildTypes.h` |
| Public wrapper for private `ResolveGrid()` — called once during `LoadWorld` after the building spawn loop so the first frame after load is correct (no 2 s brownout flicker) | `UAstrawildPowerSubsystem::ResolveGridNow()` (UFUNCTION BlueprintCallable) | NEW | `AstrawildPowerSubsystem.h/.cpp` |
| Save/load integration — `LoadWorld` calls `Power->ResolveGridNow()` right after the building spawn loop (buildings have BeginPlay'd + RegisterPower'd via `FromSaveData → InitializeFromDefinition`) | `UAstrawildSaveSubsystem::LoadWorld` | UPDATE | `AstrawildSaveSubsystem.cpp` |
| `ResolveGrid` writes back `Consumer->bIsPowered = bPowered` every 2 s tick (UE net driver short-circuits unchanged values → no extra bandwidth) | `UAstrawildPowerSubsystem::ResolveGrid` | UPDATE | `AstrawildPowerSubsystem.cpp` |
| `ToSaveData` captures `Power->IsBuildingPowered(this)` at save time; `FromSaveData` restores the hint value (overwritten on next `ResolveGrid`) | `AAstrawildBuildingActor::ToSaveData` / `FromSaveData` | UPDATE | `AstrawildBuildingActor.cpp` |
| REVIEW-2 fix — re-`RegisterEcho` immediately after `Echo->InitializeFromDefinition(Definition)` so the species `WildCount` bumps (BeginPlay registered before `EchoDefinition` was set → WildCount bump was being skipped) | `UAstrawildHostileSpawnerSubsystem::SpawnOneHostile` (line 137-140) | FIX INLINE | `AstrawildHostileSpawnerSubsystem.cpp` |

### Quest chain impact

Quest 5 ("Defeat 3 Gloomfang", `Quest_DawnGuard` objective `DefeatCreature TargetId=Echo_Gloomfang RequiredCount=3`) now chain-completes organically: the spawner keeps `Echo_Gloomfang` population at `TargetGloomfangPopulation=4` around the player pawn every 25 s; the existing death pipeline (`EchoCharacter::ApplyElementalDamage → OnDefeated → EventBus TAG_Astrawild_Event_HostileDefeated → QuestComponent::ApplyEventToQuest`) auto-increments the kill counter. No new quest wiring was required.

## Changes in the previous round (2026-08-30 — Production Audit + Batch 1: Core Loop Unblocked)

### Audit deliverables (evidence-based, line-cited)

- `Docs/ASTRAWILD_UE5_PRODUCTION_AUDIT.md` — every Checklist V2 section audited with file:line evidence;
  system classification KEEP/REFACTOR/ADD matrix; honest compile/runtime status
- `Docs/ASTRAWILD_IMPLEMENTATION_GAP_REPORT.md` — 8 CRITICAL / 14 HIGH / 14 MEDIUM / 9 LOW gaps
- `Docs/ASTRAWILD_ULTIMATE_GAP_ANALYSIS.md` — 12-category analysis vs Roadmap V3 with acceptance criteria

### Batch 1 fixes (dependency-ordered; all server-authoritative, zero-asset compatible)

| ID | Fix | Files |
|---|---|---|
| C-1 | Crafting station compile error (TPair → element iteration) | `AstrawildCraftingStationActor.cpp` |
| C-1b | Latent compile error: `MakeRuntimeAction` missing param default | `AstrawildPlayerCharacter.h` |
| C-2 | Research unlock path: free root techs auto-granted at session start + **Research Desk is now interactable** (E spends pooled RP on cheapest unlockable tech) + HUD research readout + `GetNextUnlockableTechId`/`GrantStartingTechnologies` APIs + registry `GetAllTechnologies` | `AstrawildResearchSubsystem`, `AstrawildGameMode`, `AstrawildBuildingActor` (now `IAstrawildInteractable`), `AstrawildItemRegistrySubsystem`, `AstrawildHudWidget` |
| C-3 | **Runtime navmesh**: `UNavigationInvokerComponent` on player (120m/160m radii) + every Echo (50m/70m) + `bGenerateNavigationOnlyAroundNavigationInvokers=True` config + bootstrapper build kick — creature pathfinding now possible in the zero-asset world | `AstrawildPlayerCharacter`, `AstrawildEchoCharacter`, `Config/DefaultEngine.ini`, `AstrawildWorldBootstrapper` |
| C-4 | Dungeon completion: empty rooms (entry) auto-clear; boss counts toward clear; completion grants +10 RP (first completion consumer) | `AstrawildDungeonGeneratorActor`, `AstrawildDungeonRoomActor` |
| C-5 | **Phased boss finally spawns** in the boss room (was fully-coded dead code); boss damage now routes through player mitigation (dodge/block apply) | `AstrawildDungeonRoomActor`, `AstrawildEchoBossCharacter` |
| C-6 | Build piece cycling: mouse-wheel action + HUD build readout (piece name, index/count, control hints); signed wrap-around cycling | `AstrawildPlayerCharacter`, `AstrawildBuildingComponent`, `AstrawildHudWidget` |
| C-7 | Work sites interactable: **E collects stored output** (weight-gate safe, publishes ItemCollected) or **assigns nearest idle captured Echo**; dynamic prompts — the automation loop is reachable in play | `AstrawildWorkSiteActor` (now `IAstrawildInteractable`) |
| C-8 | Respawn input death fixed: input context (re)bound on every `PossessedBy` (idempotent remove-then-add) | `AstrawildPlayerCharacter` |
| H-1 | Saved vitals now restored on load (`SetStatsForRestore`, clamp-safe, never load-dead) instead of `FullRestore()` | `AstrawildSurvivalComponent`, `AstrawildSaveSubsystem` |
| H-2 | Captured party **respawns around the player on load** (`SpawnPartyActors`); redundant double roster import removed | `AstrawildEchoRosterSubsystem`, `AstrawildSaveSubsystem` |
| H-3 | `LoadLatest()` — F9/cheat/optional boot-continue now load the NEWEST slot (autosave was previously write-only) | `AstrawildSaveSubsystem`, `AstrawildPlayerCharacter`, `AstrawildCheatManager`, `AstrawildGameMode` (`bAutoLoadLatestOnBeginPlay`, default off) |
| H-4 | Weather `GetProfile` dangling-reference UB → returns by value | `AstrawildWeatherSubsystem` |
| H-5 | Building damage persists across load (health applied AFTER definition re-init) | `AstrawildBuildingActor` |
| H-7 | Captured "Attack" command can no longer target the owner (owner exclusion in target acquisition) | `AstrawildEchoAIController` |
| H-8 | `OnAIStateChanged` now actually broadcasts (public `SetAIState`, controller routes through it) | `AstrawildEchoCharacter`, `AstrawildEchoAIController` |
| M-4 | HUD shows real ambient temperature (was hardcoded 20 °C) | `AstrawildHudWidget` |
| M-12 | Missing native tags added: `Element.Ember`, `State.Creature.Injured`, `State.Creature.Dead` | `AstrawildGameplayTags` |
| — | `PlayerController::Notify` wires gameplay feedback to the HUD notification line (research/work results) | `AstrawildPlayerController` |
| — | Dungeon room shells rebuild with real template extents (BeginPlay ordering fix) | `AstrawildDungeonRoomActor` |

**Vertical-slice loop after Batch 1 (pending engine compile):**
Start → gather → craft (station path compiles) → build (any unlocked piece, wheel-cycled) →
research (auto root tech + Research Desk E) → power (Tech_Electrical now reachable) →
work (site interact) → dungeon (completes) → phased boss → save (F5) → quit → load (F9,
newest slot, vitals + party + buildings restored) → continue. **No cheats required.**

## Changes in the previous round (2026-08-30 — content wave 3: equipment progression, NPCs, loot tables + docs sync)

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
| Automation suite (12 tests) | NOT_RUN | Run via Session Frontend, filter `ASTRAWILD` (Batch 3 added `ASTRAWILD.Equipment.ArmorMath` + `ASTRAWILD.Combat.StatusEffectFactory`; Batch 4 added `ASTRAWILD.Economy.VendorSellValue`) |
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
| Low | Log-line count drift | PlayerCharacter.cpp / ContentLibrary.cpp | Read logs | Resolved for now: log says "19 actions" / "23 items, 13 recipes, … 2 loot tables, 2 NPCs" and matches the code |
| ~~Low~~ Closed | NPC vendor purchase logic | `AstrawildNPCCharacter` | Talk to Trader Tam | **CLOSED in Batch 4 (`c16fecd`)** — `TryPurchase`/`TrySell` server-authoritative transaction flow live (450 cm trade range, Dawn Shard currency, wares listed in an Interact HUD toast; buy/sell via `AW.BuyItem`/`AW.SellItem`). The shop **UMG screen** remains open (future round). Compile pending on target machine |
| Medium | H-9 / H-12 RPC layer for multiplayer | `PlayerCharacter.cpp` / `EchoCharacter.cpp` | 2-PIE capture / eat / feed / equip / command | **Pending — MP batch.** Single-player only at present — Item B `DismantleBuilding` and the Batch-4 vendor transactions both use direct method calls gated on `GetLocalRole() == ROLE_Authority` (fine for SP/listen-server; a future shop UI for remote clients must route through a Server RPC — noted in `NPCCharacter.cpp:132-134`) |
| ~~Medium~~ Closed | Weapon element override (player attack element hardcoded Ash) | `CombatComponent.h` `GetResolvedAttackElement` | Hit a creature with a Crystal Blade | **CLOSED in Batch 3 (`021f93a`)** — weapon `Element` overrides the tunable; Dawn Crystal Blade = Pulse → Shock. Compile pending on target machine |
| Low | `HostileSpawnerSubsystem::Tick` requires server world | `AstrawildHostileSpawnerSubsystem.cpp:52` | Clients never run the spawn sweep | Clients see populated hostiles via replication only — server authoritative; matches the rest of the simulation |
| Low | `PowerSubsystem::ResolveGridNow` is server-only | `AstrawildPowerSubsystem.cpp:28-33` | Clients never run `ResolveGrid` | Clients receive correct state via the new `bIsPowered` replicated UPROPERTY on `AAstrawildBuildingActor` — `ResolveGrid`'s server-only early return (`World->GetNetMode() == NM_Client`) at `.cpp:55` guards the path |

## Handoff to Antigravity

The C++ core (single module `AstrawildCore`, **~16.0k LOC, 88 source files**), the zero-asset playability
layer (procedural world + runtime input + C++ HUD), save schema v2 (with wave 3 equipment persistence +
wave 4 building power persistence + wave 5 armor-slot persistence), the combat-depth layer (status
effects + stagger + armor), the survival-feel layer (sprint stamina drain + live block penalty +
aligned thirst rate), the vendor economy layer (Dawn Shard currency + buy/sell transactions + cheats),
the CODE_DEFAULT content set (23 items / 13 recipes / 7 species /
10 buildings / 6 techs / 6 quests / 2 loot tables / 2 NPCs), the documentation suite, the test plan, and
the asset manifest/replacement pipeline are all in the repository. Antigravity must: **pull, generate
project files, compile `ASTRAWILDEditor Win64 Development`, run the 12 automation tests, execute the
17-step first-playable checklist, and fill this report with real results.** Do not mark `COMPLETE` until
Compile, the automation suite, the core-loop Playtest, and Save/Load have all passed (see
`Docs/ASTRAWILD_DEFINITION_OF_DONE.md`).

**Handoff tally:** 88 C++ files / 15,990 LOC / 49 docs / 12 automation tests / 28 replicated props across
9 classes / 19 input actions / 15 console cheats. Compile status: `NOT RUN (sandbox has no UE engine —
must be verified on UE 5.8 + Antigravity target machine).`
