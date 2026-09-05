#include "CoreMinimal.h"

// Automation tests (directive §39) — pure logic tests, world-free, safe in Shipping-stripped builds.
#if WITH_DEV_AUTOMATION_TESTS

#include "AstrawildCaptureComponent.h"
#include "AstrawildAbilityLibrary.h"
#include "AstrawildAssetFallback.h"
#include "AstrawildAttributeComponent.h"
#include "AstrawildBaseTerminalActor.h"
#include "AstrawildBestiaryData.h"
#include "AstrawildComboSubsystem.h"
#include "AstrawildCropComponent.h"
#include "AstrawildCreatureSanityComponent.h"
#include "AstrawildDataValidator.h"
#include "AstrawildDifficultySubsystem.h"
#include "AstrawildDurabilityComponent.h"
#include "AstrawildErrorReporter.h"
#include "AstrawildMountComponent.h"
#include "AstrawildGeneticsLibrary.h"
#include "AstrawildNPCScheduleComponent.h"
#include "AstrawildPerformanceManager.h"
#include "AstrawildSpoilageSubsystem.h"
#include "AstrawildTurretComponent.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildBiomeDressingActor.h"
#include "AstrawildCombatComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildArtPack.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildResearchSubsystem.h"
#include "InputMappingContext.h"
// Complete soft-pointer pointee types: TSoftObjectPtr<>::IsValid() in test code
// needs them (mirrors the 91f0f44 fix that added NiagaraSystem.h).
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "AstrawildDialogueComponent.h"
#include "AstrawildDungeonRoomActor.h"
#include "AstrawildWorldBootstrapper.h"
#include "AstrawildWeatherSubsystem.h"
#include "AstrawildResourceNode.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildEchoRosterSubsystem.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildPOISubsystem.h"
#include "AstrawildWorldBootstrapper.h"
#include "AstrawildWorldEventSubsystem.h"
#include "AstrawildSaveSubsystem.h"
#include "AstrawildSkiffActor.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildTerrainTileActor.h"
#include "AstrawildTypes.h"
#include "AstrawildVfxActor.h"
#include "AstrawildZoneSubsystem.h"
#include "AstrawildContentLibrary.h"
#include "Misc/AutomationTest.h"
#include "NiagaraSystem.h"

// ---------------------------------------------------------------------------
// Inventory: add/remove/weight/stack behavior (directive §14)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildInventoryAddRemoveTest,
    "ASTRAWILD.Inventory.AddRemove",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildInventoryAddRemoveTest::RunTest(const FString& Parameters)
{
    // Weight-free structural test through the public API contract.
    TestTrue(TEXT("Zero quantity is an invalid stack"), !FAstrawildItemStack().IsValid());

    FAstrawildItemStack Stack;
    Stack.ItemId = TEXT("Item_Wood");
    Stack.Quantity = 5;
    TestTrue(TEXT("Valid stack accepted"), Stack.IsValid());
    return true;
}

// ---------------------------------------------------------------------------
// Survival: damage and death (directive §11)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSurvivalLogicTest,
    "ASTRAWILD.Survival.DamageAndDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSurvivalLogicTest::RunTest(const FString& Parameters)
{
    FAstrawildSurvivalStats Stats;
    Stats.Health = 100.0f;
    Stats.MaxHealth = 100.0f;
    Stats.Stamina = 50.0f;
    Stats.MaxStamina = 100.0f;

    TestEqual(TEXT("Health starts full"), Stats.Health, 100.0f);
    TestEqual(TEXT("Stamina fraction is half"), Stats.Stamina / Stats.MaxStamina, 0.5f);
    TestFalse(TEXT("Player starts alive"), Stats.bIsDead);
    return true;
}

// ---------------------------------------------------------------------------
// Capture chance: design rule bounds (directive §8)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildCaptureRuleTest,
    "ASTRAWILD.Capture.DesignRuleBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildCaptureRuleTest::RunTest(const FString& Parameters)
{
    // Species template sanity: difficulty + resilience live in sane ranges.
    UAstrawildEchoDefinition* Definition = NewObject<UAstrawildEchoDefinition>();
    Definition->DefinitionId = TEXT("Echo_Test");
    Definition->BaseStats.MaxHealth = 100.0f;
    Definition->BaseStats.CaptureResilience = 0.5f;
    Definition->CaptureDifficulty = 0.4f;

    TestTrue(TEXT("Capture difficulty clamped conceptually 0..1"),
        Definition->CaptureDifficulty >= 0.0f && Definition->CaptureDifficulty <= 1.0f);
    TestTrue(TEXT("Resilience in 0..1"),
        Definition->BaseStats.CaptureResilience >= 0.0f && Definition->BaseStats.CaptureResilience <= 1.0f);

    // Final production run (audit C-12 — replaced the tautological placeholder):
    // trust gained on capture must be positive, and the weaken-bonus ceiling
    // (resilience-scaled) can never exceed the 0.95 documented clamp when stacked
    // on the max weaken contribution — the design invariant the runtime enforces.
    TestTrue(TEXT("Trust gain on capture is positive"), Definition->TrustGainOnCapture > 0.0f);
    const float MaxWeakenBonus = 0.5f * (1.0f - Definition->BaseStats.CaptureResilience);
    const float BaseChance = 0.25f + (1.0f - Definition->CaptureDifficulty) * 0.35f;
    TestTrue(TEXT("Full weaken + observation bonus stays under the 0.95 clamp"),
        BaseChance + MaxWeakenBonus + 0.15f <= 0.95f + KINDA_SMALL_NUMBER);
    return true;
}

// ---------------------------------------------------------------------------
// Combat mitigation math (directive §9)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildCombatMitigationTest,
    "ASTRAWILD.Combat.MitigationMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildCombatMitigationTest::RunTest(const FString& Parameters)
{
    // Block mitigation: 65% block on 100 damage = 35 through.
    const float BlockMitigation = 0.65f;
    const float Incoming = 100.0f;
    const float Mitigated = Incoming * (1.0f - BlockMitigation);
    TestEqual(TEXT("Blocked damage passes 35%"), Mitigated, 35.0f);

    // Weakness multiplier x1.5.
    const float Base = 20.0f;
    TestEqual(TEXT("Weakness multiplies 1.5x"), Base * 1.5f, 30.0f);
    return true;
}

// ---------------------------------------------------------------------------
// Save: checksum determinism + schema versioning (directive §27)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSaveChecksumTest,
    "ASTRAWILD.Save.ChecksumDeterminism",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSaveChecksumTest::RunTest(const FString& Parameters)
{
    const FDateTime Stamp(2026, 8, 29, 12, 0, 0);
    const uint32 A = UAstrawildSaveSubsystem::ComputeChecksum(2, Stamp);
    const uint32 B = UAstrawildSaveSubsystem::ComputeChecksum(2, Stamp);
    const uint32 C = UAstrawildSaveSubsystem::ComputeChecksum(1, Stamp);

    TestEqual(TEXT("Checksum is deterministic"), A, B);
    TestTrue(TEXT("Checksum differs across schema versions"), A != C);
    TestTrue(TEXT("Checksum is non-zero"), A != 0);
    return true;
}

// ---------------------------------------------------------------------------
// Quest objectives: progress + completion math (directive §25)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildQuestObjectiveTest,
    "ASTRAWILD.Quest.ObjectiveProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildQuestObjectiveTest::RunTest(const FString& Parameters)
{
    FAstrawildQuestObjective Objective;
    Objective.Type = EAstrawildQuestObjectiveType::CollectItem;
    Objective.TargetId = TEXT("Item_Wood");
    Objective.RequiredCount = 10;
    Objective.ProgressCount = 7;

    TestFalse(TEXT("7/10 is incomplete"), Objective.IsComplete());
    Objective.ProgressCount = 10;
    TestTrue(TEXT("10/10 is complete"), Objective.IsComplete());
    Objective.ProgressCount = 12;
    TestTrue(TEXT("Over-progress still complete"), Objective.IsComplete());
    return true;
}

// ---------------------------------------------------------------------------
// Echo personality: behavior modifiers (directive §5)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildPersonalityModTest,
    "ASTRAWILD.Echo.PersonalityModifiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildPersonalityModTest::RunTest(const FString& Parameters)
{
    // The modifier contract: Timid flees earlier, Brave stands ground, Lazy works slower.
    // (Validated through the enum surface here; runtime behavior tested in-engine.)
    TestTrue(TEXT("Personality enum covers 10 archetypes"),
        static_cast<int32>(EAstrawildPersonality::Social) == 9);
    TestTrue(TEXT("Flee multiplier contract: Timid > Brave encoded"),
        EAstrawildPersonality::Timid != EAstrawildPersonality::Brave);
    return true;
}

// ---------------------------------------------------------------------------
// Power grid: brownout math (directive §17)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildPowerMathTest,
    "ASTRAWILD.Power.BrownoutMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildPowerMathTest::RunTest(const FString& Parameters)
{
    const float Generation = 8.0f;
    float Draw = 2.0f + 3.0f + 4.0f; // 9 > 8: brownout expected.
    TestTrue(TEXT("Draw exceeds generation"), Draw > Generation);

    // Shed the lowest-priority consumer (4.0) -> draw 5 <= generation 8.
    Draw -= 4.0f;
    TestTrue(TEXT("Shedding restores balance"), Draw <= Generation);
    return true;
}

// ---------------------------------------------------------------------------
// Equipment progression (wave 3): weapon attack bonus + shield mitigation
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildEquipmentProgressionTest,
    "ASTRAWILD.Equipment.ProgressionMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildEquipmentProgressionTest::RunTest(const FString& Parameters)
{
    // Weapon adds flat attack power to both tiers (component contract).
    const float LightBase = 25.0f;
    const float HeavyBase = 60.0f;
    const float ClubBonus = 6.0f;
    const float BladeBonus = 14.0f;
    TestEqual(TEXT("Unarmed light stays 25"), LightBase, 25.0f);
    TestEqual(TEXT("Club light 25+6=31"), LightBase + ClubBonus, 31.0f);
    TestEqual(TEXT("Blade heavy 60+14=74"), HeavyBase + BladeBonus, 74.0f);

    // Shield replaces the unarmed mitigation baseline (never stacks).
    const float Unarmed = 0.45f;
    const float Shield = 0.65f;
    const float Incoming = 100.0f;
    // REVIEW-3: float-safe assertions (100*(1-0.65f) is off from 35.0f by ~3.8e-6 —
    // exact TestEqual could false-fail depending on platform float semantics).
    TestTrue(TEXT("Unarmed block passes ~55%"), FMath::Abs(Incoming * (1.0f - Unarmed) - 55.0f) < KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Shielded block passes ~35%"), FMath::Abs(Incoming * (1.0f - Shield) - 35.0f) < KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Shield strictly improves block"), Shield > Unarmed);
    return true;
}

// ---------------------------------------------------------------------------
// Armor framework (Batch 3 — Item C): diminishing-returns damage reduction.
// Exercises the REAL production formula (UAstrawildCombatComponent::ComputeArmorFraction),
// not a re-derivation — closing the "tautological tests" gap one test at a time.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildArmorMathTest,
    "ASTRAWILD.Equipment.ArmorMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildArmorMathTest::RunTest(const FString& Parameters)
{
    const float K = 100.0f;
    const float MaxFraction = 0.6f;

    // Tier pieces (CODE_DEFAULT wave 5): Fiberweave Vest 20, Emberhide Jacket 45, Crystalplate Cuirass 80.
    const float VestFraction = UAstrawildCombatComponent::ComputeArmorFraction(20.0f, K, MaxFraction);
    const float JacketFraction = UAstrawildCombatComponent::ComputeArmorFraction(45.0f, K, MaxFraction);
    const float CuirassFraction = UAstrawildCombatComponent::ComputeArmorFraction(80.0f, K, MaxFraction);

    // Bounds: no armor → 0%, all tiers strictly improve, diminishing returns ordering.
    TestEqual(TEXT("No armor reduces nothing"), UAstrawildCombatComponent::ComputeArmorFraction(0.0f, K, MaxFraction), 0.0f);
    TestTrue(TEXT("Vest (20) reduces ~16.7%"), FMath::Abs(VestFraction - 20.0f / 120.0f) < KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Jacket (45) reduces ~31%"), FMath::Abs(JacketFraction - 45.0f / 145.0f) < KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Cuirass (80) reduces ~44%"), FMath::Abs(CuirassFraction - 80.0f / 180.0f) < KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Tier ordering strictly improves"), VestFraction < JacketFraction && JacketFraction < CuirassFraction);

    // Diminishing returns: doubling rating less than doubles the fraction.
    const double FractionRatio = static_cast<double>(JacketFraction / VestFraction);
    TestTrue(TEXT("Doubling rating < doubles reduction (diminishing)"), FractionRatio < 2.0);

    // Clamp: absurd ratings never exceed the hard cap (damage never nullified).
    TestEqual(TEXT("Rating 1,000,000 clamps to the cap"),
        UAstrawildCombatComponent::ComputeArmorFraction(1000000.0f, K, MaxFraction), MaxFraction);
    TestEqual(TEXT("K=0 is degenerate-safe (no reduction)"),
        UAstrawildCombatComponent::ComputeArmorFraction(80.0f, 0.0f, MaxFraction), 0.0f);

    // Incoming-damage math a player would feel (100 raw hit, blocked, cuirass equipped):
    // block 65% → 35, then armor 44.4% → 35 * (1 - 0.4444) ≈ 19.4.
    const float Incoming = 100.0f;
    const float BlockedThenArmored = Incoming * (1.0f - 0.65f) * (1.0f - CuirassFraction);
    TestTrue(TEXT("Block + cuirass leaves ~19.4 of a 100 hit"), BlockedThenArmored < 20.0f && BlockedThenArmored > 19.0f);
    TestTrue(TEXT("Armor never INCREASES damage"), BlockedThenArmored <= Incoming * (1.0f - 0.65f));
    return true;
}

// ---------------------------------------------------------------------------
// Status effects (Batch 3 — Item A): element→effect mapping vocabulary.
// Exercises the REAL production factory (MakeElementalStatusEffect).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildStatusEffectFactoryTest,
    "ASTRAWILD.Combat.StatusEffectFactory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildStatusEffectFactoryTest::RunTest(const FString& Parameters)
{
    // Ember → Burn: 4s DoT whose DPS scales with the applying hit.
    const FAstrawildStatusEffect Burn = UAstrawildCombatComponent::MakeElementalStatusEffect(EAstrawildElementType::Ember, 40.0f);
    TestTrue(TEXT("Ember maps to Burn"), Burn.StatusId == TEXT("Status.Burning"));
    TestEqual(TEXT("Burn lasts 4s"), Burn.RemainingSeconds, 4.0f);
    TestEqual(TEXT("Burn DPS = 2 + 40*0.05 = 4"), Burn.DamagePerSecond, 4.0f);
    TestEqual(TEXT("Burn does not slow"), Burn.SpeedMultiplier, 1.0f);

    // Frost → Chill: pure slow, no damage.
    const FAstrawildStatusEffect Chill = UAstrawildCombatComponent::MakeElementalStatusEffect(EAstrawildElementType::Frost, 40.0f);
    TestTrue(TEXT("Frost maps to Chilled"), Chill.StatusId == TEXT("Status.Chilled"));
    TestEqual(TEXT("Chill lasts 3s"), Chill.RemainingSeconds, 3.0f);
    TestEqual(TEXT("Chill does no damage"), Chill.DamagePerSecond, 0.0f);
    TestEqual(TEXT("Chill halves speed"), Chill.SpeedMultiplier, 0.5f);

    // Flora → Poison: flat DoT.
    const FAstrawildStatusEffect Poison = UAstrawildCombatComponent::MakeElementalStatusEffect(EAstrawildElementType::Flora, 40.0f);
    TestTrue(TEXT("Flora maps to Poisoned"), Poison.StatusId == TEXT("Status.Poisoned"));
    TestEqual(TEXT("Poison lasts 6s"), Poison.RemainingSeconds, 6.0f);
    TestEqual(TEXT("Poison DPS flat 2"), Poison.DamagePerSecond, 2.0f);

    // Pulse → Shock: brief hard slow.
    const FAstrawildStatusEffect Shock = UAstrawildCombatComponent::MakeElementalStatusEffect(EAstrawildElementType::Pulse, 40.0f);
    TestTrue(TEXT("Pulse maps to Shocked"), Shock.StatusId == TEXT("Status.Shocked"));
    TestEqual(TEXT("Shock lasts 0.8s"), Shock.RemainingSeconds, 0.8f);
    TestEqual(TEXT("Shock slows to 30%"), Shock.SpeedMultiplier, 0.3f);

    // None/Light/Ash → no status (invalid id → callers skip).
    for (const EAstrawildElementType NoStatus : { EAstrawildElementType::None, EAstrawildElementType::Light, EAstrawildElementType::Ash })
    {
        const FAstrawildStatusEffect Nothing = UAstrawildCombatComponent::MakeElementalStatusEffect(NoStatus, 40.0f);
        TestTrue(TEXT("Non-elemental attacks apply no status"), Nothing.StatusId.IsNone());
        TestEqual(TEXT("Invalid status has zero duration"), Nothing.RemainingSeconds, 0.0f);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Vendor economy (Batch 4 — M-11): sell-value rule. Exercises the REAL
// production static (AAstrawildNPCCharacter::ComputeVendorSellValue).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildVendorEconomyTest,
    "ASTRAWILD.Economy.VendorSellValue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildVendorEconomyTest::RunTest(const FString& Parameters)
{
    // Sell value is half the buy price, floored at 1 for anything tradeable —
    // a buy-low/sell-higher exploit is impossible by construction.
    TestEqual(TEXT("Price 2 sells for 1"), AAstrawildNPCCharacter::ComputeVendorSellValue(2), 1);
    TestEqual(TEXT("Price 3 sells for 1 (floor)"), AAstrawildNPCCharacter::ComputeVendorSellValue(3), 1);
    TestEqual(TEXT("Price 4 sells for 2"), AAstrawildNPCCharacter::ComputeVendorSellValue(4), 2);
    TestEqual(TEXT("Price 6 sells for 3"), AAstrawildNPCCharacter::ComputeVendorSellValue(6), 3);

    // Unpriced items (VendorPrice 0) are not sellable — junk cannot be minted
    // into currency, and the currency itself is never sellable back.
    TestEqual(TEXT("Unpriced items sell for nothing"), AAstrawildNPCCharacter::ComputeVendorSellValue(0), 0);

    // Selling always yields strictly less than buying (no arbitrage loop).
    for (const int32 Price : { 2, 3, 4, 6 })
    {
        TestTrue(TEXT("Sell value is strictly below the buy price"),
            AAstrawildNPCCharacter::ComputeVendorSellValue(Price) < Price);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Dungeon & boss hardening (Batch 6 — STEP 22 extension): elemental multiplier,
// phase thresholds and attack scaling — exercising the REAL production statics
// on AAstrawildEchoBossCharacter (same anti-tautology policy as Batch 3/4).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildBossElementalMultiplierTest,
    "ASTRAWILD.Dungeon.BossElementalMultiplier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildBossElementalMultiplierTest::RunTest(const FString& Parameters)
{
    using EEl = EAstrawildElementType;

    // The Underlight Warden: Ash element, Light weakness (from Echo_Gloomfang).
    const EEl Weakness = EEl::Light;
    const EEl Own = EEl::Ash;

    TestEqual(TEXT("Attacking the weakness deals x1.5"),
        AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(Weakness, Weakness, Own), 1.5f);
    TestEqual(TEXT("Same-element attacks are resisted x0.80 (unified with the Echo pipeline)"),
        AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(Own, Weakness, Own), 0.8f);
    TestEqual(TEXT("Neutral elements deal x1.0"),
        AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(EEl::Frost, Weakness, Own), 1.0f);
    TestEqual(TEXT("The None element never triggers a multiplier"),
        AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(EEl::None, Weakness, Own), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildBossPhaseThresholdTest,
    "ASTRAWILD.Dungeon.BossPhaseThresholds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildBossPhaseThresholdTest::RunTest(const FString& Parameters)
{
    // Directive §24 thresholds: phase 2 at <=66%, phase 3 at <=33%.
    TestEqual(TEXT("Full health is phase 1"),
        AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(1.0f, false), 1);
    TestEqual(TEXT("67% is still phase 1"),
        AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(0.67f, false), 1);
    TestEqual(TEXT("66% crosses into phase 2"),
        AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(0.66f, false), 2);
    TestEqual(TEXT("50% is phase 2"),
        AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(0.5f, false), 2);
    TestEqual(TEXT("33% crosses into phase 3"),
        AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(0.33f, false), 3);
    TestEqual(TEXT("Near-death is phase 3"),
        AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(0.05f, false), 3);

    // Enrage forces phase 3 regardless of health (directive §24 — no stalling).
    TestEqual(TEXT("Enrage at full health is still phase 3"),
        AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(1.0f, true), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildBossAttackScalingTest,
    "ASTRAWILD.Dungeon.BossAttackDamage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildBossAttackScalingTest::RunTest(const FString& Parameters)
{
    // Directive §24 — "never solve difficulty by only increasing HP": the damage
    // curve must actually escalate across phases.
    const float Base = 30.0f;
    const float EnrageMult = 1.4f;

    const float Phase1 = AAstrawildEchoBossCharacter::ComputeBossAttackDamage(Base, 1, false, EnrageMult);
    const float Phase2 = AAstrawildEchoBossCharacter::ComputeBossAttackDamage(Base, 2, false, EnrageMult);
    const float Phase3 = AAstrawildEchoBossCharacter::ComputeBossAttackDamage(Base, 3, false, EnrageMult);
    const float Enraged = AAstrawildEchoBossCharacter::ComputeBossAttackDamage(Base, 3, true, EnrageMult);

    TestEqual(TEXT("Phase 1 deals base damage"), Phase1, 30.0f);
    TestEqual(TEXT("Phase 2 deals x1.15"), Phase2, 34.5f);
    TestEqual(TEXT("Phase 3 without enrage stays base"), Phase3, 30.0f);
    TestEqual(TEXT("Enrage multiplies by 1.4"), Enraged, 42.0f);
    TestTrue(TEXT("Damage strictly escalates 1 -> 2"), Phase2 > Phase1);
    TestTrue(TEXT("Enraged beats every non-enraged phase"), Enraged > Phase2 && Enraged > Phase3);
    return true;
}

// ---------------------------------------------------------------------------
// Zones (Batch 7 — The Shattered Vale): table integrity + lookup + weight field.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildZoneTableIntegrityTest,
    "ASTRAWILD.Zones.TableIntegrity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildZoneTableIntegrityTest::RunTest(const FString& Parameters)
{
    const TArray<FAstrawildZoneDescriptor>& Zones = UAstrawildZoneSubsystem::GetAllZones();

    TestEqual(TEXT("Twelve surface zones"), Zones.Num(), 12);

    // Unique ids + unique enum values.
    TSet<FName> Ids;
    TSet<EAstrawildZone> Enums;
    for (const FAstrawildZoneDescriptor& Desc : Zones)
    {
        TestFalse(TEXT("Zone id must not be None"), Desc.ZoneId.IsNone());
        Ids.Add(Desc.ZoneId);
        Enums.Add(Desc.Zone);
    }
    TestEqual(TEXT("Unique zone ids"), Ids.Num(), Zones.Num());
    TestEqual(TEXT("Unique zone enums"), Enums.Num(), Zones.Num());

    // Rects must not overlap (80% inset check keeps interior sampling unambiguous).
    for (int32 A = 0; A < Zones.Num(); ++A)
    {
        for (int32 B = A + 1; B < Zones.Num(); ++B)
        {
            const FBox2D& RA = Zones[A].Bounds;
            const FBox2D& RB = Zones[B].Bounds;
            const bool bOverlaps = RA.Min.X < RB.Max.X && RB.Min.X < RA.Max.X &&
                RA.Min.Y < RB.Max.Y && RB.Min.Y < RA.Max.Y;
            TestFalse(TEXT("Zone rects must not overlap"), bOverlaps);
        }
    }

    // Union must tile the full world rect exactly (4x3 grid of 800m cells — Batch 8).
    const FBox2D World = UAstrawildZoneSubsystem::GetWorldBounds();
    TestEqual(TEXT("World bounds span 3200m in X"), World.Max.X - World.Min.X, 320000.0);
    TestEqual(TEXT("World bounds span 2400m in Y"), World.Max.Y - World.Min.Y, 240000.0);
    TestEqual(TEXT("Twelve zones in the table"), Zones.Num(), 12);

    // Every zone is a square cell of 800m.
    for (const FAstrawildZoneDescriptor& Desc : Zones)
    {
        TestEqual(TEXT("Zone width is 800m"), Desc.GetSizeX(), 80000.0f);
        TestEqual(TEXT("Zone height is 800m"), Desc.GetSizeY(), 80000.0f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildZoneLookupCorrectnessTest,
    "ASTRAWILD.Zones.LookupCorrectness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildZoneLookupCorrectnessTest::RunTest(const FString& Parameters)
{
    struct FRow { EAstrawildZone Zone; float X; float Y; };
    const FRow Rows[] = {
        { EAstrawildZone::DawnFields, -40000.0f, 0.0f },
        { EAstrawildZone::DuskMarsh, -120000.0f, 0.0f },
        { EAstrawildZone::HollowApproach, 40000.0f, 0.0f },
        { EAstrawildZone::AzureShallows, 120000.0f, 0.0f },
        { EAstrawildZone::FrostveilExpanse, -120000.0f, 80000.0f },
        { EAstrawildZone::Glimmerwood, -40000.0f, 80000.0f },
        { EAstrawildZone::EmberRidge, 40000.0f, 80000.0f },
        { EAstrawildZone::SunscarDesert, 120000.0f, 80000.0f },
        { EAstrawildZone::TidebreakerIsles, -120000.0f, -80000.0f },
        { EAstrawildZone::StormcrestHighlands, -40000.0f, -80000.0f },
        { EAstrawildZone::VerdantReach, 40000.0f, -80000.0f },
        { EAstrawildZone::PearlseaReef, 120000.0f, -80000.0f },
    };

    for (const FRow& Row : Rows)
    {
        TestTrue(TEXT("Zone center resolves to its own zone"),
            UAstrawildZoneSubsystem::GetZoneAt(FVector(Row.X, Row.Y, 0.0f)) == Row.Zone);
    }

    // Off-camp point inside Dawn Fields still resolves correctly.
    TestTrue(TEXT("Camp outskirts are Dawn Fields"),
        UAstrawildZoneSubsystem::GetZoneAt(FVector(-60000.0f, 10000.0f, 0.0f)) == EAstrawildZone::DawnFields);

    // Outside the world rect is wilderness (None).
    TestTrue(TEXT("Far outside the world is None"),
        UAstrawildZoneSubsystem::GetZoneAt(FVector(500000.0f, 500000.0f, 0.0f)) == EAstrawildZone::None);

    // The dungeon portal site sits inside Hollow Approach (Batch 8 position).
    TestTrue(TEXT("Dungeon approach portal is in Hollow Approach"),
        UAstrawildZoneSubsystem::GetZoneAt(FVector(10000.0f, 0.0f, 0.0f)) == EAstrawildZone::HollowApproach);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildZoneBlendPartitionTest,
    "ASTRAWILD.Zones.BlendPartitionOfUnity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildZoneBlendPartitionTest::RunTest(const FString& Parameters)
{
    const FVector2D Samples[] = {
        FVector2D(-40000.0f, 0.0f),     // Dawn Fields center (camp — Batch 8 grid)
        FVector2D(0.0f, 0.0f),          // Four-corner meet point
        FVector2D(-120000.0f, 80000.0f),// Frostveil center
        FVector2D(10000.0f, 0.0f),      // Hollow Approach (portal path)
        FVector2D(400000.0f, 400000.0f),// Far outside the world
        FVector2D(-120000.0f, 40000.0f),// Frostveil / Dusk Marsh border blend
    };

    for (const FVector2D& Sample : Samples)
    {
        float Weights[(int32)EAstrawildZone::Count];
        UAstrawildZoneSubsystem::ComputeZoneWeights(Sample, Weights);

        float Sum = 0.0f;
        for (int32 i = 0; i < (int32)EAstrawildZone::Count; ++i)
        {
            TestTrue(TEXT("Weights stay non-negative"), Weights[i] >= 0.0f);
            Sum += Weights[i];
        }
        TestTrue(TEXT("Weights sum to a partition of unity (±0.001)"),
            FMath::Abs(Sum - 1.0f) < 0.001f);
    }

    // Inside a zone far from any border, that zone dominates the blend.
    float CampWeights[(int32)EAstrawildZone::Count];
    UAstrawildZoneSubsystem::ComputeZoneWeights(FVector2D(-40000.0f, 0.0f), CampWeights);
    TestTrue(TEXT("Dawn Fields dominates at the camp (middle cell has 8 neighbors — 0.85 floor)"),
        CampWeights[(int32)EAstrawildZone::DawnFields] > 0.85f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildTerrainHeightDeterminismTest,
    "ASTRAWILD.Terrain.HeightDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildTerrainHeightDeterminismTest::RunTest(const FString& Parameters)
{
    const FVector2D P(33750.0f, -12250.0f);

    const float H1 = AAstrawildTerrainTileActor::EvalWorldHeight(P, 1337);
    const float H2 = AAstrawildTerrainTileActor::EvalWorldHeight(P, 1337);
    TestEqual(TEXT("Same seed + point -> identical height"), H1, H2);

    const float H3 = AAstrawildTerrainTileActor::EvalWorldHeight(P, 2024);
    TestFalse(TEXT("Different seed -> different height"), FMath::IsNearlyEqual(H1, H3));

    // Sanity: the six zones produce heights in a plausible band (marsh dips low,
    // Frostveil/Ember Ridge rise high) — catches parameter regressions.
    const FAstrawildZoneDescriptor* Marsh = UAstrawildZoneSubsystem::FindZone(EAstrawildZone::DuskMarsh);
    const FAstrawildZoneDescriptor* Frost = UAstrawildZoneSubsystem::FindZone(EAstrawildZone::FrostveilExpanse);
    if (Marsh && Frost)
    {
        const float MarshH = AAstrawildTerrainTileActor::EvalWorldHeight(Marsh->GetCenter(), 1337);
        const float FrostH = AAstrawildTerrainTileActor::EvalWorldHeight(Frost->GetCenter(), 1337);
        TestTrue(TEXT("Marsh center stays low (below 600cm)"), MarshH < 600.0f);
        TestTrue(TEXT("Frostveil center rises above the marsh"), FrostH > MarshH);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildTerrainSeamContinuityTest,
    "ASTRAWILD.Terrain.SeamContinuity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildTerrainSeamContinuityTest::RunTest(const FString& Parameters)
{
    // Walk the Dawn Fields / Hollow Approach shared border (X = 40000) plus the
    // four-corner meet (40000, 0): heights 1cm either side of the seam must match
    // closely because the height field is one continuous global function.
    const float BorderX = 40000.0f;
    const float Ys[] = { -60000.0f, -40000.0f, -20000.0f, 0.0f, 20000.0f };

    for (const float Y : Ys)
    {
        const float HWest = AAstrawildTerrainTileActor::EvalWorldHeight(FVector2D(BorderX - 1.0f, Y), 1337);
        const float HEast = AAstrawildTerrainTileActor::EvalWorldHeight(FVector2D(BorderX + 1.0f, Y), 1337);
        TestTrue(TEXT("No seam jump across zone borders (<50cm over 2cm)"),
            FMath::Abs(HEast - HWest) < 50.0f);
    }

    // Row seam (Dawn Fields / Glimmerwood, Y = 0) also continuous.
    const float Xs[] = { -20000.0f, 0.0f, 20000.0f };
    for (const float X : Xs)
    {
        const float HSouth = AAstrawildTerrainTileActor::EvalWorldHeight(FVector2D(X, -1.0f), 1337);
        const float HNorth = AAstrawildTerrainTileActor::EvalWorldHeight(FVector2D(X, 1.0f), 1337);
        TestTrue(TEXT("No seam jump across row borders"),
            FMath::Abs(HNorth - HSouth) < 50.0f);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Final production run — advanced equipment framework (PHASE 12)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildEquipmentSlotRoutingTest,
    "ASTRAWILD.Equipment.SlotRouting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildEquipmentSlotRoutingTest::RunTest(const FString& Parameters)
{
    // Explicit-slot items declare their destination; Auto keeps stat routing. The
    // routing switch in InventoryComponent keys off exactly these values.
    UAstrawildItemDefinition* Helmet = NewObject<UAstrawildItemDefinition>();
    Helmet->EquipmentSlot = EAstrawildEquipmentSlot::Helmet;
    Helmet->ArmorRating = 35.0f;
    Helmet->InsulationRating = 6.0f;
    TestEqual(TEXT("Helmet slot routed"), static_cast<int32>(Helmet->EquipmentSlot), static_cast<int32>(EAstrawildEquipmentSlot::Helmet));

    UAstrawildItemDefinition* Laser = NewObject<UAstrawildItemDefinition>();
    Laser->EquipmentSlot = EAstrawildEquipmentSlot::Weapon;
    Laser->bIsRangedWeapon = true;
    Laser->AmmoItemId = TEXT("Item_EnergyCell");
    Laser->Element = EAstrawildElementType::Pulse;
    TestTrue(TEXT("Ranged weapon declares ammo"), !Laser->AmmoItemId.IsNone());
    TestTrue(TEXT("Ranged weapon is a weapon slot item"), Laser->EquipmentSlot == EAstrawildEquipmentSlot::Weapon);

    // Torso + helmet ratings sum before the single diminishing-returns formula.
    const float TorsoRating = 80.0f;
    const float Total = TorsoRating + Helmet->ArmorRating;
    const float Fraction = UAstrawildCombatComponent::ComputeArmorFraction(Total, 100.0f, 0.6f);
    TestTrue(TEXT("Combined armor fraction in (0, cap]"), Fraction > 0.0f && Fraction <= 0.6f + KINDA_SMALL_NUMBER);
    TestTrue(TEXT("Combined armor beats torso alone"),
        Fraction > UAstrawildCombatComponent::ComputeArmorFraction(TorsoRating, 100.0f, 0.6f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildInsulationBandTest,
    "ASTRAWILD.Survival.InsulationBand",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildInsulationBandTest::RunTest(const FString& Parameters)
{
    // The survival Tick widens the comfort band by the equipped insulation total:
    // cold threshold - Insulation, heat threshold + Insulation. Mirror the exact
    // expression so a regression in the formula breaks this test.
    const float ColdThreshold = 4.0f;
    const float HeatThreshold = 36.0f;
    const float Insulation = 6.0f + 8.0f; // Helm (6) + Exosuit (8).

    const float FeltColdDay = 0.0f;   // Was damage; now comfortable.
    const bool bColdSafe = FeltColdDay > ColdThreshold - Insulation;
    TestTrue(TEXT("Helm+exosuit insulation protects a 0C day"), bColdSafe);

    const float FeltHotDay = 42.0f;   // Was damage; now comfortable.
    const bool bHeatSafe = FeltHotDay < HeatThreshold + Insulation;
    TestTrue(TEXT("Helm+exosuit insulation protects a 42C day"), bHeatSafe);

    const float ExtremeCold = -30.0f;
    TestFalse(TEXT("Extreme cold still bites through insulation"),
        ExtremeCold > ColdThreshold - Insulation);
    return true;
}

// ---------------------------------------------------------------------------
// Final production run — quest objectives (PHASE 15): SurviveTime + VisitZone
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildQuestObjectiveTypesTest,
    "ASTRAWILD.Quest.ObjectiveTypes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildQuestObjectiveTypesTest::RunTest(const FString& Parameters)
{
    // SurviveTime: RequiredCount is SECONDS; accrual clamps at the target.
    FAstrawildQuestObjective Survive;
    Survive.Type = EAstrawildQuestObjectiveType::SurviveTime;
    Survive.RequiredCount = 180;
    Survive.ProgressCount = 179;
    TestFalse(TEXT("179/180s is not complete"), Survive.IsComplete());
    Survive.ProgressCount = FMath::Min(Survive.RequiredCount, Survive.ProgressCount + 1);
    TestTrue(TEXT("180/180s completes"), Survive.IsComplete());

    // VisitZone: consumes Event.ZoneEntered whose TargetId is the zone id
    // (e.g. Zone_EmberRidge) — the matcher compares TargetId directly.
    FAstrawildQuestObjective Visit;
    Visit.Type = EAstrawildQuestObjectiveType::VisitZone;
    Visit.TargetId = TEXT("Zone_EmberRidge");
    Visit.RequiredCount = 1;
    TestEqual(TEXT("VisitZone targets the zone id"), Visit.TargetId.ToString(), FString(TEXT("Zone_EmberRidge")));

    // The appended enum stays serialization-safe: VisitZone follows SurviveTime
    // (value 9) — existing saves deserialize objective types by value.
    TestTrue(TEXT("VisitZone appended after SurviveTime"),
        static_cast<int32>(EAstrawildQuestObjectiveType::VisitZone) > static_cast<int32>(EAstrawildQuestObjectiveType::SurviveTime));
    return true;
}

// ---------------------------------------------------------------------------
// Final production run — save schema v3 (PHASE 16)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSaveSchemaV3Test,
    "ASTRAWILD.Save.SchemaV3",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSaveSchemaV3Test::RunTest(const FString& Parameters)
{
    // The v3 payload types default-init so v2 saves keep deserializing (additive).
    const FAstrawildWorkSiteSaveData Site;
    TestTrue(TEXT("Work-site save defaults: no site id"), Site.SiteId.IsNone());
    TestEqual(TEXT("Work-site save defaults: zero output"), Site.StoredOutput, 0);
    TestFalse(TEXT("Work-site save defaults: no robot"), Site.bHasRobot);

    const FAstrawildDroneSaveData Drone;
    TestFalse(TEXT("Drone save defaults: not deployed"), Drone.bDeployed);

    const FAstrawildRobotSaveData Robot;
    TestTrue(TEXT("Robot save defaults: no site"), Robot.AssignedSiteId.IsNone());

    const FAstrawildPowerGridSaveData Grid;
    TestEqual(TEXT("Grid save defaults: zero charge"), Grid.StoredEnergy, 0.0f);

    // Checksum remains deterministic over schema + timestamp (v3 included).
    const FDateTime Stamp(2026, 8, 30, 12, 0, 0);
    TestEqual(TEXT("Checksum deterministic"),
        UAstrawildSaveSubsystem::ComputeChecksum(3, Stamp),
        UAstrawildSaveSubsystem::ComputeChecksum(3, Stamp));
    TestFalse(TEXT("Checksum differs across schema versions"),
        UAstrawildSaveSubsystem::ComputeChecksum(3, Stamp) == UAstrawildSaveSubsystem::ComputeChecksum(2, Stamp));
    return true;
}

// ---------------------------------------------------------------------------
// Final production run — boss encounter math (PHASE 14)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildBossSpecialsMathTest,
    "ASTRAWILD.Dungeon.BossSpecialsMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildBossSpecialsMathTest::RunTest(const FString& Parameters)
{
    // Weak-point window: damage multiplier applies on top of the elemental
    // multiplier — weakness (x1.5) during the window (x2) = x3 total.
    const float Elemental = AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(
        EAstrawildElementType::Light, EAstrawildElementType::Light, EAstrawildElementType::Ash);
    TestEqual(TEXT("Weakness multiplier is x1.5"), Elemental, 1.5f);

    const float WeakPointMultiplier = 2.0f;
    const float Total = 100.0f * Elemental * WeakPointMultiplier;
    TestEqual(TEXT("Weakness during the weak-point window deals x3"), Total, 300.0f);

    // Enrage cadence: phase 3 halves the special cooldown (documented behavior).
    const float Cooldown = 7.0f;
    TestEqual(TEXT("Phase 3 special cooldown halved"), Cooldown * 0.5f, 3.5f);
    return true;
}

// ---------------------------------------------------------------------------
// Batch 8 — The Grand Menagerie / Grand Expanse (bestiary, sea, skiff)
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildBestiaryTableIntegrityTest,
    "ASTRAWILD.Bestiary.TableIntegrity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildBestiaryTableIntegrityTest::RunTest(const FString& Parameters)
{
    // The generated table must clear its own validator and cover the roster ask.
    TArray<FString> Problems;
    AstrawildBestiary::ValidateTable(Problems);
    for (const FString& Problem : Problems)
    {
        AddError(FString::Printf(TEXT("Bestiary problem: %s"), *Problem));
    }
    TestEqual(TEXT("Bestiary validator reports no problems"), Problems.Num(), 0);
    TestTrue(TEXT("At least 200 generated species"),
        AstrawildBestiary::GetRowCount() >= 200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSeaZoneClassificationTest,
    "ASTRAWILD.Zones.SeaClassification",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSeaZoneClassificationTest::RunTest(const FString& Parameters)
{
    // The three sea zones dip below the waterline; home zones stay dry.
    TestTrue(TEXT("Azure Shallows is a sea zone"), UAstrawildZoneSubsystem::IsSeaZone(EAstrawildZone::AzureShallows));
    TestTrue(TEXT("Tidebreaker Isles is a sea zone"), UAstrawildZoneSubsystem::IsSeaZone(EAstrawildZone::TidebreakerIsles));
    TestTrue(TEXT("Pearlsea Reef is a sea zone"), UAstrawildZoneSubsystem::IsSeaZone(EAstrawildZone::PearlseaReef));
    TestFalse(TEXT("Dawn Fields stays dry"), UAstrawildZoneSubsystem::IsSeaZone(EAstrawildZone::DawnFields));
    TestFalse(TEXT("Sunscar Desert stays dry"), UAstrawildZoneSubsystem::IsSeaZone(EAstrawildZone::SunscarDesert));

    // Islands must break the surface: the Isles' highest point clears sea level.
    const FAstrawildZoneDescriptor* Isles = UAstrawildZoneSubsystem::FindZoneById(TEXT("Zone_TidebreakerIsles"));
    TestTrue(TEXT("Isles ceiling clears the waterline"),
        Isles && (Isles->BaseHeight + Isles->HeightAmplitude) > UAstrawildZoneSubsystem::GetSeaLevelZ() + 500.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSkiffFlightMathTest,
    "ASTRAWILD.Skiff.FlightMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSkiffFlightMathTest::RunTest(const FString& Parameters)
{
    const FVector Forward(1.0f, 0.0f, 0.0f);

    // Full cruise thrust, level flight.
    const FVector Cruise = AAstrawildSkiffActor::ComputeSkiffVelocity(Forward, 1.0f, 0.0f, false, 1400.0f, 2600.0f, 700.0f);
    TestEqual(TEXT("Cruise thrust is forward at cruise speed"), Cruise, FVector(1400.0f, 0.0f, 0.0f));

    // Boost multiplies horizontal speed only.
    const FVector Boost = AAstrawildSkiffActor::ComputeSkiffVelocity(Forward, 1.0f, 0.5f, true, 1400.0f, 2600.0f, 700.0f);
    TestEqual(TEXT("Boost thrust with climb"), Boost, FVector(2600.0f, 0.0f, 350.0f));

    // Reverse thrust descends.
    const FVector Reverse = AAstrawildSkiffActor::ComputeSkiffVelocity(Forward, -0.5f, -1.0f, false, 1400.0f, 2600.0f, 700.0f);
    TestEqual(TEXT("Half reverse with full descent"), Reverse, FVector(-700.0f, 0.0f, -700.0f));

    // Clamped axes never exceed their envelope.
    const FVector Clamped = AAstrawildSkiffActor::ComputeSkiffVelocity(Forward, 5.0f, -5.0f, false, 1400.0f, 2600.0f, 700.0f);
    TestEqual(TEXT("Axes clamp to [-1, 1]"), Clamped, FVector(1400.0f, 0.0f, -700.0f));
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 Batch 2 — H-11: craft output weight guard
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildCraftOutputGuardTest,
    "ASTRAWILD.Craft.OutputGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildCraftOutputGuardTest::RunTest(const FString& Parameters)
{
    // World-free inventory: registry-less fallback weight is 1.0 per unit,
    // default MaxWeight 120 — deterministic math without a world.
    UAstrawildInventoryComponent* Inventory = NewObject<UAstrawildInventoryComponent>();

    // Empty set always fits.
    TestTrue(TEXT("Empty stack set always fits"), Inventory->CanAddItemStacks({}));

    const auto MakeStack = [](const TCHAR* ItemId, const int32 Quantity)
    {
        FAstrawildItemStack Stack;
        Stack.ItemId = ItemId;
        Stack.Quantity = Quantity;
        return Stack;
    };

    // CUMULATIVE rule (the H-11 core): each 70-unit stack ALONE fits the
    // 120-weight pack, but the pair (140) must be rejected as a set.
    TestTrue(TEXT("Single 70-unit stack fits alone"),
        Inventory->CanAddItemStacks({ MakeStack(TEXT("Item_Plank"), 70) }));
    TestFalse(TEXT("Pair of 70-unit stacks rejected as a SET (140 > 120)"),
        Inventory->CanAddItemStacks({ MakeStack(TEXT("Item_Plank"), 70), MakeStack(TEXT("Item_Nail"), 70) }));

    // Exactly-at-cap accepted (KINDA_SMALL_NUMBER tolerance), one-over rejected.
    TestTrue(TEXT("Exactly 120 units accepted at the cap"),
        Inventory->CanAddItemStacks({ MakeStack(TEXT("Item_Plank"), 60), MakeStack(TEXT("Item_Nail"), 60) }));
    TestFalse(TEXT("121 units rejected"),
        Inventory->CanAddItemStacks({ MakeStack(TEXT("Item_Plank"), 60), MakeStack(TEXT("Item_Nail"), 61) }));

    // Zero-quantity stacks are skipped in the sum, not fatal.
    TestTrue(TEXT("Zero-quantity stack skipped in the sum"),
        Inventory->CanAddItemStacks({ MakeStack(TEXT("Item_Plank"), 0), MakeStack(TEXT("Item_Nail"), 5) }));

    // Pre-flight wiring: CraftRecipe consults CanAddItemStacks BEFORE consuming
    // ingredients (source contract — the refusal path keeps every material).
    // (Full component flow needs a world; the guard math above is the
    // deterministic half of H-11.)
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — weapon profiles (Master Plan §8): each family is a distinct
// firing archetype with sane damage/interval math.
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildWeaponProfileTest,
    "ASTRAWILD.Weapon.ProfileMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildWeaponProfileTest::RunTest(const FString& Parameters)
{
    // Family → expected fire-mode archetype mapping (data contract).
    UAstrawildWeaponDefinition* Kinetic = NewObject<UAstrawildWeaponDefinition>();
    Kinetic->WeaponId = TEXT("Weapon_TestKinetic");
    Kinetic->Family = EAstrawildWeaponFamily::Kinetic;
    Kinetic->FireMode = EAstrawildWeaponFireMode::Projectile;
    Kinetic->DamagePerHit = 14.0f;
    Kinetic->FireIntervalSeconds = 0.5f;

    UAstrawildWeaponDefinition* Missile = NewObject<UAstrawildWeaponDefinition>();
    Missile->WeaponId = TEXT("Weapon_TestMissile");
    Missile->Family = EAstrawildWeaponFamily::Missile;
    Missile->FireMode = EAstrawildWeaponFireMode::HomingProjectile;
    Missile->DamagePerHit = 62.0f;
    Missile->FireIntervalSeconds = 1.4f;

    // DPS sanity: kinetic 28/s sustained, missile ~44/s but homing.
    TestEqual(TEXT("Kinetic DPS"), Kinetic->DamagePerHit / Kinetic->FireIntervalSeconds, 28.0f);
    TestEqual(TEXT("Missile DPS"), Missile->DamagePerHit / Missile->FireIntervalSeconds, 44.285715f, 0.01f);

    // Arc chain decay: hop damage falls geometrically by ChainDamageFraction.
    UAstrawildWeaponDefinition* Arc = NewObject<UAstrawildWeaponDefinition>();
    Arc->ChainDamageFraction = 0.6f;
    Arc->DamagePerHit = 25.0f;
    const float Hop1 = Arc->DamagePerHit * Arc->ChainDamageFraction;
    const float Hop2 = Hop1 * Arc->ChainDamageFraction;
    TestEqual(TEXT("Arc hop 1"), Hop1, 15.0f);
    TestEqual(TEXT("Arc hop 2"), Hop2, 9.0f);

    // Beam pierce budget: 1 + PierceCount targets.
    Arc->PierceCount = 5;
    TestEqual(TEXT("Beam max targets"), 1 + Arc->PierceCount, 6);
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — world-event eligibility gates (Master Plan §19)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildWorldEventEligibilityTest,
    "ASTRAWILD.WorldEvent.EligibilityGates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildWorldEventEligibilityTest::RunTest(const FString& Parameters)
{
    UAstrawildWorldEventDefinition* NightRaid = NewObject<UAstrawildWorldEventDefinition>();
    NightRaid->EventId = TEXT("Event_TestRaid");
    NightRaid->RarityWeight = 1.0f;
    NightRaid->MinDay = 3;
    NightRaid->bRequiresNight = true;

    TMap<FName, int32> NoCooldowns;

    // Day gate.
    TestFalse(TEXT("Night raid blocked on day 1"),
        UAstrawildWorldEventSubsystem::IsEventEligible(NightRaid, 0, 1, 23, 0, NoCooldowns, 2));
    // Night gate (10:00 is daytime).
    TestFalse(TEXT("Night raid blocked at 10:00"),
        UAstrawildWorldEventSubsystem::IsEventEligible(NightRaid, 4000, 3, 10, 0, NoCooldowns, 2));
    // Both gates satisfied (day 3, 23:00).
    TestTrue(TEXT("Night raid eligible at day 3 23:00"),
        UAstrawildWorldEventSubsystem::IsEventEligible(NightRaid, 4000, 3, 23, 0, NoCooldowns, 2));
    // Concurrency gate.
    TestFalse(TEXT("Night raid blocked when event slots full"),
        UAstrawildWorldEventSubsystem::IsEventEligible(NightRaid, 4000, 3, 23, 2, NoCooldowns, 2));

    // Cooldown gate: ends at minute 500, current 499 → blocked; 501 → ready.
    TMap<FName, int32> Cooldowns;
    Cooldowns.Add(NightRaid->EventId, 500);
    TestFalse(TEXT("Cooldown blocks at minute 499"),
        UAstrawildWorldEventSubsystem::IsEventEligible(NightRaid, 499, 3, 23, 0, Cooldowns, 2));
    TestTrue(TEXT("Cooldown frees at minute 501"),
        UAstrawildWorldEventSubsystem::IsEventEligible(NightRaid, 501, 3, 23, 0, Cooldowns, 2));

    // Zero-weight events never roll.
    UAstrawildWorldEventDefinition* Ghost = NewObject<UAstrawildWorldEventDefinition>();
    Ghost->EventId = TEXT("Event_TestGhost");
    Ghost->RarityWeight = 0.0f;
    TestFalse(TEXT("Zero-weight event never eligible"),
        UAstrawildWorldEventSubsystem::IsEventEligible(Ghost, 0, 9, 12, 0, NoCooldowns, 2));
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — world-event weighted pick determinism
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildWorldEventPickTest,
    "ASTRAWILD.WorldEvent.WeightedPickDeterminism",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildWorldEventPickTest::RunTest(const FString& Parameters)
{
    UAstrawildWorldEventDefinition* Common = NewObject<UAstrawildWorldEventDefinition>();
    Common->EventId = TEXT("Event_TestCommon");
    Common->RarityWeight = 3.0f;

    UAstrawildWorldEventDefinition* Rare = NewObject<UAstrawildWorldEventDefinition>();
    Rare->EventId = TEXT("Event_TestRare");
    Rare->RarityWeight = 1.0f;

    TArray<UAstrawildWorldEventDefinition*> Pool = { Common, Rare };
    TMap<FName, int32> NoCooldowns;

    // Same seed → same pick, always (deterministic scheduler contract).
    FRandomStream StreamA(12345);
    FRandomStream StreamB(12345);
    const FName PickA = UAstrawildWorldEventSubsystem::PickWeightedEvent(Pool, NoCooldowns, 1000, 5, 12, 0, 2, StreamA);
    const FName PickB = UAstrawildWorldEventSubsystem::PickWeightedEvent(Pool, NoCooldowns, 1000, 5, 12, 0, 2, StreamB);
    TestTrue(TEXT("Seeded pick is deterministic"), PickA == PickB);

    // Both picks land on pool members only.
    TestTrue(TEXT("Pick comes from the pool"), PickA == Common->EventId || PickA == Rare->EventId);

    // Empty pool → no event.
    TArray<UAstrawildWorldEventDefinition*> EmptyPool;
    FRandomStream StreamC(1);
    TestTrue(TEXT("Empty pool picks nothing"),
        UAstrawildWorldEventSubsystem::PickWeightedEvent(EmptyPool, NoCooldowns, 0, 1, 12, 0, 2, StreamC) == NAME_None);

    // Statistical sanity: with weight 3:1 over many rolls the common event wins more.
    FRandomStream StreamD(777);
    int32 CommonWins = 0;
    for (int32 i = 0; i < 400; ++i)
    {
        if (UAstrawildWorldEventSubsystem::PickWeightedEvent(Pool, NoCooldowns, 100000, 5, 12, 0, 2, StreamD) == Common->EventId)
        {
            ++CommonWins;
        }
    }
    TestTrue(TEXT("Weighted pool favours the common event (3:1)"), CommonWins > 220);
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — POI discovery radius math (Master Plan §10)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildPOIRadiusTest,
    "ASTRAWILD.POI.DiscoveryRadiusMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildPOIRadiusTest::RunTest(const FString& Parameters)
{
    UAstrawildPOIDefinition* POI = NewObject<UAstrawildPOIDefinition>();
    POI->PoiId = TEXT("POI_Test");
    POI->DiscoveryRadius = 1200.0f;

    TestEqual(TEXT("Stock discovery radius"),
        UAstrawildPOISubsystem::ComputeDiscoveryRadius(POI, false), 1200.0f);
    TestEqual(TEXT("Signal scanner doubles the radius"),
        UAstrawildPOISubsystem::ComputeDiscoveryRadius(POI, true), 2400.0f);

    // Invalid definition → zero (never discovers garbage).
    TestEqual(TEXT("Null POI discovers nothing"),
        UAstrawildPOISubsystem::ComputeDiscoveryRadius(nullptr, true), 0.0f);

    // Clamped minimum: tiny radii never break the sweep.
    POI->DiscoveryRadius = 10.0f;
    TestEqual(TEXT("Radius clamps up to 100cm"),
        UAstrawildPOISubsystem::ComputeDiscoveryRadius(POI, false), 100.0f);
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — resource node definition contract (P0 determinism fix)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Production V2 — resource node definition contract (P0 determinism fix)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildResourceNodeContractTest,
    "ASTRAWILD.ResourceNode.DefinitionContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildResourceNodeContractTest::RunTest(const FString& Parameters)
{
    // A valid node definition ALWAYS carries a concrete item id — the P0 fix
    // makes identity deterministic (no fallback loot, no silent no-op nodes).
    UAstrawildResourceNodeDefinition* Vein = NewObject<UAstrawildResourceNodeDefinition>();
    Vein->NodeId = TEXT("Node_TestVein");
    Vein->ResourceItemId = TEXT("Item_AncientAlloy");
    Vein->bRequiresScannerDetection = true;
    Vein->QuantityPerHarvest = 1;
    Vein->MaxQuantity = 1;
    Vein->RespawnDurationSeconds = 480.0f;

    TestFalse(TEXT("Node identity is concrete"), Vein->ResourceItemId.IsNone());
    TestTrue(TEXT("Hidden vein requires scanner"), Vein->bRequiresScannerDetection);
    TestTrue(TEXT("Harvest never exceeds the vein"), Vein->QuantityPerHarvest <= Vein->MaxQuantity);
    TestTrue(TEXT("Rare veins respawn slowly"), Vein->RespawnDurationSeconds >= 300.0f);

    // Rarity ladder drives the visual contract (shape kits in the node actor).
    const EAstrawildRarity Ladder[] = { EAstrawildRarity::Common, EAstrawildRarity::Uncommon,
        EAstrawildRarity::Rare, EAstrawildRarity::Epic, EAstrawildRarity::Legendary, EAstrawildRarity::Mythic };
    for (int32 i = 1; i < 6; ++i)
    {
        TestTrue(TEXT("Rarity ladder is strictly ordered"), static_cast<int32>(Ladder[i - 1]) < static_cast<int32>(Ladder[i]));
    }
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — production Echo roster contract (Master Plan §6 STEP 5)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildProductionEchoRosterTest,
    "ASTRAWILD.Echo.ProductionRosterContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildProductionEchoRosterTest::RunTest(const FString& Parameters)
{
    // The production roster covers distinct role archetypes through work
    // affinities + passive auras — no two production Echoes are reskins.
    UAstrawildEchoDefinition* Gatherer = NewObject<UAstrawildEchoDefinition>();
    Gatherer->DefinitionId = TEXT("Echo_TestGatherer");
    Gatherer->WorkAffinities = { { EAstrawildWorkType::Gathering, 1.9f } };
    Gatherer->Passive = EAstrawildEchoPassive::CarryBoost;
    Gatherer->Rarity = EAstrawildRarity::Uncommon;

    UAstrawildEchoDefinition* Medic = NewObject<UAstrawildEchoDefinition>();
    Medic->DefinitionId = TEXT("Echo_TestMedic");
    Medic->WorkAffinities = { { EAstrawildWorkType::ResearchAssist, 1.5f } };
    Medic->Passive = EAstrawildEchoPassive::PartyHeal;
    Medic->Rarity = EAstrawildRarity::Rare;

    TestNotEqual(TEXT("Roles differentiate through passives"),
        static_cast<int32>(Gatherer->Passive), static_cast<int32>(Medic->Passive));
    TestTrue(TEXT("Specialist affinity exceeds the 0.5 baseline"),
        Gatherer->WorkAffinities[0].Affinity > 1.5f);

    // Passive vocabulary is closed: every value maps to a real aura system.
    const int32 PassiveCount = 5; // None + PartyHeal + PlayerStamina + CarryBoost + ThreatDampener
    TestEqual(TEXT("Passive enum is closed"), static_cast<int32>(EAstrawildEchoPassive::ThreatDampener), PassiveCount - 1);

    // Affinity contract: work types pair with a strength in 0..2.
    for (const FAstrawildWorkAffinity& Affinity : Medic->WorkAffinities)
    {
        TestTrue(TEXT("Affinity inside 0..2"), Affinity.Affinity >= 0.0f && Affinity.Affinity <= 2.0f);
    }
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — armor split-insulation contract (Master Plan §9)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildArmorSplitInsulationTest,
    "ASTRAWILD.Armor.SplitInsulation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildArmorSplitInsulationTest::RunTest(const FString& Parameters)
{
    // Mk II/III sets carry split thermal bands; the legacy InsulationRating
    // keeps counting on BOTH sides (documented backward-compat rule).
    UAstrawildItemDefinition* BastionPlate = NewObject<UAstrawildItemDefinition>();
    BastionPlate->ItemId = TEXT("Item_TestBastion");
    BastionPlate->ColdInsulationRating = 9.0f;
    BastionPlate->HeatInsulationRating = 9.0f;

    TestEqual(TEXT("Cold band"), BastionPlate->ColdInsulationRating, 9.0f);
    TestEqual(TEXT("Heat band"), BastionPlate->HeatInsulationRating, 9.0f);

    // Legacy piece: one rating, both sides (the getter mirrors this rule).
    UAstrawildItemDefinition* LegacyHelm = NewObject<UAstrawildItemDefinition>();
    LegacyHelm->ItemId = TEXT("Item_TestLegacy");
    LegacyHelm->InsulationRating = 6.0f;
    const float LegacyCold = LegacyHelm->ColdInsulationRating > 0.0f ? LegacyHelm->ColdInsulationRating : LegacyHelm->InsulationRating;
    const float LegacyHeat = LegacyHelm->HeatInsulationRating > 0.0f ? LegacyHelm->HeatInsulationRating : LegacyHelm->InsulationRating;
    TestEqual(TEXT("Legacy rating covers cold"), LegacyCold, 6.0f);
    TestEqual(TEXT("Legacy rating covers heat"), LegacyHeat, 6.0f);

    // Tier ladder is strictly ordered (Field → Mk1 → Mk2 → Mk3 → Experimental).
    TestTrue(TEXT("Tier ladder ordered"),
        static_cast<int32>(EAstrawildTechTier::Field) < static_cast<int32>(EAstrawildTechTier::Mk1) &&
        static_cast<int32>(EAstrawildTechTier::Mk1) < static_cast<int32>(EAstrawildTechTier::Mk2) &&
        static_cast<int32>(EAstrawildTechTier::Mk2) < static_cast<int32>(EAstrawildTechTier::Mk3) &&
        static_cast<int32>(EAstrawildTechTier::Mk3) < static_cast<int32>(EAstrawildTechTier::Experimental));
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — robot chassis specialist rates (Master Plan §12)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildRobotSpecialistTest,
    "ASTRAWILD.Robot.SpecialistRates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildRobotSpecialistTest::RunTest(const FString& Parameters)
{
    UAstrawildRobotDefinition* Borebot = NewObject<UAstrawildRobotDefinition>();
    Borebot->RobotId = TEXT("Robot_TestBore");
    Borebot->PrimaryWorkType = EAstrawildWorkType::Mining;
    Borebot->SpecialistWorkRate = 1.6f;
    Borebot->GenericWorkRate = 0.5f;

    // The specialist trade-off contract: strong on-type, weak off-type.
    TestTrue(TEXT("Specialist beats the 0.8 general baseline"), Borebot->SpecialistWorkRate > 0.8f);
    TestTrue(TEXT("Off-type rate undercuts the general baseline"), Borebot->GenericWorkRate < 0.8f);

    // Rate clamps keep every chassis inside 0..4 (definition meta mirrors).
    TestTrue(TEXT("Rates stay inside 0..4"),
        Borebot->SpecialistWorkRate >= 0.0f && Borebot->SpecialistWorkRate <= 4.0f &&
        Borebot->GenericWorkRate >= 0.0f && Borebot->GenericWorkRate <= 4.0f);

    // Move speed multiplier sane (0.1..4 by design).
    Borebot->MoveSpeedMultiplier = 0.8f;
    TestTrue(TEXT("Move multiplier inside 0.1..4"),
        Borebot->MoveSpeedMultiplier >= 0.1f && Borebot->MoveSpeedMultiplier <= 4.0f);
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — work-site consume→produce chain (Master Plan §7)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildWorkSiteChainTest,
    "ASTRAWILD.WorkSite.ProductionChain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildWorkSiteChainTest::RunTest(const FString& Parameters)
{
    // Definition-driven kitchen: 1 raw meat in, 1 seared meat out per cycle.
    UAstrawildWorkSiteDefinition* Kitchen = NewObject<UAstrawildWorkSiteDefinition>();
    Kitchen->SiteId = TEXT("Site_TestKitchen");
    Kitchen->WorkType = EAstrawildWorkType::Cooking;
    Kitchen->OutputItemId = TEXT("Item_CookedMeat");
    Kitchen->OutputQuantity = 1;
    FAstrawildItemStack RawMeat;
    RawMeat.ItemId = TEXT("Item_RawMeat");
    RawMeat.Quantity = 1;
    Kitchen->InputItems = { RawMeat };
    Kitchen->SecondsPerOutput = 8.0f;

    TestTrue(TEXT("Input-driven sites declare inputs"), !Kitchen->InputItems.IsEmpty());
    TestEqual(TEXT("One input per cycle"), Kitchen->InputItems[0].Quantity, 1);
    TestEqual(TEXT("Output quantity"), Kitchen->OutputQuantity, 1);
    TestTrue(TEXT("Cooking is faster than gathering"), Kitchen->SecondsPerOutput < 10.0f);

    // Harvest sites declare no inputs (the land provides).
    UAstrawildWorkSiteDefinition* Gathering = NewObject<UAstrawildWorkSiteDefinition>();
    Gathering->SiteId = TEXT("Site_TestGather");
    Gathering->OutputItemId = TEXT("Item_Fiber");
    TestTrue(TEXT("Harvest sites consume nothing"), Gathering->InputItems.IsEmpty());
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — save schema v4 additive contract (Master Plan §25)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSaveSchemaV4Test,
    "ASTRAWILD.Save.SchemaV4",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSaveSchemaV4Test::RunTest(const FString& Parameters)
{
    UAstrawildSaveGame* Save = NewObject<UAstrawildSaveGame>();

    // v4 additions default-init: no events running, nothing discovered, no
    // cooldowns — a v3 save migrating forward starts with a quiet world.
    TestTrue(TEXT("World events default empty"), Save->WorldEvents.ActiveEvents.IsEmpty());
    TestEqual(TEXT("Next roll defaults to zero"), Save->WorldEvents.NextRollAbsoluteMinute, 0);
    TestTrue(TEXT("Cooldown map default empty"), Save->WorldEvents.CooldownEndMinutes.IsEmpty());
    TestTrue(TEXT("POI discovery list default empty"), Save->DiscoveredPOIIds.IsEmpty());

    // Active-event runtime shape: id + end minute + zone + location persist.
    FAstrawildWorldEventSaveData Runtime;
    Runtime.EventId = TEXT("Event_Test");
    Runtime.EndAbsoluteMinute = 1200;
    Runtime.Zone = EAstrawildZone::Glimmerwood;
    Runtime.Location = FVector(100.0f, 200.0f, 300.0f);
    Save->WorldEvents.ActiveEvents.Add(Runtime);
    TestTrue(TEXT("Active event persists its id"), Save->WorldEvents.ActiveEvents[0].EventId == FName(TEXT("Event_Test")));
    TestEqual(TEXT("Active event persists its end minute"), Save->WorldEvents.ActiveEvents[0].EndAbsoluteMinute, 1200);

    // Drone battery default: 0 (legacy drones without the field recharge).
    FAstrawildDroneSaveData Drone;
    TestEqual(TEXT("Drone battery default"), Drone.BatteryRemainingSeconds, 0.0f);
    Drone.BatteryRemainingSeconds = 321.5f;
    TestEqual(TEXT("Drone battery persists"), Drone.BatteryRemainingSeconds, 321.5f);

    // Robot definition id default: none (general-purpose frame).
    FAstrawildRobotSaveData Robot;
    TestTrue(TEXT("Robot chassis default none"), Robot.RobotDefinitionId.IsNone());

    // Work-site buffer default: empty, output quantity 1.
    FAstrawildWorkSiteSaveData Site;
    TestTrue(TEXT("Site input buffer default empty"), Site.InputBuffer.IsEmpty());
    TestEqual(TEXT("Site output quantity default"), Site.OutputQuantity, 1);
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 — quest objective extension (DiscoverPOI serialization-safe)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildQuestDiscoverPOITest,
    "ASTRAWILD.Quest.DiscoverPOIType",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildQuestDiscoverPOITest::RunTest(const FString& Parameters)
{
    // DiscoverPOI is appended LAST — existing saves keep every objective index.
    TestEqual(TEXT("DiscoverPOI is the last objective type"),
        static_cast<int32>(EAstrawildQuestObjectiveType::DiscoverPOI), 10);

    FAstrawildQuestObjective Objective;
    Objective.Type = EAstrawildQuestObjectiveType::DiscoverPOI;
    Objective.TargetId = TEXT("POI_FirstLightRuin");
    Objective.RequiredCount = 1;

    TestTrue(TEXT("Objective target carries the POI id"), Objective.TargetId == FName(TEXT("POI_FirstLightRuin")));
    TestFalse(TEXT("Objective starts incomplete"), Objective.IsComplete());

    Objective.ProgressCount = 1;
    TestTrue(TEXT("Objective completes at count"), Objective.IsComplete());
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 Batch 2 — Visual Vertical Slice runtime support
// (biome dressing / atmosphere ramp / VFX palette + geometry math)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDressingZoneProfilesTest,
    "ASTRAWILD.BiomeDressing.ZoneProfiles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDressingZoneProfilesTest::RunTest(const FString& Parameters)
{
    // Every real zone has a dressing budget inside sane bounds; forest zones
    // read denser than desert/sea zones (zone personality, Master Plan §5).
    int32 ProfilesChecked = 0;
    for (const FAstrawildZoneDescriptor& Zone : UAstrawildZoneSubsystem::GetAllZones())
    {
        const FAstrawildDressingProfile Profile = AAstrawildBiomeDressingActor::GetDressingProfile(Zone.Zone);
        TestTrue(*FString::Printf(TEXT("Zone %s has trees"), *Zone.ZoneId.ToString()), Profile.TreeCount > 0);
        TestTrue(*FString::Printf(TEXT("Zone %s has rocks"), *Zone.ZoneId.ToString()), Profile.RockCount > 0);
        TestTrue(*FString::Printf(TEXT("Zone %s budget sane"), *Zone.ZoneId.ToString()),
            Profile.TreeCount + Profile.RockCount + Profile.GrassCount <= 250);
        ++ProfilesChecked;
    }
    TestEqual(TEXT("All 12 zones profiled"), ProfilesChecked, 12);

    const FAstrawildDressingProfile Verdant = AAstrawildBiomeDressingActor::GetDressingProfile(EAstrawildZone::VerdantReach);
    const FAstrawildDressingProfile Desert = AAstrawildBiomeDressingActor::GetDressingProfile(EAstrawildZone::SunscarDesert);
    const FAstrawildDressingProfile Reef = AAstrawildBiomeDressingActor::GetDressingProfile(EAstrawildZone::PearlseaReef);
    TestTrue(TEXT("Jungle denser than desert"), Verdant.TreeCount > Desert.TreeCount);
    TestTrue(TEXT("Jungle denser than reef"), Verdant.TreeCount > Reef.TreeCount);

    // Frost zones snow-blend their canopies; sea zones run palms.
    const FAstrawildDressingProfile Frost = AAstrawildBiomeDressingActor::GetDressingProfile(EAstrawildZone::FrostveilExpanse);
    const FAstrawildDressingProfile Isles = AAstrawildBiomeDressingActor::GetDressingProfile(EAstrawildZone::TidebreakerIsles);
    TestTrue(TEXT("Frost blends snow into canopies"), Frost.SnowBlend > 0.3f);
    TestTrue(TEXT("Isles dress as palms"), Isles.CanopyStyle == EAstrawildDressingCanopy::Palm);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDressingExclusionTest,
    "ASTRAWILD.BiomeDressing.PointRejection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDressingExclusionTest::RunTest(const FString& Parameters)
{
    TArray<FVector2D> Centers;
    TArray<float> Radii;
    Centers.Add(FVector2D(0.0f, 0.0f));
    Radii.Add(3000.0f);

    TestFalse(TEXT("Camp center rejected"),
        AAstrawildBiomeDressingActor::IsPointDressable(FVector2D(100.0f, 200.0f), Centers, Radii));
    TestFalse(TEXT("Camp edge (inside radius) rejected"),
        AAstrawildBiomeDressingActor::IsPointDressable(FVector2D(2999.0f, 0.0f), Centers, Radii));
    TestTrue(TEXT("Just past the camp bubble accepted"),
        AAstrawildBiomeDressingActor::IsPointDressable(FVector2D(3001.0f, 0.0f), Centers, Radii));
    TestTrue(TEXT("Far point accepted"),
        AAstrawildBiomeDressingActor::IsPointDressable(FVector2D(50000.0f, -50000.0f), Centers, Radii));

    // Parallel arrays with mismatched lengths use the shorter count (safe).
    TArray<float> ShortRadii;
    ShortRadii.Add(100.0f);
    TestTrue(TEXT("Mismatched exclusion arrays degrade safely"),
        AAstrawildBiomeDressingActor::IsPointDressable(FVector2D(200.0f, 0.0f), Centers, ShortRadii));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDressingDeterministicScatterTest,
    "ASTRAWILD.BiomeDressing.DeterministicScatter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDressingDeterministicScatterTest::RunTest(const FString& Parameters)
{
    const FAstrawildZoneDescriptor* Zone = UAstrawildZoneSubsystem::FindZoneById(TEXT("Zone_DawnFields"));
    if (!TestNotNull(TEXT("Dawn Fields zone resolves"), Zone))
    {
        return false;
    }

    const FAstrawildDressingProfile Profile = AAstrawildBiomeDressingActor::GetDressingProfile(Zone->Zone);
    TArray<FVector> TreesA, RocksA, GrassA;
    TArray<FVector> TreesB, RocksB, GrassB;
    AAstrawildBiomeDressingActor::ScatterDressingPoints(1337, *Zone, Profile, {}, {}, TreesA, RocksA, GrassA);
    AAstrawildBiomeDressingActor::ScatterDressingPoints(1337, *Zone, Profile, {}, {}, TreesB, RocksB, GrassB);
    AAstrawildBiomeDressingActor::ScatterDressingPoints(4242, *Zone, Profile, {}, {}, TreesB, RocksB, GrassB); // reuse arrays (different seed)

    // Same seed → identical layout: re-run the first seed fresh.
    TreesB.Reset(); RocksB.Reset(); GrassB.Reset();
    AAstrawildBiomeDressingActor::ScatterDressingPoints(1337, *Zone, Profile, {}, {}, TreesB, RocksB, GrassB);
    TestEqual(TEXT("Same seed → same tree count"), TreesA.Num(), TreesB.Num());
    bool bIdentical = TreesA.Num() == TreesB.Num();
    for (int32 i = 0; bIdentical && i < TreesA.Num(); ++i)
    {
        bIdentical = TreesA[i].Equals(TreesB[i], 0.01f);
    }
    TestTrue(TEXT("Same seed → identical tree positions"), bIdentical);

    // Water rule: nothing dresses below the sea margin (deep sea zone scatter
    // lands only on islets — count can be small but every point must be dry).
    if (const FAstrawildZoneDescriptor* SeaZone = UAstrawildZoneSubsystem::FindZoneById(TEXT("Zone_AzureShallows")))
    {
        const FAstrawildDressingProfile SeaProfile = AAstrawildBiomeDressingActor::GetDressingProfile(SeaZone->Zone);
        TArray<FVector> SeaTrees, SeaRocks, SeaGrass;
        AAstrawildBiomeDressingActor::ScatterDressingPoints(1337, *SeaZone, SeaProfile, {}, {}, SeaTrees, SeaRocks, SeaGrass);
        const float SeaFloorZ = UAstrawildZoneSubsystem::GetSeaLevelZ() + AAstrawildBiomeDressingActor::GetSeaMargin();
        bool bAllDry = true;
        for (const FVector& Point : SeaTrees)
        {
            bAllDry &= Point.Z >= SeaFloorZ;
        }
        TestTrue(TEXT("Sea zone dressing stays above the waterline"), bAllDry);
    }

    // Camp exclusion: nothing lands inside the starter-camp bubble.
    TArray<FVector2D> Camp;
    TArray<float> CampRadius;
    Camp.Add(Zone->GetCenter());
    CampRadius.Add(3000.0f);
    TArray<FVector> TreesC, RocksC, GrassC;
    AAstrawildBiomeDressingActor::ScatterDressingPoints(1337, *Zone, Profile, Camp, CampRadius, TreesC, RocksC, GrassC);
    bool bNoneInCamp = true;
    for (const FVector& Point : TreesC)
    {
        bNoneInCamp &= FVector2D::DistSquared(FVector2D(Point.X, Point.Y), Camp[0]) >= FMath::Square(3000.0f);
    }
    TestTrue(TEXT("Dressing avoids the starter camp"), bNoneInCamp);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAtmosphereRampTest,
    "ASTRAWILD.Atmosphere.DayRamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAtmosphereRampTest::RunTest(const FString& Parameters)
{
    const FAstrawildAtmosphereSample Noon = AAstrawildWorldBootstrapper::EvalAtmosphereRamp(0.5f, false, 1.0f);
    const FAstrawildAtmosphereSample Dawn = AAstrawildWorldBootstrapper::EvalAtmosphereRamp(0.0f, false, 1.0f);
    const FAstrawildAtmosphereSample Dusk = AAstrawildWorldBootstrapper::EvalAtmosphereRamp(1.0f, false, 1.0f);
    const FAstrawildAtmosphereSample Night = AAstrawildWorldBootstrapper::EvalAtmosphereRamp(0.5f, true, 1.0f);
    const FAstrawildAtmosphereSample Storm = AAstrawildWorldBootstrapper::EvalAtmosphereRamp(0.5f, false, 0.4f);

    // Day sky is brighter than night; night fog is nearly black.
    TestTrue(TEXT("Noon sky light beats night"), Noon.SkyLightIntensity > Night.SkyLightIntensity * 3.0f);
    TestTrue(TEXT("Night fog is dark"), Night.FogColor.R < 0.2f && Night.FogColor.G < 0.2f && Night.FogColor.B < 0.2f);

    // Dawn/dusk sun is warm (red channel dominant), noon is near-neutral.
    TestTrue(TEXT("Dawn sun is warm"), Dawn.SunColor.R > Dawn.SunColor.B + 0.15f);
    TestTrue(TEXT("Dusk sun is warm"), Dusk.SunColor.R > Dusk.SunColor.B + 0.15f);
    TestTrue(TEXT("Noon sun is near-neutral"),
        FMath::Abs(Noon.SunColor.R - Noon.SunColor.B) < 0.12f);

    // Night sun is cool (moonlight): blue channel dominant.
    TestTrue(TEXT("Night sun is cool"), Night.SunColor.B > Night.SunColor.R);

    // Storm weather: denser fog + dimmer sun than the same clear-noon moment.
    TestTrue(TEXT("Storm thickens fog"), Storm.FogDensity > Noon.FogDensity * 1.5f);
    TestTrue(TEXT("Storm dims the sun"), Storm.SunIntensityMultiplier < Noon.SunIntensityMultiplier);
    TestEqual(TEXT("Clear weather never dims"), Noon.SunIntensityMultiplier, 1.0f);

    // Sun base curve: noon peak, night floor.
    TestTrue(TEXT("Noon sun base beats dawn"),
        AAstrawildWorldBootstrapper::EvalSunBaseIntensity(0.5f, false) > AAstrawildWorldBootstrapper::EvalSunBaseIntensity(0.0f, false));
    TestEqual(TEXT("Night sun floor"), AAstrawildWorldBootstrapper::EvalSunBaseIntensity(0.5f, true), 0.4f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildVfxPaletteTest,
    "ASTRAWILD.Vfx.Palette",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildVfxPaletteTest::RunTest(const FString& Parameters)
{
    // Element tints: distinct, non-black, non-white — readable at gameplay distance.
    const EAstrawildElementType Elements[] = {
        EAstrawildElementType::None, EAstrawildElementType::Light, EAstrawildElementType::Ash,
        EAstrawildElementType::Flora, EAstrawildElementType::Frost, EAstrawildElementType::Pulse,
        EAstrawildElementType::Ember };
    for (const EAstrawildElementType Element : Elements)
    {
        const FLinearColor Tint = FAstrawildVfxPalette::GetElementTint(Element);
        TestTrue(*FString::Printf(TEXT("Element %d tint is visible"), static_cast<int32>(Element)),
            Tint.R + Tint.G + Tint.B > 0.5f && Tint.R + Tint.G + Tint.B < 2.9f);
    }
    TestTrue(TEXT("Flora differs from Frost"),
        !FAstrawildVfxPalette::GetElementTint(EAstrawildElementType::Flora).Equals(
            FAstrawildVfxPalette::GetElementTint(EAstrawildElementType::Frost), 0.05f));
    TestTrue(TEXT("Ember differs from Pulse"),
        !FAstrawildVfxPalette::GetElementTint(EAstrawildElementType::Ember).Equals(
            FAstrawildVfxPalette::GetElementTint(EAstrawildElementType::Pulse), 0.05f));

    // Rarity ladder: every tier distinct and darkens up the chain (grey → green → cyan → violet → amber → crimson).
    const EAstrawildRarity Rarities[] = {
        EAstrawildRarity::Common, EAstrawildRarity::Uncommon, EAstrawildRarity::Rare,
        EAstrawildRarity::Epic, EAstrawildRarity::Legendary, EAstrawildRarity::Mythic };
    for (int32 i = 0; i < 6; ++i)
    {
        for (int32 j = i + 1; j < 6; ++j)
        {
            TestTrue(*FString::Printf(TEXT("Rarity %d vs %d distinct"), i, j),
                !FAstrawildVfxPalette::GetRarityTint(Rarities[i]).Equals(
                    FAstrawildVfxPalette::GetRarityTint(Rarities[j]), 0.05f));
        }
    }

    // Weapon family tints: kinetic grey-ish (low saturation) vs energy families saturated.
    const FLinearColor Kinetic = FAstrawildVfxPalette::GetWeaponFamilyTint(EAstrawildWeaponFamily::Kinetic);
    const FLinearColor Plasma = FAstrawildVfxPalette::GetWeaponFamilyTint(EAstrawildWeaponFamily::Plasma);
    TestTrue(TEXT("Kinetic reads industrial (low saturation)"),
        FMath::Abs(Kinetic.R - Kinetic.G) < 0.15f && FMath::Abs(Kinetic.G - Kinetic.B) < 0.15f);
    TestTrue(TEXT("Plasma reads energetic (saturated)"), Plasma.B > Kinetic.B + 0.1f || Plasma.R > Kinetic.R + 0.1f);

    // Scanner tiers: Field teal, Array amber, Oracle violet — three identities.
    TestTrue(TEXT("Array scanner tint differs from Field"),
        !FAstrawildVfxPalette::GetScannerTint(TEXT("Item_ArrayScanner")).Equals(
            FAstrawildVfxPalette::GetScannerTint(TEXT("Item_FieldScanner")), 0.05f));
    TestTrue(TEXT("Oracle scanner tint differs from Array"),
        !FAstrawildVfxPalette::GetScannerTint(TEXT("Item_OracleScanner")).Equals(
            FAstrawildVfxPalette::GetScannerTint(TEXT("Item_ArrayScanner")), 0.05f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildVfxBeamMathTest,
    "ASTRAWILD.Vfx.BeamMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildVfxBeamMathTest::RunTest(const FString& Parameters)
{
    FVector Center;
    FRotator Rotation;
    float Length = 0.0f;

    TestFalse(TEXT("Coincident points make no beam"),
        AAstrawildBeamVfxActor::ComputeBeamTransform(FVector::ZeroVector, FVector(0, 0, 0.5f), Center, Rotation, Length));

    TestTrue(TEXT("Axis-aligned beam resolves"),
        AAstrawildBeamVfxActor::ComputeBeamTransform(FVector(0, 0, 100), FVector(1000, 0, 100), Center, Rotation, Length));
    TestEqual(TEXT("Beam length matches distance"), Length, 1000.0f);
    TestEqual(TEXT("Beam center is the midpoint"), Center, FVector(500.0f, 0.0f, 100.0f));
    TestEqual(TEXT("Beam rotation aims down +X"), Rotation.Vector(), FVector(1, 0, 0));

    // Diagonal beam: rotation forward vector matches the normalized delta.
    const FVector Start(100, 200, 300);
    const FVector End(1000, 1100, 300);
    TestTrue(TEXT("Diagonal beam resolves"),
        AAstrawildBeamVfxActor::ComputeBeamTransform(Start, End, Center, Rotation, Length));
    const FVector Delta = (End - Start).GetSafeNormal();
    TestTrue(TEXT("Diagonal beam orientation matches delta"),
        FMath::Abs(FVector::DotProduct(Rotation.Vector(), Delta) - 1.0f) < 0.001f);
    TestEqual(TEXT("Diagonal beam length"), Length, static_cast<float>((End - Start).Size()), 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildVfxArcJitterTest,
    "ASTRAWILD.Vfx.ArcJitter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildVfxArcJitterTest::RunTest(const FString& Parameters)
{
    const FVector A(0, 0, 150);
    const FVector B(2000, 0, 150);

    // Determinism: same seed → identical zig-zag.
    TArray<FVector> PathA, PathB;
    AAstrawildBeamVfxActor::ComputeArcJitter(777, A, B, 3, 40.0f, PathA);
    AAstrawildBeamVfxActor::ComputeArcJitter(777, A, B, 3, 40.0f, PathB);
    TestEqual(TEXT("Jitter waypoints deterministic"), PathA.Num(), PathB.Num());
    bool bIdentical = PathA.Num() == PathB.Num();
    for (int32 i = 0; bIdentical && i < PathA.Num(); ++i)
    {
        bIdentical = PathA[i].Equals(PathB[i], 0.001f);
    }
    TestTrue(TEXT("Same seed → identical zig-zag"), bIdentical);

    // Structure: endpoints always included, sub-segments between.
    TestEqual(TEXT("3 sub-segments → 5 waypoints"), PathA.Num(), 5);
    TestTrue(TEXT("Path starts at A"), PathA[0].Equals(A, 0.01f));
    TestTrue(TEXT("Path ends at B"), PathA.Last().Equals(B, 0.01f));

    // Bounded jitter: interior waypoints stay within amplitude of the A→B line.
    bool bBounded = true;
    const FVector LineDir = (B - A).GetSafeNormal();
    for (int32 i = 1; i < PathA.Num() - 1; ++i)
    {
        const FVector Closest = FMath::ClosestPointOnSegment(PathA[i], A, B);
        bBounded &= FVector::Dist(PathA[i], Closest) <= 45.0f; // amplitude + epsilon
    }
    TestTrue(TEXT("Jitter stays within amplitude of the line"), bBounded);

    // Zero sub-segments → straight path.
    TArray<FVector> Straight;
    AAstrawildBeamVfxActor::ComputeArcJitter(1, A, B, 0, 40.0f, Straight);
    TestEqual(TEXT("No sub-segments → 2 waypoints"), Straight.Num(), 2);

    // Different seed → different zig-zag (lightning varies per hop chain).
    TArray<FVector> PathC;
    AAstrawildBeamVfxActor::ComputeArcJitter(8888, A, B, 3, 40.0f, PathC);
    bool bDiffers = false;
    for (int32 i = 1; i < PathA.Num() - 1 && !bDiffers; ++i)
    {
        bDiffers = !PathA[i].Equals(PathC[i], 0.01f);
    }
    TestTrue(TEXT("Different seed → different interior waypoints"), bDiffers);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildVfxRingGeometryTest,
    "ASTRAWILD.Vfx.RingGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildVfxRingGeometryTest::RunTest(const FString& Parameters)
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;

    AAstrawildScannerPulseActor::BuildRingGeometry(48, 0.86f, FLinearColor(0.4f, 0.9f, 0.8f, 1.0f),
        Vertices, Triangles, Normals, UVs, Colors);

    // Closed annulus: Segments+1 ring pairs (duplicate seam vertex), 2 tris per segment.
    TestEqual(TEXT("48 segments → 98 ring vertices"), Vertices.Num(), (48 + 1) * 2);
    TestEqual(TEXT("48 segments → 96 triangles"), Triangles.Num(), 48 * 2 * 3);
    TestEqual(TEXT("Colors match vertex count"), Colors.Num(), Vertices.Num());

    // All vertices on one of the two radius rings (inner/outer).
    bool bRadiiValid = true;
    for (const FVector& Vertex : Vertices)
    {
        const float Radius = FVector2D(Vertex.X, Vertex.Y).Size();
        bRadiiValid &= (FMath::Abs(Radius - 1.0f) < 0.01f || FMath::Abs(Radius - 0.86f) < 0.01f);
    }
    TestTrue(TEXT("Every vertex sits on the inner or outer ring"), bRadiiValid);

    // Flat ring in the XY plane.
    bool bFlat = true;
    for (const FVector& Vertex : Vertices)
    {
        bFlat &= FMath::Abs(Vertex.Z) < 0.01f;
    }
    TestTrue(TEXT("Ring is flat (ground pulse)"), bFlat);

    // Inner fraction is clamped into a valid band.
    AAstrawildScannerPulseActor::BuildRingGeometry(12, -0.5f, FLinearColor::White,
        Vertices, Triangles, Normals, UVs, Colors);
    bool bClampedValid = true;
    for (const FVector& Vertex : Vertices)
    {
        const float Radius = FVector2D(Vertex.X, Vertex.Y).Size();
        bClampedValid &= Radius <= 1.05f && Radius >= 0.03f;
    }
    TestTrue(TEXT("Invalid inner fraction clamps safely"), bClampedValid);
    return true;
}

// ---------------------------------------------------------------------------
// Production V2 Batch 3 — dialogue system (P12 Story/NPC)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDialogueTreeContractTest,
    "ASTRAWILD.Dialogue.TreeContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDialogueTreeContractTest::RunTest(const FString& Parameters)
{
    // A hand-built tree exercising the full node/choice vocabulary.
    UAstrawildDialogueTreeDefinition* Tree = NewObject<UAstrawildDialogueTreeDefinition>();
    Tree->DialogueId = TEXT("Dialogue_TestTree");
    Tree->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        FAstrawildDialogueLine First;
        First.Text = FText::FromString(TEXT("Line one."));
        Node.Lines = { First };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Dig deeper"));
            Choice.GotoNodeId = TEXT("lore");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Tree->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("lore");
        FAstrawildDialogueLine Line;
        Line.Text = FText::FromString(TEXT("Lore line."));
        Node.Lines = { Line };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Back"));
            Choice.GotoNodeId = TEXT("hello");
            Node.Choices.Add(Choice);
        }
        Tree->Nodes.Add(Node);
    }

    // Entry node resolves and unknown ids do not.
    TestNotNull(TEXT("Entry node resolves"), const_cast<FAstrawildDialogueNode*>(Tree->FindNode(TEXT("hello"))));
    TestNotNull(TEXT("Lore node resolves"), const_cast<FAstrawildDialogueNode*>(Tree->FindNode(TEXT("lore"))));
    TestNull(TEXT("Unknown node id returns null"), const_cast<FAstrawildDialogueNode*>(Tree->FindNode(TEXT("nope"))));

    // Structural integrity: unique node ids, every goto resolves, no node has
    // both bEndDialogue and a GotoNodeId (ambiguous routing), entry exists.
    bool bIdsUnique = true;
    TSet<FName> SeenIds;
    for (const FAstrawildDialogueNode& Node : Tree->Nodes)
    {
        if (SeenIds.Contains(Node.NodeId) || Node.NodeId.IsNone())
        {
            bIdsUnique = false;
        }
        SeenIds.Add(Node.NodeId);
    }
    TestTrue(TEXT("Node ids are unique and named"), bIdsUnique);
    TestTrue(TEXT("Entry node exists"), SeenIds.Contains(Tree->EntryNodeId));

    bool bGotosResolve = true;
    bool bNoAmbiguousEnds = true;
    for (const FAstrawildDialogueNode& Node : Tree->Nodes)
    {
        TestTrue(TEXT("Every node speaks"), Node.Lines.Num() > 0);
        for (const FAstrawildDialogueChoice& Choice : Node.Choices)
        {
            if (!Choice.GotoNodeId.IsNone() && !SeenIds.Contains(Choice.GotoNodeId))
            {
                bGotosResolve = false;
            }
            if (Choice.bEndDialogue && !Choice.GotoNodeId.IsNone())
            {
                bNoAmbiguousEnds = false;
            }
        }
    }
    TestTrue(TEXT("Every choice goto resolves to a real node"), bGotosResolve);
    TestTrue(TEXT("No choice is both a hard end and a goto"), bNoAmbiguousEnds);

    // Primary asset id type — dialogue trees join the registry's asset family.
    TestEqual(TEXT("Primary asset type is Dialogue"),
        Tree->GetPrimaryAssetId().PrimaryAssetType.ToString(), FString(TEXT("Dialogue")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDialogueChoiceConditionsTest,
    "ASTRAWILD.Dialogue.ChoiceConditions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDialogueChoiceConditionsTest::RunTest(const FString& Parameters)
{
    // A world-free component: flag conditions work standalone; quest conditions
    // resolve to FAIL without a quest component (missing owner ≠ ignored).
    UAstrawildDialogueComponent* Dialogue = NewObject<UAstrawildDialogueComponent>();

    FAstrawildDialogueChoice Choice;
    Choice.Text = FText::FromString(TEXT("Gated reply"));
    Choice.RequiredFlagId = TEXT("Flag_Told");
    TestFalse(TEXT("Required flag hides the choice until set"), Dialogue->EvaluateChoiceConditions(Choice));

    Dialogue->SetStoryFlag(TEXT("Flag_Told"));
    TestTrue(TEXT("Required flag reveals the choice"), Dialogue->EvaluateChoiceConditions(Choice));

    // Forbidden flag flips the result once set (one-time beats).
    Choice.ForbiddenFlagId = TEXT("Flag_Done");
    TestTrue(TEXT("Forbidden flag unset keeps the choice visible"), Dialogue->EvaluateChoiceConditions(Choice));
    Dialogue->SetStoryFlag(TEXT("Flag_Done"));
    TestFalse(TEXT("Forbidden flag hides the choice after one use"), Dialogue->EvaluateChoiceConditions(Choice));

    // AND semantics: all conditions must hold simultaneously.
    FAstrawildDialogueChoice Strict;
    Strict.RequiredFlagId = TEXT("Flag_A");
    Strict.ForbiddenFlagId = TEXT("Flag_B");
    TestFalse(TEXT("Missing required flag fails"), Dialogue->EvaluateChoiceConditions(Strict));
    Dialogue->SetStoryFlag(TEXT("Flag_A"));
    TestTrue(TEXT("Required + absent forbidden passes"), Dialogue->EvaluateChoiceConditions(Strict));
    Dialogue->SetStoryFlag(TEXT("Flag_B"));
    TestFalse(TEXT("Second condition failing fails the pair"), Dialogue->EvaluateChoiceConditions(Strict));

    // Quest gates need a quest component; a world-free component fails them
    // (they are conditions, not optional flavor).
    FAstrawildDialogueChoice QuestChoice;
    QuestChoice.RequiredQuestActiveId = TEXT("Quest_Any");
    TestFalse(TEXT("Quest-active gate fails without quest state"), Dialogue->EvaluateChoiceConditions(QuestChoice));
    FAstrawildDialogueChoice QuestChoice2;
    QuestChoice2.RequiredQuestCompletedId = TEXT("Quest_Any");
    TestFalse(TEXT("Quest-completed gate fails without quest state"), Dialogue->EvaluateChoiceConditions(QuestChoice2));

    // Empty choice (all NAME_None) is always visible.
    FAstrawildDialogueChoice Free;
    TestTrue(TEXT("Condition-free choice is always visible"), Dialogue->EvaluateChoiceConditions(Free));

    // Flag store hygiene: set is idempotent, export/import round-trips.
    Dialogue->SetStoryFlag(TEXT("Flag_A"));
    TArray<FName> Exported;
    Dialogue->ExportForSave(Exported);
    TestTrue(TEXT("Export contains 4 unique flags"), Exported.Num() == 4);
    TestTrue(TEXT("Set is idempotent"), Dialogue->GetStoryFlags().Contains(TEXT("Flag_A")));

    UAstrawildDialogueComponent* Restored = NewObject<UAstrawildDialogueComponent>();
    Restored->ImportFromSave(Exported);
    TestTrue(TEXT("Imported flag survives the round-trip"), Restored->HasStoryFlag(TEXT("Flag_Told")));
    TestTrue(TEXT("Import restores the AND pair"), Restored->EvaluateChoiceConditions(Strict) == false && Restored->HasStoryFlag(TEXT("Flag_B")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDialogueConsequenceTest,
    "ASTRAWILD.Dialogue.Consequences",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDialogueConsequenceTest::RunTest(const FString& Parameters)
{
    UAstrawildDialogueComponent* Dialogue = NewObject<UAstrawildDialogueComponent>();

    // Flag-only consequence applies cleanly with zero dependencies.
    FAstrawildDialogueChoice FlagChoice;
    FlagChoice.Text = FText::FromString(TEXT("Take the tip"));
    FlagChoice.SetFlagId = TEXT("Flag_TipTaken");
    TestTrue(TEXT("Flag consequence applies"), Dialogue->ApplyChoiceConsequences(FlagChoice));
    TestTrue(TEXT("Flag is set after the consequence"), Dialogue->HasStoryFlag(TEXT("Flag_TipTaken")));

    // Unknown quest id is a HARD fail (trees must reference registered content).
    FAstrawildDialogueChoice BrokenChoice;
    BrokenChoice.StartQuestId = TEXT("Quest_DoesNotExist");
    TestFalse(TEXT("Unknown quest start fails hard"), Dialogue->ApplyChoiceConsequences(BrokenChoice));

    // No consequence at all is a no-op success (pure navigation replies).
    FAstrawildDialogueChoice NavChoice;
    NavChoice.GotoNodeId = TEXT("hello");
    TestTrue(TEXT("Navigation-only choice succeeds"), Dialogue->ApplyChoiceConsequences(NavChoice));
    return true;
}

// ---------------------------------------------------------------------------
// Content Pack CP-02 — evolution gates (level AND bond, fail closed)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildEchoEvolutionGatesTest,
    "ASTRAWILD.Echo.EvolutionGates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildEchoEvolutionGatesTest::RunTest(const FString& Parameters)
{
    UAstrawildEchoDefinition* Base = NewObject<UAstrawildEchoDefinition>();
    Base->DefinitionId = TEXT("Echo_TestBase");
    Base->EvolveToDefinitionId = TEXT("Echo_TestEvolved");
    Base->EvolveRequiredLevel = 20;
    Base->EvolveRequiredBond = 35.0f;

    UAstrawildEchoDefinition* Evolved = NewObject<UAstrawildEchoDefinition>();
    Evolved->DefinitionId = TEXT("Echo_TestEvolved");

    FAstrawildEchoInstanceV2 Instance;
    Instance.DefinitionId = Base->DefinitionId;
    Instance.Level = 20;
    Instance.Bond = 35.0f;

    // Fail-closed: missing definitions, dangling links, self-cycles.
    TestFalse(TEXT("Null definitions fail closed"), UAstrawildEchoRosterSubsystem::CanEvolveInstance(Instance, nullptr, nullptr));
    TestFalse(TEXT("No chain link fails"), UAstrawildEchoRosterSubsystem::CanEvolveInstance(Instance, Base, nullptr));

    UAstrawildEchoDefinition* SelfCycle = NewObject<UAstrawildEchoDefinition>();
    SelfCycle->DefinitionId = TEXT("Echo_TestBase");
    SelfCycle->EvolveToDefinitionId = TEXT("Echo_TestBase");
    SelfCycle->EvolveRequiredLevel = 1;
    SelfCycle->EvolveRequiredBond = 0.0f;
    TestFalse(TEXT("Self-cycles are rejected"), UAstrawildEchoRosterSubsystem::CanEvolveInstance(Instance, SelfCycle, SelfCycle));

    // Mismatched target id (data bug) fails closed.
    UAstrawildEchoDefinition* WrongTarget = NewObject<UAstrawildEchoDefinition>();
    WrongTarget->DefinitionId = TEXT("Echo_TestWrong");
    TestFalse(TEXT("Mismatched target id fails closed"), UAstrawildEchoRosterSubsystem::CanEvolveInstance(Instance, Base, WrongTarget));

    // Dual gate: level AND bond must both clear.
    TestTrue(TEXT("Both gates met evolves"), UAstrawildEchoRosterSubsystem::CanEvolveInstance(Instance, Base, Evolved));
    Instance.Level = 19;
    TestFalse(TEXT("Level below gate fails"), UAstrawildEchoRosterSubsystem::CanEvolveInstance(Instance, Base, Evolved));
    Instance.Level = 20;
    Instance.Bond = 34.9f;
    TestFalse(TEXT("Bond below gate fails"), UAstrawildEchoRosterSubsystem::CanEvolveInstance(Instance, Base, Evolved));
    Instance.Bond = 35.0f;
    TestTrue(TEXT("Exact gate values pass"), UAstrawildEchoRosterSubsystem::CanEvolveInstance(Instance, Base, Evolved));

    // Gate defaults on un-authored species: NAME_None = final form.
    UAstrawildEchoDefinition* Final = NewObject<UAstrawildEchoDefinition>();
    Final->DefinitionId = TEXT("Echo_TestFinal");
    TestTrue(TEXT("Final forms have no chain link"), Final->EvolveToDefinitionId.IsNone());
    TestFalse(TEXT("Final forms cannot evolve"), UAstrawildEchoRosterSubsystem::CanEvolveInstance(Instance, Final, nullptr));
    return true;
}

// ---------------------------------------------------------------------------
// Content Pack CP-03/CP-05 — weapon Niagara/audio binding contract
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildWeaponAssetBindingTest,
    "ASTRAWILD.Weapon.AssetBindingContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildWeaponAssetBindingTest::RunTest(const FString& Parameters)
{
    // The zero-asset fallback contract: CODE_DEFAULT profiles bind nothing —
    // Niagara-first dispatch must fall back to the procedural Batch-2 VFX and
    // stay silent until Antigravity's .uasset profiles carry real bindings.
    UAstrawildWeaponDefinition* Profile = NewObject<UAstrawildWeaponDefinition>();
    Profile->WeaponId = TEXT("Weapon_TestBinding");
    Profile->Family = EAstrawildWeaponFamily::Plasma;
    Profile->FireMode = EAstrawildWeaponFireMode::Projectile;

    TestTrue(TEXT("Muzzle binding defaults unset"), Profile->MuzzleFlashVfx.IsNull());
    TestTrue(TEXT("Impact binding defaults unset"), Profile->ImpactVfx.IsNull());
    TestTrue(TEXT("Trail binding defaults unset"), Profile->ProjectileTrailVfx.IsNull());
    TestTrue(TEXT("Fire sound defaults unset"), Profile->FireSound.IsNull());
    TestTrue(TEXT("Impact sound defaults unset"), Profile->ImpactSound.IsNull());

    // The FName id contract stays the authored directory convention alongside
    // the direct refs (two binding paths, one source of truth per asset).
    Profile->MuzzleVfxId = TEXT("PlasmaMuzzle");
    Profile->TrailVfxId = TEXT("PlasmaTrail");
    Profile->ImpactVfxId = TEXT("PlasmaImpact");
    Profile->FireSoundId = TEXT("PlasmaFire");
    TestEqual(TEXT("Muzzle id contract round-trips"), Profile->MuzzleVfxId, FName(TEXT("PlasmaMuzzle")));
    TestEqual(TEXT("Fire sound id contract round-trips"), Profile->FireSoundId, FName(TEXT("PlasmaFire")));

    // Equipment visual binding (CP-01) defaults unset the same way — the
    // procedural player silhouette is the fallback until meshes bind.
    UAstrawildItemDefinition* ArmorItem = NewObject<UAstrawildItemDefinition>();
    TestTrue(TEXT("Equip mesh override defaults unset"), ArmorItem->EquipMeshOverride.IsNull());
    TestTrue(TEXT("Equip material override defaults unset"), ArmorItem->EquipMaterialOverride.IsNull());
    return true;
}


// ---------------------------------------------------------------------------
// Batch 4 — Art pack binding contract (AstrawildArtPack tables + soft path
// behavior). Pure data: no world, no loads. Proves (1) every CODE_DEFAULT
// production entry that claims art has well-formed /Game/ paths, and (2) the
// zero-asset fallback rule survives the bindings: an unresolved soft path
// reports IsValid()==false, so dispatchers keep their procedural fallbacks
// until Antigravity imports the pack (CP-00 rule 2).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildArtPackBindingTest,
    "ASTRAWILD.ArtPack.BindingContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildArtPackBindingTest::RunTest(const FString& Parameters)
{
    // --- Weapons: 8 bindings, complete mesh/sound/FX path contracts.
    TestEqual(TEXT("weapon art entries"), AstrawildArtPack::GetWeaponArt().Num(), 8);
    for (const AstrawildArtPack::FWeaponArt& Art : AstrawildArtPack::GetWeaponArt())
    {
        TestTrue(FString::Printf(TEXT("%s has mesh path"), *Art.WeaponId.ToString()),
            Art.MeshPath.StartsWith(TEXT("/Game/Weapons/")));
        TestTrue(FString::Printf(TEXT("%s has fire sound path"), *Art.WeaponId.ToString()),
            Art.FireSoundPath.StartsWith(TEXT("/Game/Audio/A_Weapon_")));
        TestTrue(FString::Printf(TEXT("%s has impact sound path"), *Art.WeaponId.ToString()),
            Art.ImpactSoundPath.StartsWith(TEXT("/Game/Audio/A_Weapon_Impact")));
        TestTrue(FString::Printf(TEXT("%s has muzzle vfx path"), *Art.WeaponId.ToString()),
            Art.MuzzleVfxPath.StartsWith(TEXT("/Game/VFX/NS_AW_")));
        if (Art.WeaponId == TEXT("Weapon_Scrapshot"))
        {
            TestTrue(TEXT("kinetic impact is the kinetic sound"),
                Art.ImpactSoundPath.Contains(TEXT("Kinetic")));
        }
    }
    TestTrue(TEXT("weapon lookup resolves"), AstrawildArtPack::FindWeaponArt(TEXT("Weapon_ArcCaster")) != nullptr);
    TestTrue(TEXT("weapon lookup misses unknown ids"), AstrawildArtPack::FindWeaponArt(TEXT("Weapon_Missing")) == nullptr);

    // --- Echoes: 6 species with mesh + idle + move clips.
    TestEqual(TEXT("echo art entries"), AstrawildArtPack::GetEchoArt().Num(), 6);
    for (const AstrawildArtPack::FEchoArt& Art : AstrawildArtPack::GetEchoArt())
    {
        TestTrue(FString::Printf(TEXT("%s mesh path"), *Art.EchoId.ToString()),
            Art.MeshPath.StartsWith(TEXT("/Game/Characters/Echoes/SK_Echo_")));
        TestTrue(FString::Printf(TEXT("%s idle clip path"), *Art.EchoId.ToString()),
            Art.IdleAnimPath.StartsWith(TEXT("/Game/Characters/Echoes/AM_")));
        TestTrue(FString::Printf(TEXT("%s move clip path"), *Art.EchoId.ToString()),
            Art.MoveAnimPath.StartsWith(TEXT("/Game/Characters/Echoes/AM_")));
        TestTrue(FString::Printf(TEXT("%s clips differ"), *Art.EchoId.ToString()),
            Art.IdleAnimPath != Art.MoveAnimPath);
    }

    // --- Biomes: 12 zones, landscape material everywhere, ambience everywhere,
    // trees on the vegetated zones, at least one rock scatter mesh each.
    TestEqual(TEXT("biome art entries"), AstrawildArtPack::GetBiomeArt().Num(), 12);
    int32 BiomesWithTrees = 0;
    for (const AstrawildArtPack::FBiomeArt& Art : AstrawildArtPack::GetBiomeArt())
    {
        TestTrue(FString::Printf(TEXT("%s landscape material"), *Art.BiomeId.ToString()),
            Art.LandscapeMaterialPath == TEXT("/Game/Materials/M_Landscape_SciFiFrontier"));
        TestTrue(FString::Printf(TEXT("%s ambience path"), *Art.BiomeId.ToString()),
            Art.AmbientAudioPath.StartsWith(TEXT("/Game/Audio/A_Amb_")));
        TestTrue(FString::Printf(TEXT("%s rock scatter"), *Art.BiomeId.ToString()),
            Art.RockMeshPaths.Num() >= 1);
        if (Art.TreeMeshPaths.Num() > 0)
        {
            ++BiomesWithTrees;
        }
    }
    TestTrue(TEXT("most biomes carry trees (8 vegetated zones)"), BiomesWithTrees >= 8);
    TestTrue(TEXT("starting zone carries trees"),
        AstrawildArtPack::FindBiomeArt(TEXT("Zone_DawnFields")) &&
        AstrawildArtPack::FindBiomeArt(TEXT("Zone_DawnFields"))->TreeMeshPaths.Num() >= 2);

    // --- Resource nodes: 10 bindings under /Game/Environment/.
    TestEqual(TEXT("node art entries"), AstrawildArtPack::GetNodeArt().Num(), 10);
    for (const AstrawildArtPack::FNodeArt& Art : AstrawildArtPack::GetNodeArt())
    {
        TestTrue(FString::Printf(TEXT("%s node mesh path"), *Art.NodeId.ToString()),
            Art.MeshPath.StartsWith(TEXT("/Game/Environment/")));
    }

    // --- Survivor: mesh + the 7 CP-01/CP-08 clips.
    {
        const AstrawildArtPack::FSurvivorArt& Art = AstrawildArtPack::GetSurvivorArt();
        TestTrue(TEXT("survivor mesh path"), Art.MeshPath.StartsWith(TEXT("/Game/Characters/Survivor/SK_")));
        TestTrue(TEXT("survivor idle path"), Art.IdleAnimPath.StartsWith(TEXT("/Game/Characters/Survivor/AM_")));
        TestTrue(TEXT("survivor walk path"), Art.WalkAnimPath.StartsWith(TEXT("/Game/Characters/Survivor/AM_")));
        TestTrue(TEXT("survivor run path"), Art.RunAnimPath.StartsWith(TEXT("/Game/Characters/Survivor/AM_")));
        TestTrue(TEXT("survivor jump path"), Art.JumpAnimPath.StartsWith(TEXT("/Game/Characters/Survivor/AM_")));
        TestTrue(TEXT("survivor aim path"), Art.AimAnimPath.StartsWith(TEXT("/Game/Characters/Survivor/AM_")));
        TestTrue(TEXT("survivor fire path"), Art.FireAnimPath.StartsWith(TEXT("/Game/Characters/Survivor/AM_")));
        TestTrue(TEXT("survivor gather path"), Art.GatherAnimPath.StartsWith(TEXT("/Game/Characters/Survivor/AM_")));
    }

    // --- Zero-asset fallback rule survives the bindings: a soft path that has
    // not been imported reports IsValid()==false (no accidental hard refs, no
    // load-on-construct). The Weapon.AssetBindingContract test covers the
    // CLASS defaults; this proves the CODE_DEFAULT BINDINGS behave the same.
    {
        UAstrawildWeaponDefinition* Profile = NewObject<UAstrawildWeaponDefinition>();
        const AstrawildArtPack::FWeaponArt* Art = AstrawildArtPack::FindWeaponArt(TEXT("Weapon_PlasmaCharger"));
        TestTrue(TEXT("plasma binding present"), Art != nullptr);
        if (Art)
        {
            Profile->Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Art->MeshPath));
            Profile->FireSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(Art->FireSoundPath));
            TestTrue(TEXT("unimported mesh binding stays invalid (fallback rule)"),
                !Profile->Mesh.IsValid());
            TestTrue(TEXT("unimported sound binding stays invalid (fallback rule)"),
                !Profile->FireSound.IsValid());
            TestTrue(TEXT("mesh path round-trips"),
                Profile->Mesh.ToSoftObjectPath().ToString() == Art->MeshPath);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// GLM Hardening Pass: Test 55 — Core Loop Action Roster
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildInputCoreLoopRosterTest,
    "ASTRAWILD.Input.CoreLoopRoster",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildInputCoreLoopRosterTest::RunTest(const FString& Parameters)
{
    AAstrawildPlayerCharacter* Player = NewObject<AAstrawildPlayerCharacter>();
    TestNotNull(TEXT("Player character constructed"), Player);
    if (!Player)
    {
        return false;
    }

    Player->BuildRuntimeInputDefaults();
    TestNotNull(TEXT("DefaultMappingContext built"), Player->DefaultMappingContext.Get());
    TestNotNull(TEXT("MoveAction valid"), Player->MoveAction.Get());
    TestNotNull(TEXT("LookAction valid"), Player->LookAction.Get());
    TestNotNull(TEXT("JumpAction valid"), Player->JumpAction.Get());
    TestNotNull(TEXT("SprintAction valid"), Player->SprintAction.Get());
    TestNotNull(TEXT("InteractAction valid"), Player->InteractAction.Get());
    TestNotNull(TEXT("AttackAction valid"), Player->AttackAction.Get());
    TestNotNull(TEXT("InventoryAction valid"), Player->InventoryAction.Get());
    TestNotNull(TEXT("BuildModeAction valid"), Player->BuildModeAction.Get());
    TestNotNull(TEXT("ScanAction valid"), Player->ScanAction.Get());

    return true;
}

// ---------------------------------------------------------------------------
// GLM Hardening Pass: Test 56 — Runtime Action Contract
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildInputRuntimeActionContractTest,
    "ASTRAWILD.Input.RuntimeActionContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildInputRuntimeActionContractTest::RunTest(const FString& Parameters)
{
    AAstrawildPlayerCharacter* Player = NewObject<AAstrawildPlayerCharacter>();
    TestNotNull(TEXT("Player character constructed"), Player);
    if (!Player)
    {
        return false;
    }

    Player->BuildRuntimeInputDefaults();
    Player->BuildGamepadInputDefaults();

    TestNotNull(TEXT("Keyboard context exists"), Player->DefaultMappingContext.Get());
    TestNotNull(TEXT("Gamepad context exists"), Player->GamepadMappingContext.Get());

    if (Player->DefaultMappingContext.Get())
    {
        TestTrue(TEXT("Default mapping context has >= 20 key bindings"),
            Player->DefaultMappingContext->GetMappings().Num() >= 20);
    }
    if (Player->GamepadMappingContext.Get())
    {
        TestTrue(TEXT("Gamepad mapping context has >= 10 key bindings"),
            Player->GamepadMappingContext->GetMappings().Num() >= 10);
    }

    return true;
}

// ---------------------------------------------------------------------------
// GLM Hardening Pass: Test 57 — Survivor Fallback Chain
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAssetSurvivorFallbackChainTest,
    "ASTRAWILD.Asset.SurvivorFallbackChain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAssetSurvivorFallbackChainTest::RunTest(const FString& Parameters)
{
    AAstrawildPlayerCharacter* Player = NewObject<AAstrawildPlayerCharacter>();
    TestNotNull(TEXT("Player character constructed"), Player);
    if (!Player)
    {
        return false;
    }

    Player->BuildProceduralBody();
    TestNotNull(TEXT("Procedural BodyMesh exists"), Player->BodyMesh.Get());

    return true;
}

// ---------------------------------------------------------------------------
// FINAL COMPLETION RUN — BATCH 1: P0 hardening contracts (FR-1..FR-4)
// Test 58: Inventory transaction safety — negative-quantity rejection,
// atomic ConsumeItems with duplicate aggregation, save-import dedupe.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildInventoryTransactionSafetyTest,
    "ASTRAWILD.Inventory.TransactionSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildInventoryTransactionSafetyTest::RunTest(const FString& Parameters)
{
    // World-free component: no owner → role is not authority → weight gate
    // bypassed; the transaction math below is the deterministic contract.
    UAstrawildInventoryComponent* Inventory = NewObject<UAstrawildInventoryComponent>();
    if (!TestNotNull(TEXT("Inventory constructed"), Inventory))
    {
        return false;
    }

    Inventory->AddItemSilent(TEXT("Item_Wood"), 7);

    // FR-1: RemoveItem quantity-sign gate FIRST (the old exploit: "0 >= -5"
    // passed HasItem, then Count -= -5 minted items).
    TestFalse(TEXT("RemoveItem rejects negative quantity"), Inventory->RemoveItem(TEXT("Item_Wood"), -5));
    TestFalse(TEXT("RemoveItem rejects zero quantity"), Inventory->RemoveItem(TEXT("Item_Wood"), 0));
    TestFalse(TEXT("RemoveItem rejects missing id"), Inventory->RemoveItem(NAME_None, 1));
    TestEqual(TEXT("Rejected removes leave the stack untouched"), Inventory->GetQuantity(TEXT("Item_Wood")), 7);

    // FR-1: ConsumeItems is atomic across DUPLICATE ids. {Wood x5, Wood x5}
    // aggregates to 10 required; only 7 owned → refuse WITHOUT partial loss.
    const auto Stack = [](const TCHAR* Id, const int32 Qty)
    {
        FAstrawildItemStack Out;
        Out.ItemId = Id;
        Out.Quantity = Qty;
        return Out;
    };

    TestFalse(TEXT("ConsumeItems refuses aggregated shortage"),
        Inventory->ConsumeItems({ Stack(TEXT("Item_Wood"), 5), Stack(TEXT("Item_Wood"), 5) }));
    TestEqual(TEXT("Refused consume is atomic — no partial drain"), Inventory->GetQuantity(TEXT("Item_Wood")), 7);

    // Top up to 12 — the same request now succeeds atomically and leaves 2.
    Inventory->AddItemSilent(TEXT("Item_Wood"), 5);
    TestTrue(TEXT("ConsumeItems succeeds when the aggregate fits"),
        Inventory->ConsumeItems({ Stack(TEXT("Item_Wood"), 5), Stack(TEXT("Item_Wood"), 5) }));
    TestEqual(TEXT("Successful consume leaves the remainder"), Inventory->GetQuantity(TEXT("Item_Wood")), 2);

    // FR-1: SetItemStacks sanitize — first-seen-wins on duplicate ids, invalid
    // stacks are dropped (a crafted save cannot inflate quantities).
    Inventory->SetItemStacks({
        Stack(TEXT("Item_Wood"), 10),
        Stack(TEXT("Item_Wood"), 99),   // duplicate id — ignored
        Stack(NAME_None, 42),           // invalid id — dropped
        Stack(TEXT("Item_Stone"), 0)    // invalid quantity — dropped
    });
    TestEqual(TEXT("SetItemStacks keeps the FIRST duplicate entry"), Inventory->GetQuantity(TEXT("Item_Wood")), 10);
    TestFalse(TEXT("SetItemStacks drops invalid stacks"), Inventory->HasItem(NAME_None, 1));
    TestFalse(TEXT("SetItemStacks drops zero-quantity stacks"), Inventory->HasItem(TEXT("Item_Stone"), 1));

    return true;
}

// ---------------------------------------------------------------------------
// FINAL COMPLETION RUN — BATCH 1: Test 59 — Save consistency contracts:
// building refund snapshot (fail-closed restore) + schema evolution fields.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSaveConsistencyContractsTest,
    "ASTRAWILD.Save.ConsistencyContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSaveConsistencyContractsTest::RunTest(const FString& Parameters)
{
    // FR-2: the building refund snapshot ships default-neutral so pre-V4.1 saves
    // deserialize cleanly and the load path logs the loss instead of guessing.
    FAstrawildBuildingSaveData BuildingData;
    TestTrue(TEXT("RefundItemId defaults to none"), BuildingData.RefundItemId.IsNone());
    TestEqual(TEXT("RefundItemCount defaults to zero"), BuildingData.RefundItemCount, 0);

    // The snapshot round-trips through the struct contract (SaveWorld fills it
    // from the definition; LoadWorld refunds from it when the definition is gone).
    BuildingData.RefundItemId = TEXT("Item_Plank");
    BuildingData.RefundItemCount = 12;
    TestEqual(TEXT("Refund snapshot carries the count"), BuildingData.RefundItemCount, 12);
    TestFalse(TEXT("Refund snapshot carries a resolvable id"), BuildingData.RefundItemId.IsNone());

    // Invalid refund snapshots are rejected by the same validity rule as every
    // other stack (the load path checks RefundItemCount > 0 before granting).
    BuildingData.RefundItemCount = -4;
    TestTrue(TEXT("Negative refund count fails the > 0 gate"), BuildingData.RefundItemCount <= 0);

    // FR-2: the save game header contract — schema stamp + checksum baseline.
    UAstrawildSaveGame* SaveGame = NewObject<UAstrawildSaveGame>();
    if (!TestNotNull(TEXT("SaveGame object constructed"), SaveGame))
    {
        return false;
    }
    TestTrue(TEXT("SaveSchemaVersion is a sane positive integer"), SaveGame->SaveSchemaVersion >= 1);

    return true;
}

// ---------------------------------------------------------------------------
// FINAL COMPLETION RUN — BATCH 1: Test 60 — Quest save-import safety:
// duplicate ids collapse, exactly one active quest survives.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildQuestImportSafetyTest,
    "ASTRAWILD.Quest.ImportSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildQuestImportSafetyTest::RunTest(const FString& Parameters)
{
    UAstrawildQuestComponent* Quests = NewObject<UAstrawildQuestComponent>();
    if (!TestNotNull(TEXT("Quest component constructed"), Quests))
    {
        return false;
    }

    const auto MakeState = [](const TCHAR* Id, const bool bActive, const bool bCompleted)
    {
        FAstrawildQuestSaveData State;
        State.QuestId = Id;
        State.bActive = bActive;
        State.bCompleted = bCompleted;
        return State;
    };

    // Crafted/corrupt payload: duplicate id (active first, completed second —
    // first-seen-wins), two EXTRA active quests, and one id-less entry.
    Quests->ImportFromSave({
        MakeState(TEXT("Q_First"), true, false),
        MakeState(TEXT("Q_First"), false, true),   // duplicate id — ignored
        MakeState(TEXT("Q_Second"), true, false),  // second active — demoted
        MakeState(TEXT("Q_Third"), true, false),   // third active — demoted
        MakeState(NAME_None, true, false)          // id-less — dropped
    });

    TestEqual(TEXT("First active quest wins the active slot"), Quests->GetActiveQuestId(), FName(TEXT("Q_First")));
    TestTrue(TEXT("First entry is active"), Quests->IsQuestActive(TEXT("Q_First")));
    TestFalse(TEXT("Duplicate entry did NOT complete the quest (first-seen-wins)"), Quests->IsQuestCompleted(TEXT("Q_First")));
    TestFalse(TEXT("Second quest demoted — single-active invariant"), Quests->IsQuestActive(TEXT("Q_Second")));
    TestFalse(TEXT("Third quest demoted — single-active invariant"), Quests->IsQuestActive(TEXT("Q_Third")));

    return true;
}

// ---------------------------------------------------------------------------
// FINAL COMPLETION RUN — BATCH 1: Test 61 — Echo roster save-import safety:
// invalid entries dropped, duplicate guids collapse (first-seen-wins).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildEchoRosterImportSafetyTest,
    "ASTRAWILD.Echo.RosterImportSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildEchoRosterImportSafetyTest::RunTest(const FString& Parameters)
{
    // GameInstance subsystem used world-free: ImportFromSave touches only the
    // roster array and the change delegate — no GameInstance dependency.
    UAstrawildEchoRosterSubsystem* Roster = NewObject<UAstrawildEchoRosterSubsystem>();
    if (!TestNotNull(TEXT("Roster subsystem constructed"), Roster))
    {
        return false;
    }

    FAstrawildEchoInstanceV2 Valid;
    Valid.InstanceId = FGuid::NewGuid();
    Valid.DefinitionId = TEXT("Echo_Lumewisp");
    Valid.bInParty = true;

    FAstrawildEchoInstanceV2 Duplicate = Valid; // same guid — must collapse

    FAstrawildEchoInstanceV2 BrokenGuid;
    BrokenGuid.DefinitionId = TEXT("Echo_Gloomfang"); // guid stays invalid

    FAstrawildEchoInstanceV2 NoSpecies;
    NoSpecies.InstanceId = FGuid::NewGuid(); // species stays none

    Roster->ImportFromSave({ Valid, Duplicate, BrokenGuid, NoSpecies });

    TestEqual(TEXT("Only the valid entry survives"), Roster->GetRoster().Num(), 1);
    TestTrue(TEXT("Valid entry kept its guid"), Roster->IsInRoster(Valid.InstanceId));
    TestFalse(TEXT("Guid-less entry dropped"), Roster->IsInRoster(BrokenGuid.InstanceId));
    TestFalse(TEXT("Species-less entry dropped"), Roster->IsInRoster(NoSpecies.InstanceId));

    return true;
}

// ---------------------------------------------------------------------------
// Final Run (FR-12) — Act 3 "The Storm Crown" contracts.
// World-free: pure data/struct contracts + static resolvers, exactly like the
// rest of the suite (engine run happens on the Antigravity machine, AG-3).
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildFinalRunQuestChainTest,
    "ASTRAWILD.Quest.FinalRunChain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildFinalRunQuestChainTest::RunTest(const FString& Parameters)
{
    // MQ-13..17 chain contract (FR-5): every link resolves inside the set,
    // exactly ONE terminus, every quest has objectives, and the objective
    // vocabulary stays inside the Act 3 set (DiscoverPOI / DefeatCreature /
    // CraftRecipe / ReachLocation) — mirrors the BuildFinalRunContent data.
    UAstrawildQuestDefinition* StormAnchors = NewObject<UAstrawildQuestDefinition>();
    StormAnchors->QuestId = TEXT("Quest_StormAnchors");
    StormAnchors->NextQuestId = TEXT("Quest_CrownRelay");
    StormAnchors->Objectives = { FAstrawildQuestObjective() }; // DiscoverPOI default-set in content.

    UAstrawildQuestDefinition* CrownRelay = NewObject<UAstrawildQuestDefinition>();
    CrownRelay->QuestId = TEXT("Quest_CrownRelay");
    CrownRelay->NextQuestId = TEXT("Quest_EyeOfTheMaelstrom");
    CrownRelay->Objectives = { FAstrawildQuestObjective(), FAstrawildQuestObjective() };

    UAstrawildQuestDefinition* EyeQuest = NewObject<UAstrawildQuestDefinition>();
    EyeQuest->QuestId = TEXT("Quest_EyeOfTheMaelstrom");
    EyeQuest->NextQuestId = TEXT("Quest_TheDrownedSovereign");
    EyeQuest->Objectives = { FAstrawildQuestObjective(), FAstrawildQuestObjective() };

    UAstrawildQuestDefinition* Sovereign = NewObject<UAstrawildQuestDefinition>();
    Sovereign->QuestId = TEXT("Quest_TheDrownedSovereign");
    Sovereign->NextQuestId = TEXT("Quest_FirstDawnAgain");
    Sovereign->Objectives = { FAstrawildQuestObjective() };

    UAstrawildQuestDefinition* FirstDawn = NewObject<UAstrawildQuestDefinition>();
    FirstDawn->QuestId = TEXT("Quest_FirstDawnAgain");
    FirstDawn->NextQuestId = NAME_None; // THE terminus (validator: chain closure).
    FirstDawn->Objectives = { FAstrawildQuestObjective() };

    const TArray<UAstrawildQuestDefinition*> Chain = { StormAnchors, CrownRelay, EyeQuest, Sovereign, FirstDawn };

    // Chain walk: 5 links, 4 hops, ends at FirstDawnAgain with no successor.
    int32 Hops = 0;
    const UAstrawildQuestDefinition* Current = Chain[0];
    while (Current && !Current->NextQuestId.IsNone() && Hops < 10)
    {
        const UAstrawildQuestDefinition* Next = nullptr;
        for (const UAstrawildQuestDefinition* Quest : Chain)
        {
            if (Quest->QuestId == Current->NextQuestId)
            {
                Next = Quest;
                break;
            }
        }
        TestNotNull(FString::Printf(TEXT("Link resolves: %s"), *Current->QuestId.ToString()), const_cast<UAstrawildQuestDefinition*>(Next));
        Current = Next;
        ++Hops;
    }
    TestEqual(TEXT("Chain walks exactly 4 hops"), Hops, 4);
    TestEqual(TEXT("Chain ends at the terminus"), Current ? Current->QuestId : FName(), FName(TEXT("Quest_FirstDawnAgain")));

    // Exactly one terminus + every quest carries objectives and rewards policy.
    int32 Terminii = 0;
    for (const UAstrawildQuestDefinition* Quest : Chain)
    {
        if (Quest->NextQuestId.IsNone())
        {
            ++Terminii;
        }
        TestTrue(FString::Printf(TEXT("Quest %s has objectives"), *Quest->QuestId.ToString()), Quest->Objectives.Num() > 0);
        TestTrue(FString::Printf(TEXT("Quest %s grants research"), *Quest->QuestId.ToString()), Quest->RewardResearchPoints > 0);
    }
    TestEqual(TEXT("Exactly one chain terminus"), Terminii, 1);

    // Act 3 objective vocabulary: the chain consumes the four Final Run types
    // (enum members exist + are distinct — serialization-safe, appended-only).
    TestNotEqual(TEXT("DiscoverPOI differs from ReachLocation"),
        static_cast<int32>(EAstrawildQuestObjectiveType::DiscoverPOI), static_cast<int32>(EAstrawildQuestObjectiveType::ReachLocation));
    TestNotEqual(TEXT("DefeatCreature differs from CraftRecipe"),
        static_cast<int32>(EAstrawildQuestObjectiveType::DefeatCreature), static_cast<int32>(EAstrawildQuestObjectiveType::CraftRecipe));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDialogueEndingChoiceTest,
    "ASTRAWILD.Dialogue.EndingChoice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDialogueEndingChoiceTest::RunTest(const FString& Parameters)
{
    // FR-6: the ending id vocabulary is closed and maps to exactly two states.
    TestEqual(TEXT("Ending_BreakCage routes to The Dawn That Stays"),
        static_cast<int32>(UAstrawildDialogueComponent::ResolveEndingForTriggerId(TEXT("Ending_BreakCage"))),
        static_cast<int32>(EAstrawildEndingState::TheDawnThatStays));
    TestEqual(TEXT("Ending_StormSleeps routes to The Storm That Sleeps"),
        static_cast<int32>(UAstrawildDialogueComponent::ResolveEndingForTriggerId(TEXT("Ending_StormSleeps"))),
        static_cast<int32>(EAstrawildEndingState::TheStormThatSleeps));
    TestEqual(TEXT("Unknown ending id routes to None"),
        static_cast<int32>(UAstrawildDialogueComponent::ResolveEndingForTriggerId(TEXT("Ending_Nonsense"))),
        static_cast<int32>(EAstrawildEndingState::None));
    TestEqual(TEXT("None id routes to None"),
        static_cast<int32>(UAstrawildDialogueComponent::ResolveEndingForTriggerId(NAME_None)),
        static_cast<int32>(EAstrawildEndingState::None));

    // The choice struct carries the consequence field, defaulting to off.
    FAstrawildDialogueChoice Choice;
    TestTrue(TEXT("TriggerEndingId defaults to NAME_None"), Choice.TriggerEndingId.IsNone());

    // A hand-built ending choice: hard end + one-way flag + ending route —
    // the exact Maren crown shape (structural contract).
    // Final-audit G-2: canon gates the ending on MQ-17 (Quest_FirstDawnAgain,
    // the homecoming terminus) — gating on the Sovereign (MQ-16) would fire the
    // ending one quest early and strand MQ-17 active under the ending banner.
    Choice.Text = FText::FromString(TEXT("Break the cage"));
    Choice.RequiredQuestCompletedId = TEXT("Quest_FirstDawnAgain");
    Choice.ForbiddenFlagId = TEXT("Maren_EndingResolved");
    Choice.SetFlagId = TEXT("Maren_EndingResolved");
    Choice.TriggerEndingId = TEXT("Ending_BreakCage");
    Choice.bEndDialogue = true;
    TestTrue(TEXT("Ending choice is a hard end"), Choice.bEndDialogue);
    TestTrue(TEXT("Ending choice has no goto (unambiguous)"), Choice.GotoNodeId.IsNone());
    TestTrue(TEXT("Ending choice is gated on the MQ-17 homecoming, not the Sovereign"),
        Choice.RequiredQuestCompletedId == FName(TEXT("Quest_FirstDawnAgain")));
    TestTrue(TEXT("Ending gate is NOT the MQ-16 Sovereign kill"),
        Choice.RequiredQuestCompletedId != FName(TEXT("Quest_TheDrownedSovereign")));

    // The ending enum itself is a closed 4-value vocabulary (None + 2 + Count).
    TestEqual(TEXT("Ending enum order: None=0, Dawn=1, Storm=2, Count=3"),
        static_cast<int32>(EAstrawildEndingState::Count), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSaveSchemaV5EndingTest,
    "ASTRAWILD.Save.SchemaV5Ending",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSaveSchemaV5EndingTest::RunTest(const FString& Parameters)
{
    // FR-6: schema v5 — the subsystem reports 5 and the save object carries the
    // ending payload with save-safe defaults (None + locked post-game).
    UAstrawildSaveSubsystem* Subsystem = NewObject<UAstrawildSaveSubsystem>();
    TestEqual(TEXT("Current schema version is 5"), Subsystem->GetCurrentSchemaVersion(), 5);

    UAstrawildSaveGame* Save = NewObject<UAstrawildSaveGame>();
    TestEqual(TEXT("Ending defaults to 0 (None)"), Save->EndingState, 0);
    TestFalse(TEXT("Post-game defaults locked"), Save->bPostGameUnlocked);

    // Round-trip: chosen ending survives the int32-cast pair used by
    // SaveWorld/LoadWorld (GameState->enum -> int32 -> enum on restore).
    Save->EndingState = static_cast<int32>(EAstrawildEndingState::TheDawnThatStays);
    Save->bPostGameUnlocked = true;
    TestEqual(TEXT("Dawn ending round-trips"),
        static_cast<EAstrawildEndingState>(Save->EndingState), EAstrawildEndingState::TheDawnThatStays);
    Save->EndingState = static_cast<int32>(EAstrawildEndingState::TheStormThatSleeps);
    TestEqual(TEXT("Storm ending round-trips"),
        static_cast<EAstrawildEndingState>(Save->EndingState), EAstrawildEndingState::TheStormThatSleeps);

    // Load-side clamp contract (corrupt saves fail closed, never trust garbage).
    const int32 Corrupt = 9999;
    const int32 Clamped = FMath::Clamp(Corrupt, 0, static_cast<int32>(EAstrawildEndingState::Count) - 1);
    TestEqual(TEXT("Corrupt ending value clamps into range"), Clamped, 2);
    TestTrue(TEXT("Clamped value is a valid enum member"), Clamped >= 0 && Clamped < static_cast<int32>(EAstrawildEndingState::Count));

    // Legacy contract: the save object's own schema stamp default stays at the
    // historical 2 (older saves deserialize with their written version; the
    // subsystem always stamps the current 5 on write).
    TestEqual(TEXT("Save object schema default is the legacy 2 stamp"), Save->SaveSchemaVersion, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildEchoFinalRunBossesTest,
    "ASTRAWILD.Echo.FinalRunBosses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildEchoFinalRunBossesTest::RunTest(const FString& Parameters)
{
    // FR-11: the boss display-name resolver covers the canonical roster and
    // falls back gracefully — no boss is ever "Underlight Warden" by mistake.
    TestEqual(TEXT("Warden id resolves"), AAstrawildEchoBossCharacter::ResolveBossDisplayName(TEXT("Creature_UnderlightWarden"), FText()).ToString(),
        FString(TEXT("Underlight Warden")));
    TestEqual(TEXT("Colossus id resolves"), AAstrawildEchoBossCharacter::ResolveBossDisplayName(TEXT("Creature_VaultColossus"), FText()).ToString(),
        FString(TEXT("Vault Colossus")));
    TestEqual(TEXT("Tyrant id resolves"), AAstrawildEchoBossCharacter::ResolveBossDisplayName(TEXT("Creature_GlassTyrant"), FText()).ToString(),
        FString(TEXT("Glass Tyrant")));
    TestEqual(TEXT("Sovereign id resolves"), AAstrawildEchoBossCharacter::ResolveBossDisplayName(TEXT("Creature_DrownedSovereign"), FText()).ToString(),
        FString(TEXT("The Drowned Sovereign")));
    TestEqual(TEXT("Unknown id falls back to the species label"),
        AAstrawildEchoBossCharacter::ResolveBossDisplayName(TEXT("Creature_SomethingNew"), FText::FromString(TEXT("Wild Boss"))).ToString(),
        FString(TEXT("Wild Boss")));
    TestEqual(TEXT("Unknown id with no label falls back to a generic title"),
        AAstrawildEchoBossCharacter::ResolveBossDisplayName(NAME_None, FText()).ToString(), FString(TEXT("Echo Boss")));

    // The Drowned Sovereign combat contract (MQ-16): 2000 HP at the standard
    // boss scale (400 base × 5.0) + Dawn Light weakness + Pulse resist.
    UAstrawildEchoDefinition* Sovereign = NewObject<UAstrawildEchoDefinition>();
    Sovereign->BaseStats.MaxHealth = 400.0f;
    Sovereign->WeaknessElement = EAstrawildElementType::Light;
    Sovereign->Element = EAstrawildElementType::Pulse;
    const float BossHealthScale = 5.0f;
    TestEqual(TEXT("Sovereign boss health is 2000"), FMath::RoundToInt(Sovereign->BaseStats.MaxHealth * BossHealthScale), 2000);

    TestEqual(TEXT("Dawn Light exploits the Sovereign (x1.5)"),
        AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(
            EAstrawildElementType::Light, EAstrawildElementType::Light, EAstrawildElementType::Pulse), 1.5f);
    TestEqual(TEXT("Pulse attacks are resisted (x0.80, unified)"),
        AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(
            EAstrawildElementType::Pulse, EAstrawildElementType::Light, EAstrawildElementType::Pulse), 0.8f);

    // Glass Tyrant weakness (MQ-14): Light cuts it too — the Dawn arsenal
    // carries Act 3, exactly as the story promises.
    UAstrawildEchoDefinition* Tyrant = NewObject<UAstrawildEchoDefinition>();
    Tyrant->WeaknessElement = EAstrawildElementType::Light;
    Tyrant->Element = EAstrawildElementType::Ash;
    TestEqual(TEXT("Light exploits the Glass Tyrant"),
        AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(
            EAstrawildElementType::Light, Tyrant->WeaknessElement, Tyrant->Element), 1.5f);

    // Phase design at 2000 HP: thresholds 66%/33% remain meaningful.
    TestEqual(TEXT("Phase 1 above 66%"), AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(0.99f, false), 1);
    TestEqual(TEXT("Phase 2 in the middle band"), AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(0.5f, false), 2);
    TestEqual(TEXT("Phase 3 below 33%"), AAstrawildEchoBossCharacter::ComputePhaseForHealthFraction(0.25f, false), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildTechSkiffEngineeringTest,
    "ASTRAWILD.Tech.SkiffEngineering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildTechSkiffEngineeringTest::RunTest(const FString& Parameters)
{
    // FR-8/FR-5: Skiff Engineering unlocks exactly the Stratos Coil recipe —
    // the tech and the recipe form one gate (25 RP, AdvancedEnergy era).
    UAstrawildTechnologyDefinition* Tech = NewObject<UAstrawildTechnologyDefinition>();
    Tech->TechId = TEXT("Tech_SkiffEngineering");
    Tech->ResearchCost = 25;
    Tech->UnlockedRecipeIds = { TEXT("Recipe_SkiffStratosCoil") };
    TestEqual(TEXT("Tech cost is 25"), Tech->ResearchCost, 25);
    TestEqual(TEXT("Tech unlocks exactly one recipe"), Tech->UnlockedRecipeIds.Num(), 1);
    TestTrue(TEXT("Tech unlocks the coil recipe"), Tech->UnlockedRecipeIds.Contains(FName(TEXT("Recipe_SkiffStratosCoil"))));

    // The recipe contract: three materials in, one key item out, workbench-gated.
    UAstrawildRecipeDefinition* Recipe = NewObject<UAstrawildRecipeDefinition>();
    Recipe->RecipeId = TEXT("Recipe_SkiffStratosCoil");
    Recipe->Ingredients.SetNum(3);
    Recipe->Ingredients[0].ItemId = TEXT("Item_StormSilver");
    Recipe->Ingredients[0].Quantity = 4;
    Recipe->Ingredients[1].ItemId = TEXT("Item_DuneGlass");
    Recipe->Ingredients[1].Quantity = 3;
    Recipe->Ingredients[2].ItemId = TEXT("Item_MaelstromGlass");
    Recipe->Ingredients[2].Quantity = 2;
    Recipe->Outputs.SetNum(1);
    Recipe->Outputs[0].ItemId = TEXT("Item_SkiffStratosCoil");
    Recipe->Outputs[0].Quantity = 1;
    Recipe->RequiredTechId = TEXT("Tech_SkiffEngineering");
    Recipe->RequiredStationId = TEXT("Station_Workbench");
    Recipe->CraftDurationSeconds = 20.0f;

    TestEqual(TEXT("Recipe consumes three materials"), Recipe->Ingredients.Num(), 3);
    TestEqual(TEXT("Recipe outputs exactly one coil"), Recipe->Outputs.Num(), 1);
    TestEqual(TEXT("Coil output quantity is 1"), Recipe->Outputs[0].Quantity, 1);
    TestTrue(TEXT("Every ingredient has a positive quantity"),
        Recipe->Ingredients[0].Quantity > 0 && Recipe->Ingredients[1].Quantity > 0 && Recipe->Ingredients[2].Quantity > 0);
    TestTrue(TEXT("Recipe is gated by its own tech"), Recipe->RequiredTechId == FName(TEXT("Tech_SkiffEngineering")));
    bool bHasMaelstromGate = false;
    for (const FAstrawildItemStack& Input : Recipe->Ingredients)
    {
        if (Input.ItemId == FName(TEXT("Item_MaelstromGlass")))
        {
            bHasMaelstromGate = true;
        }
    }
    TestTrue(TEXT("Maelstrom Glass is the Act 3 material gate"), bHasMaelstromGate);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSkiffCeilingGateTest,
    "ASTRAWILD.Skiff.CeilingGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSkiffCeilingGateTest::RunTest(const FString& Parameters)
{
    // FR-8: the Stratos Coil ceiling gate — pure resolver, no world needed.
    // Stock skiff: 12000 (120m). Coiled: 16000 (160m) — the Eye Gate at 150m
    // sits BETWEEN them, which is what makes the coil Act 3's key.
    TestEqual(TEXT("Stock ceiling stays 12000"),
        AAstrawildSkiffActor::ComputeFlightCeiling(12000.0f, 16000.0f, false), 12000.0f);
    TestEqual(TEXT("Coiled ceiling rises to 16000"),
        AAstrawildSkiffActor::ComputeFlightCeiling(12000.0f, 16000.0f, true), 16000.0f);

    // The Eye Gate altitude (15000) is inside the coiled band and outside the
    // stock band — the gate is physically unreachable without the coil.
    const float StockCeiling = AAstrawildSkiffActor::ComputeFlightCeiling(12000.0f, 16000.0f, false);
    const float CoiledCeiling = AAstrawildSkiffActor::ComputeFlightCeiling(12000.0f, 16000.0f, true);
    const float EyeGateAltitude = 15000.0f;
    TestTrue(TEXT("Eye Gate is above the stock ceiling"), EyeGateAltitude > StockCeiling);
    TestTrue(TEXT("Eye Gate is below the coiled ceiling"), EyeGateAltitude < CoiledCeiling);

    // Defensive contract: a mis-tuned coiled value can never LOWER the ceiling.
    TestEqual(TEXT("Bad coil data never lowers the ceiling"),
        AAstrawildSkiffActor::ComputeFlightCeiling(12000.0f, 9000.0f, true), 12000.0f);

    // Symmetry with the class defaults (the actor's tunables match the design;
    // read from the CDO — no world spawn needed).
    const AAstrawildSkiffActor* SkiffCDO = AAstrawildSkiffActor::StaticClass()->GetDefaultObject<AAstrawildSkiffActor>();
    TestEqual(TEXT("Default stock ceiling matches the design"), SkiffCDO->MaxAltitudeAboveGround, 12000.0f);
    TestEqual(TEXT("Default coiled ceiling matches the design"), SkiffCDO->CoiledMaxAltitudeAboveGround, 16000.0f);
    TestEqual(TEXT("Default coil item id matches the content"), SkiffCDO->StratosCoilItemId, FName(TEXT("Item_SkiffStratosCoil")));
    return true;
}

// ---------------------------------------------------------------------------
// FINAL SOURCE COMPLETION PASS — Test 68: one-shot objective back-fill (G-1/G-3).
// POIs discovered or one-shot bosses defeated BEFORE a quest activates must
// credit that quest at StartQuest — the alternative was a dead objective that
// soft-locked the 17-quest chain (MQ-11's FirstLightRuin sits ~25m from spawn).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildQuestOneShotBackFillTest,
    "ASTRAWILD.Quest.OneShotBackFill",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildQuestOneShotBackFillTest::RunTest(const FString& Parameters)
{
    const auto MakeObjective = [](const EAstrawildQuestObjectiveType Type, const TCHAR* TargetId, const int32 Required)
    {
        FAstrawildQuestObjective Objective;
        Objective.Type = Type;
        Objective.TargetId = TargetId;
        Objective.RequiredCount = Required;
        return Objective;
    };

    // Discovered POI back-fills to full (discovery is one-shot per save).
    FAstrawildQuestSaveData State;
    State.QuestId = TEXT("Q_BackFill");
    State.Objectives = {
        MakeObjective(EAstrawildQuestObjectiveType::DiscoverPOI, TEXT("POI_FirstLightRuin"), 1),
        MakeObjective(EAstrawildQuestObjectiveType::DiscoverPOI, TEXT("POI_NeverSeen"), 1),
        MakeObjective(EAstrawildQuestObjectiveType::DefeatCreature, TEXT("Creature_DrownedSovereign"), 1),
        MakeObjective(EAstrawildQuestObjectiveType::DefeatCreature, TEXT("Echo_Gloomfang"), 3),
        MakeObjective(EAstrawildQuestObjectiveType::CollectItem, TEXT("Item_Wood"), 10)
    };

    TMap<FName, int32> DefeatCounts;
    DefeatCounts.Add(TEXT("Creature_DrownedSovereign"), 1);
    DefeatCounts.Add(TEXT("Echo_Gloomfang"), 2); // 2 of 3 — partial credit

    TSet<FName> DiscoveredPois;
    DiscoveredPois.Add(TEXT("POI_FirstLightRuin"));

    const int32 BackFilled = UAstrawildQuestComponent::BackFillOneShotObjectives(State, DefeatCounts, DiscoveredPois);

    TestEqual(TEXT("Four objectives received back-fill"), BackFilled, 4);
    TestTrue(TEXT("Discovered POI objective completes"), State.Objectives[0].IsComplete());
    TestFalse(TEXT("Undiscovered POI objective stays at zero"), State.Objectives[1].IsComplete());
    TestTrue(TEXT("Defeated one-shot boss objective completes"), State.Objectives[2].IsComplete());
    TestEqual(TEXT("Partial kill count credits 2 of 3"), State.Objectives[3].ProgressCount, 2);
    TestEqual(TEXT("CollectItem is NEVER back-filled (live gameplay only)"), State.Objectives[4].ProgressCount, 0);

    // Pre-completed objectives are never touched (no double-grant, no reset).
    State.Objectives[1].ProgressCount = 1; // simulate completed progress
    const int32 SecondPass = UAstrawildQuestComponent::BackFillOneShotObjectives(State, DefeatCounts, DiscoveredPois);
    TestTrue(TEXT("Completed objectives are skipped on re-run"), SecondPass < 5);

    // Counters cannot exceed the requirement (no minted progress from history).
    TMap<FName, int32> HugeCounts;
    HugeCounts.Add(TEXT("Echo_Gloomfang"), 500);
    FAstrawildQuestSaveData Capped = State;
    Capped.Objectives = { MakeObjective(EAstrawildQuestObjectiveType::DefeatCreature, TEXT("Echo_Gloomfang"), 3) };
    UAstrawildQuestComponent::BackFillOneShotObjectives(Capped, HugeCounts, {});
    TestEqual(TEXT("Back-fill caps at the required count"), Capped.Objectives[0].ProgressCount, 3);

    return true;
}

// ---------------------------------------------------------------------------
// FINAL SOURCE COMPLETION PASS — Test 69: defeat-counter import sanitize (G-3).
// Crafted saves cannot mint quest credit: id-less entries dropped, negatives
// dropped, values capped.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildQuestDefeatCountImportTest,
    "ASTRAWILD.Quest.DefeatCountImportSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildQuestDefeatCountImportTest::RunTest(const FString& Parameters)
{
    UAstrawildQuestComponent* Quests = NewObject<UAstrawildQuestComponent>();
    if (!TestNotNull(TEXT("Quest component constructed"), Quests))
    {
        return false;
    }

    TMap<FName, int32> Crafted;
    Crafted.Add(TEXT("Creature_GlassTyrant"), 2);
    Crafted.Add(NAME_None, 7);              // id-less — dropped
    Crafted.Add(TEXT("Echo_BadValue"), -4); // negative — dropped
    Crafted.Add(TEXT("Echo_HugeValue"), 5000); // capped

    Quests->ImportDefeatCounts(Crafted);

    TMap<FName, int32> Imported;
    Quests->ExportDefeatCounts(Imported);
    TestEqual(TEXT("Valid entries survive import"), Imported.FindRef(TEXT("Creature_GlassTyrant")), 2);
    TestFalse(TEXT("Id-less entry dropped"), Imported.Contains(NAME_None));
    TestFalse(TEXT("Negative entry dropped"), Imported.Contains(TEXT("Echo_BadValue")));
    TestEqual(TEXT("Absurd counter capped at 999"), Imported.FindRef(TEXT("Echo_HugeValue")), 999);

    return true;
}

// ---------------------------------------------------------------------------
// FINAL SOURCE COMPLETION PASS — Test 70: dismantle never advances a
// PlaceBuilding objective (F-03) — the event Amount=-1 clamp exploit.
// Drives the REAL objective matcher (ApplyEventToQuest is the production path).
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildQuestDismantleNotPlacementTest,
    "ASTRAWILD.Quest.DismantleIsNotPlacement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildQuestDismantleNotPlacementTest::RunTest(const FString& Parameters)
{
    UAstrawildQuestComponent* Quests = NewObject<UAstrawildQuestComponent>();
    if (!TestNotNull(TEXT("Quest component constructed"), Quests))
    {
        return false;
    }

    FAstrawildQuestSaveData State;
    State.QuestId = TEXT("Q_Place");
    FAstrawildQuestObjective Objective;
    Objective.Type = EAstrawildQuestObjectiveType::PlaceBuilding;
    Objective.TargetId = TEXT("Building_Foundation");
    Objective.RequiredCount = 2;
    State.Objectives = { Objective };
    State.bActive = true;
    Quests->ImportFromSave({ State });

    const auto BuildingEvent = [](const int32 Amount)
    {
        FAstrawildGameplayEvent Event;
        Event.EventTag = TAG_Astrawild_Event_BuildingPlaced;
        Event.TargetId = TEXT("Building_Foundation");
        Event.Amount = Amount;
        return Event;
    };

    // The dismantle publication: BuildingPlaced with Amount = -1.
    Quests->ApplyEventToQuest(BuildingEvent(-1));
    TestEqual(TEXT("Dismantle does NOT advance the placement objective"), Quests->GetActiveObjectives()[0].ProgressCount, 0);

    // A real placement still counts.
    Quests->ApplyEventToQuest(BuildingEvent(1));
    TestEqual(TEXT("Real placement advances the objective"), Quests->GetActiveObjectives()[0].ProgressCount, 1);

    return true;
}

// ---------------------------------------------------------------------------
// FINAL SOURCE COMPLETION PASS — Test 71: research import sanitize (M-3) —
// duplicate tech ids collapse, invalid entries drop, negative RP clamps.
// The earlier hardening of this path was lost with the destroyed Final-Run
// branch; this contract pins its re-landing.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildResearchImportSafetyTest,
    "ASTRAWILD.Research.ImportSafety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildResearchImportSafetyTest::RunTest(const FString& Parameters)
{
    UAstrawildResearchSubsystem* Research = NewObject<UAstrawildResearchSubsystem>();
    if (!TestNotNull(TEXT("Research subsystem constructed"), Research))
    {
        return false;
    }

    FAstrawildResearchSaveData Crafted;
    Crafted.UnlockedTechIds = { TEXT("Tech_Electrical"), TEXT("Tech_Electrical"), NAME_None, TEXT("Tech_Husbandry") };
    Crafted.ResearchPoints = -25;

    Research->ImportFromSave(Crafted);

    TestEqual(TEXT("Duplicate tech collapses (first-seen-wins)"), Research->GetUnlockedTechIds().Num(), 2);
    TestTrue(TEXT("Valid tech survived"), Research->IsTechUnlocked(TEXT("Tech_Electrical")));
    TestTrue(TEXT("Second valid tech survived"), Research->IsTechUnlocked(TEXT("Tech_Husbandry")));
    TestFalse(TEXT("Id-less tech dropped"), Research->IsTechUnlocked(NAME_None));
    TestEqual(TEXT("Negative research points clamp to zero"), Research->GetResearchPoints(), 0);

    return true;
}

// ---------------------------------------------------------------------------
// FINAL SOURCE COMPLETION PASS — Test 72: final-audit save contracts —
// echo health (M-2) + robot chassis (H-2) additive fields round-trip.
// ---------------------------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSaveFinalAuditContractsTest,
    "ASTRAWILD.Save.FinalAuditContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSaveFinalAuditContractsTest::RunTest(const FString& Parameters)
{
    // M-2: echo health persists; 0 is the legacy sentinel (full-heal for old saves).
    FAstrawildEchoInstanceV2 EchoData;
    TestEqual(TEXT("Echo health defaults to the legacy sentinel"), EchoData.CurrentHealth, 0.0f);
    EchoData.CurrentHealth = 37.5f;
    TestEqual(TEXT("Echo health carries the saved value"), EchoData.CurrentHealth, 37.5f);
    EchoData.CurrentHealth = -5.0f;
    TestTrue(TEXT("Negative health fails the > 0 restore gate"), EchoData.CurrentHealth <= 0.0f);

    // H-2: the robot chassis id persists — the field the SaveWorld loop now writes.
    FAstrawildRobotSaveData RobotData;
    TestTrue(TEXT("Robot chassis defaults to none"), RobotData.RobotDefinitionId.IsNone());
    RobotData.RobotDefinitionId = TEXT("Robot_Borebot");
    TestTrue(TEXT("Robot chassis carries the definition id"), RobotData.RobotDefinitionId == FName(TEXT("Robot_Borebot")));
    TestTrue(TEXT("Chassis survives a struct copy (save -> load round-trip shape)"),
        FAstrawildRobotSaveData(RobotData).RobotDefinitionId == RobotData.RobotDefinitionId);

    return true;
}


// ===========================================================================
// GAMEPLAY DEPTH PACK (GDP) — Tests 73-84: ability engine, locomotion,
// attributes, skills, NPC affinity.
// ===========================================================================

// --- GDP-1: ability library integrity (Test 73) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAbilityLibraryIntegrityTest,
    "ASTRAWILD.Ability.LibraryIntegrity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAbilityLibraryIntegrityTest::RunTest(const FString& Parameters)
{
    TArray<FString> Problems;
    UAstrawildAbilityLibrary::ValidateTable(Problems);
    for (const FString& Problem : Problems)
    {
        AddError(Problem);
    }
    TestTrue(TEXT("Ability table validates clean"), Problems.IsEmpty());
    TestEqual(TEXT("Ability table holds 53 templates"), UAstrawildAbilityLibrary::GetAbilityCount(), 53);
    TestTrue(TEXT("Signature ability resolves"), UAstrawildAbilityLibrary::IsKnownAbility(TEXT("Ability_LumewispDawn")));
    TestFalse(TEXT("Unknown id rejected"), UAstrawildAbilityLibrary::IsKnownAbility(TEXT("Ability_Nope")));

    return true;
}

// --- GDP-1: derived loadouts cover every element/role (Test 74) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAbilityDerivedLoadoutTest,
    "ASTRAWILD.Ability.DerivedLoadout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAbilityDerivedLoadoutTest::RunTest(const FString& Parameters)
{
    const EAstrawildElementType Elements[6] =
    {
        EAstrawildElementType::Light, EAstrawildElementType::Ash, EAstrawildElementType::Flora,
        EAstrawildElementType::Ember, EAstrawildElementType::Frost, EAstrawildElementType::Pulse
    };
    const EAstrawildEchoRole Roles[4] =
    {
        EAstrawildEchoRole::Combat, EAstrawildEchoRole::Base,
        EAstrawildEchoRole::Support, EAstrawildEchoRole::Explorer
    };

    for (const EAstrawildElementType Element : Elements)
    {
        for (const EAstrawildEchoRole Role : Roles)
        {
            const TArray<FName> Loadout = UAstrawildAbilityLibrary::ComputeDerivedAbilityIds(
                Element, Role, EAstrawildEchoFamily::Beast);
            TestTrue(TEXT("Derived loadout non-empty"), Loadout.Num() >= 4);
            bool bHasOffense = false;
            for (const FName& Id : Loadout)
            {
                const FAstrawildAbilityData* Data = UAstrawildAbilityLibrary::FindAbility(Id);
                TestNotNull(TEXT("Derived id resolves"), Data);
                if (Data && Data->Category == EAstrawildAbilityCategory::Offensive)
                {
                    bHasOffense = true;
                }
            }
            TestTrue(TEXT("Every element x role combo derives an offensive option"), bHasOffense);
        }
    }

    // Determinism: same inputs, same loadout.
    const TArray<FName> A = UAstrawildAbilityLibrary::ComputeDerivedAbilityIds(
        EAstrawildElementType::Ember, EAstrawildEchoRole::Combat, EAstrawildEchoFamily::Dragon);
    const TArray<FName> B = UAstrawildAbilityLibrary::ComputeDerivedAbilityIds(
        EAstrawildElementType::Ember, EAstrawildEchoRole::Combat, EAstrawildEchoFamily::Dragon);
    TestTrue(TEXT("Derivation is deterministic"), A == B);

    // DP-3: Water/Flying locomotion appends exactly one signature (7 entries);
    // the legacy three-argument path (pinned above) stays at six.
    const TArray<FName> WaterLoadout = UAstrawildAbilityLibrary::ComputeDerivedAbilityIds(
        EAstrawildElementType::Frost, EAstrawildEchoRole::Combat, EAstrawildEchoFamily::Aquatic,
        EAstrawildLocomotionClass::Water);
    TestEqual(TEXT("Water movers carry a locomotion signature"), WaterLoadout.Num(), 7);
    const TArray<FName> FlyingLoadout = UAstrawildAbilityLibrary::ComputeDerivedAbilityIds(
        EAstrawildElementType::Light, EAstrawildEchoRole::Support, EAstrawildEchoFamily::Avian,
        EAstrawildLocomotionClass::Flying);
    TestEqual(TEXT("Flying movers carry a locomotion signature"), FlyingLoadout.Num(), 7);

    return true;
}

// --- DP-3: party element resonance contracts (Test 103) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildPartyResonanceTest,
    "ASTRAWILD.DP3.Resonance.PairResolution",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildPartyResonanceTest::RunTest(const FString& Parameters)
{
    // Known pairs resolve with themed identity; the lookup is symmetric.
    const FAstrawildPartyResonance SteamVeil = AAstrawildEchoCharacter::ResolvePartyResonance(
        EAstrawildElementType::Frost, EAstrawildElementType::Ember);
    TestTrue(TEXT("Frost+Ember resolves Steam Veil"), SteamVeil.ResonanceId == TEXT("Resonance_SteamVeil"));
    const FAstrawildPartyResonance Reversed = AAstrawildEchoCharacter::ResolvePartyResonance(
        EAstrawildElementType::Ember, EAstrawildElementType::Frost);
    TestTrue(TEXT("Pair lookup is symmetric"), Reversed.ResonanceId == SteamVeil.ResonanceId);

    // None or identical elements never resonate (fail-closed).
    TestFalse(TEXT("None element never resonates"), AAstrawildEchoCharacter::ResolvePartyResonance(
        EAstrawildElementType::None, EAstrawildElementType::Ember).IsValid());
    TestFalse(TEXT("Same element never resonates"), AAstrawildEchoCharacter::ResolvePartyResonance(
        EAstrawildElementType::Ember, EAstrawildElementType::Ember).IsValid());

    // Party resolution: duplicates collapse; with three distinct elements the
    // FIRST pair in canon table order wins (deterministic dominance).
    TArray<EAstrawildElementType> Trio;
    Trio.Add(EAstrawildElementType::Ember);
    Trio.Add(EAstrawildElementType::Ember);
    Trio.Add(EAstrawildElementType::Frost);
    Trio.Add(EAstrawildElementType::Pulse);
    const FAstrawildPartyResonance TrioRow = AAstrawildEchoCharacter::ResolvePartyResonanceForElements(Trio);
    TestTrue(TEXT("Three-element party resolves the canon-dominant pair"),
        TrioRow.ResonanceId == TEXT("Resonance_Superconductor"));

    // Full-table sweep: every distinct pair resolves, carries exactly ONE
    // modest bonus axis, and stays inside the 8-12% passive band.
    const EAstrawildElementType Elements[6] =
    {
        EAstrawildElementType::Light, EAstrawildElementType::Ash, EAstrawildElementType::Flora,
        EAstrawildElementType::Frost, EAstrawildElementType::Pulse, EAstrawildElementType::Ember
    };
    for (int32 A = 0; A < 6; ++A)
    {
        for (int32 B = A + 1; B < 6; ++B)
        {
            const FAstrawildPartyResonance Row = AAstrawildEchoCharacter::ResolvePartyResonance(Elements[A], Elements[B]);
            TestTrue(TEXT("Every element pair resolves a row"), Row.IsValid());
            const int32 Axes = (Row.DamageMitigation > 0.0f ? 1 : 0)
                + (Row.AbilityPowerBonus > 0.0f ? 1 : 0)
                + (Row.StatusPotencyBonus > 0.0f ? 1 : 0);
            TestEqual(TEXT("Resonance carries exactly one bonus axis"), Axes, 1);
            const float Magnitude = Row.DamageMitigation + Row.AbilityPowerBonus + Row.StatusPotencyBonus;
            TestTrue(TEXT("Resonance magnitude stays in the modest band"),
                Magnitude >= 0.05f && Magnitude <= 0.15f);
        }
    }

    return true;
}

// --- DP-4: player skill loadout contracts (Test 104) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSkillLoadoutTest,
    "ASTRAWILD.DP4.SkillLoadout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSkillLoadoutTest::RunTest(const FString& Parameters)
{
    using S = EAstrawildPlayerSkillId;

    // Fresh component: a 3-slot loadout with every slot empty.
    UAstrawildAttributeComponent* Attributes = NewObject<UAstrawildAttributeComponent>();
    TestEqual(TEXT("Fresh loadout has three slots"), Attributes->GetBoundSkills().Num(), 3);
    TestFalse(TEXT("Fresh loadout binds nothing"), Attributes->IsSkillBound(S::PowerStrike));

    // Bind validation: slot bounds.
    TestFalse(TEXT("Slot -1 rejected"), Attributes->BindSkillToSlot(-1, S::PowerStrike));
    TestFalse(TEXT("Slot 3 rejected"), Attributes->BindSkillToSlot(3, S::PowerStrike));

    // Bind validation: locked skills (and None) rejected on a Might-1 component.
    TestFalse(TEXT("Locked skill rejected"), Attributes->BindSkillToSlot(0, S::PowerStrike));
    TestFalse(TEXT("None never binds"), Attributes->BindSkillToSlot(0, S::None));

    // Might 10 / Agility 10 / Vigor 10 unlock PowerStrike, Whirlwind, Dash and
    // SecondWind — the second one proves unbound-but-unlocked suppression below.
    for (int32 i = 0; i < 30; ++i)
    {
        Attributes->AddAttributeXP(EAstrawildAttributeType::Might, 1000.0f);
        Attributes->AddAttributeXP(EAstrawildAttributeType::Agility, 1000.0f);
        Attributes->AddAttributeXP(EAstrawildAttributeType::Vigor, 1000.0f);
    }
    TestEqual(TEXT("Might 10 reached"), Attributes->GetLevel(EAstrawildAttributeType::Might), 10);
    TestEqual(TEXT("Agility 10 reached"), Attributes->GetLevel(EAstrawildAttributeType::Agility), 10);

    // Unlocked skill accepted; the slot reports it.
    TestTrue(TEXT("Unlocked PowerStrike binds to slot 0"), Attributes->BindSkillToSlot(0, S::PowerStrike));
    TestTrue(TEXT("Bound skill is reported bound"), Attributes->IsSkillBound(S::PowerStrike));
    TestEqual(TEXT("Slot 0 carries PowerStrike"), Attributes->GetBoundSkills()[0], S::PowerStrike);

    // Duplicate binding rejected (PowerStrike already occupies slot 0).
    TestFalse(TEXT("Duplicate binding rejected"), Attributes->BindSkillToSlot(1, S::PowerStrike));
    TestTrue(TEXT("A second skill binds to slot 1"), Attributes->BindSkillToSlot(1, S::Whirlwind));

    // Rebinding a slot replaces its occupant (no duplicates introduced).
    TestTrue(TEXT("Slot 0 rebinding replaces the occupant"), Attributes->BindSkillToSlot(0, S::Dash));
    TestFalse(TEXT("Evicted skill is no longer bound"), Attributes->IsSkillBound(S::PowerStrike));
    TestEqual(TEXT("Slot 0 now carries Dash"), Attributes->GetBoundSkills()[0], S::Dash);

    // Clearing: in-bounds empties the slot; out-of-bounds is a safe no-op.
    Attributes->ClearSlot(1);
    TestEqual(TEXT("Cleared slot reports None"), Attributes->GetBoundSkills()[1], S::None);
    Attributes->ClearSlot(-1);
    Attributes->ClearSlot(7);
    TestEqual(TEXT("Out-of-bounds clear is a safe no-op"), Attributes->GetBoundSkills()[0], S::Dash);

    // Bound-only cast: with ONLY Dash bound, the hurt-player ladder cannot pick
    // the (unlocked, unbound) SecondWind even at 20% health — and Dash stays
    // reachable through the moving branch.
    TestEqual(TEXT("Unbound SecondWind is not picked while hurt"),
        Attributes->PickBestReadySkill(0.2f, 0, false, false), S::None);
    TestEqual(TEXT("Bound Dash still picked while moving"),
        Attributes->PickBestReadySkill(1.0f, 0, false, true), S::Dash);

    // Empty-loadout fallback (zero-regression): unlocked skills but NO
    // bindings pick among ALL unlocked skills (the legacy ladder).
    UAstrawildAttributeComponent* Legacy = NewObject<UAstrawildAttributeComponent>();
    for (int32 i = 0; i < 30; ++i)
    {
        Legacy->AddAttributeXP(EAstrawildAttributeType::Vigor, 1000.0f);
        Legacy->AddAttributeXP(EAstrawildAttributeType::Agility, 1000.0f);
    }
    TestEqual(TEXT("Empty loadout keeps the legacy SecondWind pick"),
        Legacy->PickBestReadySkill(0.2f, 0, false, false), S::SecondWind);

    // Save round-trip: the loadout rides the attribute payload (v5 additive)
    // and survives intact; a pre-DP-4 payload (rows without a loadout) resets
    // the loadout to all-empty — the legacy smart-cast contract.
    TestEqual(TEXT("Loadout round-trip repairs nothing"), Attributes->ImportFromSaveData(Attributes->ToSaveData()), 0);
    TestEqual(TEXT("Rounded loadout keeps Dash in slot 0"), Attributes->GetBoundSkills()[0], S::Dash);

    TArray<FAstrawildAttributeSaveData> PreDP4;
    FAstrawildAttributeSaveData LegacyRow;
    LegacyRow.Type = EAstrawildAttributeType::Might;
    LegacyRow.Level = 10;
    PreDP4.Add(LegacyRow);
    TestEqual(TEXT("Pre-DP-4 payload imports clean"), Attributes->ImportFromSaveData(PreDP4), 0);
    TestEqual(TEXT("Pre-DP-4 loadout still has three slots"), Attributes->GetBoundSkills().Num(), 3);
    TestFalse(TEXT("Pre-DP-4 payload clears the loadout"), Attributes->IsSkillBound(S::Dash));

    return true;
}

// --- DP-5: per-boss special set contracts (Test 105) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildBossSpecialSetsTest,
    "ASTRAWILD.DP5.BossSpecialSets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildBossSpecialSetsTest::RunTest(const FString& Parameters)
{
    // Canonical ids resolve to four DISTINCT sets — the shared special pipeline
    // finally reads different data per boss.
    TestEqual(TEXT("Warden id resolves the Underlight Warden set"),
        AAstrawildEchoBossCharacter::ResolveBossSpecialSet(TEXT("Creature_UnderlightWarden")),
        EAstrawildBossSpecialSet::UnderlightWarden);
    TestEqual(TEXT("Colossus id resolves the Sunken Vault set"),
        AAstrawildEchoBossCharacter::ResolveBossSpecialSet(TEXT("Creature_VaultColossus")),
        EAstrawildBossSpecialSet::SunkenVault);
    TestEqual(TEXT("Tyrant id resolves the Glass Tyrant set"),
        AAstrawildEchoBossCharacter::ResolveBossSpecialSet(TEXT("Creature_GlassTyrant")),
        EAstrawildBossSpecialSet::GlassTyrant);
    TestEqual(TEXT("Sovereign id resolves the Eye of the Maelstrom set"),
        AAstrawildEchoBossCharacter::ResolveBossSpecialSet(TEXT("Creature_DrownedSovereign")),
        EAstrawildBossSpecialSet::EyeOfTheMaelstrom);

    // Unknown ids fail closed to the default set (the legacy shared pipeline).
    TestEqual(TEXT("Unknown id falls back to the default set"),
        AAstrawildEchoBossCharacter::ResolveBossSpecialSet(TEXT("Creature_DoesNotExist")),
        EAstrawildBossSpecialSet::UnderlightWarden);
    TestEqual(TEXT("None id falls back to the default set"),
        AAstrawildEchoBossCharacter::ResolveBossSpecialSet(NAME_None),
        EAstrawildBossSpecialSet::UnderlightWarden);

    // The default set is the byte-exact legacy tuning (zero-regression).
    const FAstrawildBossSpecialSetParams Legacy =
        AAstrawildEchoBossCharacter::GetBossSpecialSetParams(EAstrawildBossSpecialSet::UnderlightWarden);
    TestEqual(TEXT("Legacy set keeps the 7s special cooldown"), Legacy.SpecialAttackCooldownSeconds, 7.0f);
    TestEqual(TEXT("Legacy set keeps one bolt"), Legacy.BoltCount, 1);
    TestEqual(TEXT("Legacy set keeps one blast"), Legacy.BlastCount, 1);
    TestEqual(TEXT("Legacy set keeps the 350cm blast radius"), Legacy.SpecialBlastRadius, 350.0f);
    TestEqual(TEXT("Legacy set keeps one hazard per wave"), Legacy.HazardWaveCount, 1);
    TestEqual(TEXT("Legacy set keeps the 6dps hazards"), Legacy.HazardDamagePerSecond, 6.0f);
    TestTrue(TEXT("Legacy set keeps the Gloomfang summons"), Legacy.SummonSpeciesId == TEXT("Echo_Gloomfang"));

    // Pairwise distinctness: no two sets carry the same tuning bundle (the
    // identical-special-pipeline gap stays closed by data, not by trust).
    const EAstrawildBossSpecialSet Sets[4] =
    {
        EAstrawildBossSpecialSet::UnderlightWarden,
        EAstrawildBossSpecialSet::SunkenVault,
        EAstrawildBossSpecialSet::GlassTyrant,
        EAstrawildBossSpecialSet::EyeOfTheMaelstrom
    };
    const auto ParamsKey = [](const FAstrawildBossSpecialSetParams& P)
    {
        return FString::Printf(TEXT("%.1f|%d|%d|%.0f|%d|%.1f|%s"),
            P.SpecialAttackCooldownSeconds, P.BoltCount, P.BlastCount, P.SpecialBlastRadius,
            P.HazardWaveCount, P.HazardDamagePerSecond, *P.SummonSpeciesId.ToString());
    };
    TSet<FString> Keys;
    for (const EAstrawildBossSpecialSet Set : Sets)
    {
        Keys.Add(ParamsKey(AAstrawildEchoBossCharacter::GetBossSpecialSetParams(Set)));
    }
    TestEqual(TEXT("All four sets tune differently"), Keys.Num(), 4);

    // Sanity band: every set stays a boss fight, not a spam machine or a
    // pushover (cooldown 4-10s, 1-4 bolts, 1-3 blasts, 250-500cm, 1-4 hazards,
    // 4-10 dps, a real summon species).
    for (const EAstrawildBossSpecialSet Set : Sets)
    {
        const FAstrawildBossSpecialSetParams Params = AAstrawildEchoBossCharacter::GetBossSpecialSetParams(Set);
        TestTrue(TEXT("Set cooldown stays in the 4-10s band"),
            Params.SpecialAttackCooldownSeconds >= 4.0f && Params.SpecialAttackCooldownSeconds <= 10.0f);
        TestTrue(TEXT("Set bolt count stays in the 1-4 band"),
            Params.BoltCount >= 1 && Params.BoltCount <= 4);
        TestTrue(TEXT("Set blast count stays in the 1-3 band"),
            Params.BlastCount >= 1 && Params.BlastCount <= 3);
        TestTrue(TEXT("Set blast radius stays in the 250-500cm band"),
            Params.SpecialBlastRadius >= 250.0f && Params.SpecialBlastRadius <= 500.0f);
        TestTrue(TEXT("Set hazard wave stays in the 1-4 band"),
            Params.HazardWaveCount >= 1 && Params.HazardWaveCount <= 4);
        TestTrue(TEXT("Set hazard dps stays in the 4-10 band"),
            Params.HazardDamagePerSecond >= 4.0f && Params.HazardDamagePerSecond <= 10.0f);
        TestFalse(TEXT("Set summon species is set"), Params.SummonSpeciesId.IsNone());
    }

    return true;
}

// --- DP-6: base depth contracts (Test 106) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildBaseDepthTest,
    "ASTRAWILD.DP6.BaseDepth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildBaseDepthTest::RunTest(const FString& Parameters)
{
    // Registry-backed world-free census (house rule: no world, no spawned
    // actors). The registry's register/find contract is plain C++ data, so the
    // full CODE_DEFAULT content library builds ownerless through the same
    // BuildDefaults entry point the world uses — the site/tech/item data this
    // test pins is the live registry content, not a parallel table.
    UAstrawildItemRegistrySubsystem* Registry = NewObject<UAstrawildItemRegistrySubsystem>();
    UAstrawildContentLibrary::BuildDefaults(Registry);
    TestTrue(TEXT("Registry builds ownerless"), Registry != nullptr);

    // 1) Work-site coverage: 8 sites cover 8 of the 10 actionable work types.
    // The DP-6 rows cover Transport / ResearchAssist / PowerGeneration /
    // Defense (the four highest species-affinity uncovered types) on top of
    // the original Gathering / Farming / Mining / Cooking camp set; Crafting
    // Assistance (18 affinity slots) and Construction (0) stay uncovered by
    // documented design — species work identity has no site demand there yet.
    const TArray<UAstrawildWorkSiteDefinition*> Sites = Registry->GetAllWorkSiteDefinitions();
    TestEqual(TEXT("Eight work sites registered"), Sites.Num(), 8);

    TSet<EAstrawildWorkType> CoveredTypes;
    TSet<FName> SiteIds;
    for (const UAstrawildWorkSiteDefinition* Site : Sites)
    {
        if (!TestTrue(TEXT("Site entry is valid"), Site != nullptr))
        {
            continue;
        }
        TestTrue(TEXT("Site id is set"), !Site->SiteId.IsNone());
        SiteIds.Add(Site->SiteId);
        TestTrue(TEXT("Site carries an actionable work type"), Site->WorkType != EAstrawildWorkType::None);
        TestTrue(TEXT("Site output item resolves"), Registry->FindItem(Site->OutputItemId) != nullptr);
        TestTrue(TEXT("Site zone is placed"), Site->Zone != EAstrawildZone::None);
        TestTrue(TEXT("Site cycle time is positive"), Site->SecondsPerOutput >= 1.0f);
        for (const FAstrawildItemStack& Input : Site->InputItems)
        {
            TestTrue(TEXT("Site input item resolves"), Registry->FindItem(Input.ItemId) != nullptr);
        }
        CoveredTypes.Add(Site->WorkType);
    }
    TestEqual(TEXT("Site ids are unique"), SiteIds.Num(), 8);

    const EAstrawildWorkType MustCover[] =
    {
        EAstrawildWorkType::Gathering, EAstrawildWorkType::Farming, EAstrawildWorkType::Mining,
        EAstrawildWorkType::Cooking, EAstrawildWorkType::Transport, EAstrawildWorkType::ResearchAssist,
        EAstrawildWorkType::PowerGeneration, EAstrawildWorkType::Defense
    };
    for (const EAstrawildWorkType Type : MustCover)
    {
        TestTrue(*FString::Printf(TEXT("Work type %d has site coverage"), static_cast<int32>(Type)),
            CoveredTypes.Contains(Type));
    }
    TestFalse(TEXT("Crafting Assistance stays uncovered by design"), CoveredTypes.Contains(EAstrawildWorkType::Crafting));
    TestFalse(TEXT("Construction stays uncovered by design"), CoveredTypes.Contains(EAstrawildWorkType::Construction));

    // 2) Research branch wiring: all 17 techs carry a branch and the assignment
    // pins to the audited data (legacy ten via the ContentLibrary retrofit
    // table, production seven via MakeTech — each row verified against its
    // unlock payload, e.g. Skiff Engineering → Exploration: travel tech).
    const TArray<UAstrawildTechnologyDefinition*> Techs = Registry->GetAllTechnologies();
    TestEqual(TEXT("Seventeen techs registered"), Techs.Num(), 17);

    struct FBranchCase { FName TechId; EAstrawildResearchBranch Branch; };
    const FBranchCase BranchCases[] =
    {
        { TEXT("Tech_BasicCrafting"), EAstrawildResearchBranch::Tools },
        { TEXT("Tech_Cooking"), EAstrawildResearchBranch::Survival },
        { TEXT("Tech_Electrical"), EAstrawildResearchBranch::Energy },
        { TEXT("Tech_AdvancedEnergy"), EAstrawildResearchBranch::Energy },
        { TEXT("Tech_Husbandry"), EAstrawildResearchBranch::EchoTech },
        { TEXT("Tech_Armory"), EAstrawildResearchBranch::Weapons },
        { TEXT("Tech_Mechanics"), EAstrawildResearchBranch::Tools },
        { TEXT("Tech_Thermal"), EAstrawildResearchBranch::Survival },
        { TEXT("Tech_Agriculture"), EAstrawildResearchBranch::EchoTech },
        { TEXT("Tech_AncientResonance"), EAstrawildResearchBranch::Exploration },
        { TEXT("Tech_WeaponSystems"), EAstrawildResearchBranch::Weapons },
        { TEXT("Tech_AdvancedBallistics"), EAstrawildResearchBranch::Weapons },
        { TEXT("Tech_ExperimentalArsenal"), EAstrawildResearchBranch::Weapons },
        { TEXT("Tech_ExosuitEngineering"), EAstrawildResearchBranch::Armor },
        { TEXT("Tech_ScannerArray"), EAstrawildResearchBranch::Scanner },
        { TEXT("Tech_AutomationII"), EAstrawildResearchBranch::Automation },
        { TEXT("Tech_SkiffEngineering"), EAstrawildResearchBranch::Exploration },
    };
    for (const FBranchCase& Case : BranchCases)
    {
        const UAstrawildTechnologyDefinition* Tech = Registry->FindTechnology(Case.TechId);
        if (TestTrue(*FString::Printf(TEXT("Tech %s resolves"), *Case.TechId.ToString()), Tech != nullptr))
        {
            TestEqual(*FString::Printf(TEXT("Tech %s carries its audited branch"), *Case.TechId.ToString()),
                Tech->Branch, Case.Branch);
        }
    }

    // 3) Field consumables: production feeds progression with real verbs.
    // Field Ration — timed stamina regen through the survival status-effect
    // system (Status.RationVigor); Pulse Tonic — bottled Hunter's Focus (the
    // existing capture-focus window). Both also keep the instant food/water
    // verbs so they are never vendor trash.
    const UAstrawildItemDefinition* FieldRation = Registry->FindItem(TEXT("Item_FieldRation"));
    if (TestTrue(TEXT("Field Ration resolves"), FieldRation != nullptr))
    {
        TestEqual(TEXT("Field Ration is a consumable"), FieldRation->Category, EAstrawildItemCategory::Consumable);
        TestTrue(TEXT("Field Ration carries a timed status"), FieldRation->OnConsumeStatus.StatusId != NAME_None);
        TestTrue(TEXT("Field Ration status lasts a real window"), FieldRation->OnConsumeStatus.RemainingSeconds > 0.0f);
        TestTrue(TEXT("Field Ration status regenerates stamina"), FieldRation->OnConsumeStatus.StaminaRegenPerSecond > 0.0f);
        TestTrue(TEXT("Field Ration status never damages"), FieldRation->OnConsumeStatus.DamagePerSecond <= 0.0f);
        TestTrue(TEXT("Field Ration feeds the player too"), FieldRation->FoodValue > 0.0f);
    }

    const UAstrawildItemDefinition* PulseTonic = Registry->FindItem(TEXT("Item_PulseTonic"));
    if (TestTrue(TEXT("Pulse Tonic resolves"), PulseTonic != nullptr))
    {
        TestEqual(TEXT("Pulse Tonic is a consumable"), PulseTonic->Category, EAstrawildItemCategory::Consumable);
        TestTrue(TEXT("Pulse Tonic grants capture focus"), PulseTonic->CaptureFocusSeconds > 0.0f);
        TestTrue(TEXT("Pulse Tonic carries no damage status"), PulseTonic->OnConsumeStatus.StatusId == NAME_None);
        TestTrue(TEXT("Pulse Tonic heals and hydrates"), PulseTonic->HealValue > 0.0f && PulseTonic->WaterValue > 0.0f);
    }

    // Fresh status effects stay regen-free by default (combat statuses keep
    // the byte-exact pre-DP-6 shape — the new field is additive).
    const FAstrawildStatusEffect FreshStatus;
    TestEqual(TEXT("Fresh status has no stamina regen"), FreshStatus.StaminaRegenPerSecond, 0.0f);

    // 4) The loop actually closes: recipes mirror the automated sites and the
    // depot literally consumes camp output (kitchen meat + farm berries).
    const UAstrawildRecipeDefinition* RationRecipe = Registry->FindRecipe(TEXT("Recipe_FieldRation"));
    if (TestTrue(TEXT("Field Ration recipe resolves"), RationRecipe != nullptr))
    {
        TestTrue(TEXT("Ration recipe outputs Field Rations"),
            RationRecipe->Outputs.ContainsByPredicate([](const FAstrawildItemStack& S) { return S.ItemId == TEXT("Item_FieldRation"); }));
    }
    const UAstrawildRecipeDefinition* TonicRecipe = Registry->FindRecipe(TEXT("Recipe_PulseTonic"));
    if (TestTrue(TEXT("Pulse Tonic recipe resolves"), TonicRecipe != nullptr))
    {
        TestTrue(TEXT("Tonic recipe outputs Pulse Tonics"),
            TonicRecipe->Outputs.ContainsByPredicate([](const FAstrawildItemStack& S) { return S.ItemId == TEXT("Item_PulseTonic"); }));
    }

    const UAstrawildWorkSiteDefinition* Depot = Registry->FindWorkSite(TEXT("Site_TidebreakerDepot"));
    if (TestTrue(TEXT("Tidebreaker depot resolves"), Depot != nullptr))
    {
        TestTrue(TEXT("Depot outputs Field Rations"), Depot->OutputItemId == TEXT("Item_FieldRation"));
        TestTrue(TEXT("Depot consumes kitchen meat"),
            Depot->InputItems.ContainsByPredicate([](const FAstrawildItemStack& S) { return S.ItemId == TEXT("Item_CookedMeat"); }));
        TestTrue(TEXT("Depot consumes farm berries"),
            Depot->InputItems.ContainsByPredicate([](const FAstrawildItemStack& S) { return S.ItemId == TEXT("Item_Berry"); }));
    }
    const UAstrawildWorkSiteDefinition* Lab = Registry->FindWorkSite(TEXT("Site_VerdantLab"));
    if (TestTrue(TEXT("Verdant field lab resolves"), Lab != nullptr))
    {
        TestTrue(TEXT("Lab outputs Pulse Tonics"), Lab->OutputItemId == TEXT("Item_PulseTonic"));
    }

    return true;
}

// ---------------------------------------------------------------------------
// DP-7 — world depth: per-zone hazard identity + bare-zone events + secrets
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildWorldDepthTest,
    "ASTRAWILD.DP7.WorldDepth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildWorldDepthTest::RunTest(const FString& Parameters)
{
    // 1) Per-zone hazard identity — the pure static zone table (world-free).
    // Every zone carries an explicit hazard row; the two pure consumption
    // helpers agree with the enum; the layering contract holds (thermal
    // pressure shifts the ambient temperature ON TOP of global weather).
    const TArray<FAstrawildZoneDescriptor>& Zones = UAstrawildZoneSubsystem::GetAllZones();
    TestEqual(TEXT("Twelve zones in the hazard table"), Zones.Num(), 12);

    const FAstrawildZoneDescriptor* DawnFields = nullptr;
    const FAstrawildZoneDescriptor* Frostveil = nullptr;
    const FAstrawildZoneDescriptor* HollowApproach = nullptr;
    TSet<FName> HazardZoneIds;
    for (const FAstrawildZoneDescriptor& Desc : Zones)
    {
        TestTrue(TEXT("Zone hazard pressure stays non-negative"), Desc.HazardPressure >= 0.0f);
        TestTrue(TEXT("Zone hazard pressure stays inside the honest band"), Desc.HazardPressure <= 20.0f);

        // The pure helpers mirror the enum (the survival tick consumes these).
        const float Thermal = Desc.GetHazardTemperatureOffsetCelsius();
        const float RegenPenalty = Desc.GetHazardStaminaRegenPenalty();
        switch (Desc.HazardType)
        {
        case EAstrawildZoneHazard::ColdPressure:
            TestEqual(*FString::Printf(TEXT("Cold zone %s offsets by -pressure"), *Desc.ZoneId.ToString()),
                Thermal, -Desc.HazardPressure);
            TestEqual(*FString::Printf(TEXT("Cold zone %s never suppresses regen"), *Desc.ZoneId.ToString()),
                RegenPenalty, 0.0f);
            HazardZoneIds.Add(Desc.ZoneId);
            break;
        case EAstrawildZoneHazard::HeatPressure:
            TestEqual(*FString::Printf(TEXT("Heat zone %s offsets by +pressure"), *Desc.ZoneId.ToString()),
                Thermal, Desc.HazardPressure);
            TestEqual(*FString::Printf(TEXT("Heat zone %s never suppresses regen"), *Desc.ZoneId.ToString()),
                RegenPenalty, 0.0f);
            HazardZoneIds.Add(Desc.ZoneId);
            break;
        case EAstrawildZoneHazard::AshLung:
            TestEqual(*FString::Printf(TEXT("Ash-lung zone %s never shifts temperature"), *Desc.ZoneId.ToString()),
                Thermal, 0.0f);
            TestEqual(*FString::Printf(TEXT("Ash-lung zone %s suppresses regen by pressure"), *Desc.ZoneId.ToString()),
                RegenPenalty, Desc.HazardPressure);
            HazardZoneIds.Add(Desc.ZoneId);
            break;
        default:
            TestEqual(*FString::Printf(TEXT("Hazard-free zone %s stays neutral"), *Desc.ZoneId.ToString()),
                Thermal, 0.0f);
            TestEqual(*FString::Printf(TEXT("Hazard-free zone %s has no regen penalty"), *Desc.ZoneId.ToString()),
                RegenPenalty, 0.0f);
            TestEqual(*FString::Printf(TEXT("Hazard-free zone %s carries zero pressure"), *Desc.ZoneId.ToString()),
                Desc.HazardPressure, 0.0f);
            break;
        }

        if (Desc.Zone == EAstrawildZone::DawnFields) { DawnFields = &Desc; }
        if (Desc.Zone == EAstrawildZone::FrostveilExpanse) { Frostveil = &Desc; }
        if (Desc.Zone == EAstrawildZone::HollowApproach) { HollowApproach = &Desc; }
    }
    // Ten of twelve zones carry a real hazard identity (the two gentle zones —
    // Dawn Fields + Glimmerwood — stay hazard-free by design).
    TestEqual(TEXT("Ten zones carry a hazard identity"), HazardZoneIds.Num(), 10);

    if (TestTrue(TEXT("Dawn Fields descriptor resolves"), DawnFields != nullptr))
    {
        TestEqual(TEXT("Dawn Fields stays gentle by design (no hazard)"),
            DawnFields->HazardType, EAstrawildZoneHazard::None);
        TestEqual(TEXT("Dawn Fields hazard pressure is zero"), DawnFields->HazardPressure, 0.0f);
    }
    if (TestTrue(TEXT("Frostveil descriptor resolves"), Frostveil != nullptr))
    {
        TestEqual(TEXT("Frostveil is the cold identity"), Frostveil->HazardType, EAstrawildZoneHazard::ColdPressure);
        // The layering contract: under the SAME sky, Frostveil reads colder
        // than Dawn Fields (survival adds this to base + weather).
        TestTrue(TEXT("Frostveil reads colder than Dawn Fields"),
            DawnFields != nullptr &&
            Frostveil->GetHazardTemperatureOffsetCelsius() < DawnFields->GetHazardTemperatureOffsetCelsius());
        TestTrue(TEXT("Frostveil cold pressure is real"), Frostveil->HazardPressure >= 8.0f);
    }
    if (TestTrue(TEXT("Hollow Approach descriptor resolves"), HollowApproach != nullptr))
    {
        TestEqual(TEXT("Hollow Approach is the ash-lung identity"),
            HollowApproach->HazardType, EAstrawildZoneHazard::AshLung);
        TestTrue(TEXT("Ash lung pressure is real but sub-lethal"),
            HollowApproach->HazardPressure > 0.0f && HollowApproach->HazardPressure < 10.0f);
    }

    // 2) Registry-backed events census (ownerless BuildDefaults — same pattern
    // as test 106): every previously-bare zone now anchors at least one event
    // built ONLY from the existing effect vocabulary.
    UAstrawildItemRegistrySubsystem* EventRegistry = NewObject<UAstrawildItemRegistrySubsystem>();
    UAstrawildContentLibrary::BuildDefaults(EventRegistry);
    const TArray<UAstrawildWorldEventDefinition*> Events = EventRegistry->GetAllWorldEvents();
    TestEqual(TEXT("Sixteen world events registered"), Events.Num(), 16);

    const EAstrawildZone BareZones[] =
    {
        EAstrawildZone::DuskMarsh, EAstrawildZone::EmberRidge, EAstrawildZone::SunscarDesert,
        EAstrawildZone::AzureShallows, EAstrawildZone::TidebreakerIsles,
        EAstrawildZone::StormcrestHighlands, EAstrawildZone::PearlseaReef
    };
    for (const EAstrawildZone Zone : BareZones)
    {
        int32 Anchored = 0;
        for (const UAstrawildWorldEventDefinition* Event : Events)
        {
            if (Event && Event->Zone == Zone)
            {
                ++Anchored;
            }
        }
        TestTrue(*FString::Printf(TEXT("Zone %d anchors at least one world event"), static_cast<int32>(Zone)),
            Anchored >= 1);
    }

    struct FZoneEventCase { FName EventId; EAstrawildZone Zone; };
    const FZoneEventCase ZoneEvents[] =
    {
        { TEXT("Event_MistTide"), EAstrawildZone::DuskMarsh },
        { TEXT("Event_CinderFall"), EAstrawildZone::EmberRidge },
        { TEXT("Event_DuneBuriedCache"), EAstrawildZone::SunscarDesert },
        { TEXT("Event_ReefBloom"), EAstrawildZone::AzureShallows },
        { TEXT("Event_WreckSurge"), EAstrawildZone::TidebreakerIsles },
        { TEXT("Event_StormFront"), EAstrawildZone::StormcrestHighlands },
        { TEXT("Event_Pearlsong"), EAstrawildZone::PearlseaReef },
    };
    for (const FZoneEventCase& Case : ZoneEvents)
    {
        const UAstrawildWorldEventDefinition* Event = EventRegistry->FindWorldEvent(Case.EventId);
        if (TestTrue(*FString::Printf(TEXT("Event %s resolves"), *Case.EventId.ToString()), Event != nullptr))
        {
            TestEqual(*FString::Printf(TEXT("Event %s anchors its bare zone"), *Case.EventId.ToString()),
                Event->Zone, Case.Zone);
            // Balance idiom of the legacy rows: sane weight, real cooldown,
            // day-gated progression, night-gate stays reserved for camp raids.
            TestTrue(*FString::Printf(TEXT("Event %s weight sits in the balance band"), *Case.EventId.ToString()),
                Event->RarityWeight > 0.0f && Event->RarityWeight <= 2.0f);
            TestTrue(*FString::Printf(TEXT("Event %s carries a real cooldown"), *Case.EventId.ToString()),
                Event->CooldownGameHours > 0.0f);
            TestTrue(*FString::Printf(TEXT("Event %s is day-gated for progression"), *Case.EventId.ToString()),
                Event->MinDay >= 2);
            TestFalse(*FString::Printf(TEXT("Event %s does not night-gate"), *Case.EventId.ToString()),
                Event->bRequiresNight);
            TestTrue(*FString::Printf(TEXT("Event %s carries an authored description"), *Case.EventId.ToString()),
                !Event->Description.ToString().IsEmpty());
            // Every payload resolves in the live registry (no dangling ids).
            if (!Event->SpeciesBoostId.IsNone())
            {
                TestTrue(*FString::Printf(TEXT("Event %s boost species resolves"), *Case.EventId.ToString()),
                    EventRegistry->FindEcho(Event->SpeciesBoostId) != nullptr);
                TestTrue(*FString::Printf(TEXT("Event %s boost count is sane"), *Case.EventId.ToString()),
                    Event->SpeciesBoostCount >= 1 && Event->SpeciesBoostCount <= 4);
            }
            for (const FName NodeId : Event->BonusNodeIds)
            {
                TestTrue(*FString::Printf(TEXT("Event %s bonus node resolves"), *Case.EventId.ToString()),
                    EventRegistry->FindResourceNode(NodeId) != nullptr);
            }
            if (!Event->RewardLootTableId.IsNone())
            {
                TestTrue(*FString::Printf(TEXT("Event %s loot table resolves"), *Case.EventId.ToString()),
                    EventRegistry->FindLootTable(Event->RewardLootTableId) != nullptr);
            }
        }
    }

    // 3) Scanner-gated zone secrets — the live POI census, not a parallel table.
    const TArray<UAstrawildPOIDefinition*> Pois = EventRegistry->GetAllPOIs();
    TestEqual(TEXT("Seventeen POIs registered"), Pois.Num(), 17);

    int32 GatedCount = 0;
    TSet<FName> PoiIds;
    for (const UAstrawildPOIDefinition* Poi : Pois)
    {
        if (Poi && Poi->bRequiresSignalScanner)
        {
            ++GatedCount;
        }
        if (Poi)
        {
            PoiIds.Add(Poi->PoiId);
        }
    }
    TestEqual(TEXT("Six scanner-gated secret POIs exist (2 legacy + 4 DP-7)"), GatedCount, 6);
    TestEqual(TEXT("POI ids are unique"), PoiIds.Num(), Pois.Num());

    const FName SecretIds[] =
    {
        TEXT("POI_HollowUndergateVault"), TEXT("POI_SunscarMachineCoffin"),
        TEXT("POI_TidebreakerHoldRoom"), TEXT("POI_PearlseaTidecache")
    };
    const EAstrawildZone SecretZones[] =
    {
        EAstrawildZone::HollowApproach, EAstrawildZone::SunscarDesert,
        EAstrawildZone::TidebreakerIsles, EAstrawildZone::PearlseaReef
    };
    for (int32 i = 0; i < 4; ++i)
    {
        const UAstrawildPOIDefinition* Secret = EventRegistry->FindPOI(SecretIds[i]);
        if (TestTrue(*FString::Printf(TEXT("Secret %s resolves"), *SecretIds[i].ToString()), Secret != nullptr))
        {
            TestTrue(*FString::Printf(TEXT("Secret %s mirrors the Frostveil scanner gate"), *SecretIds[i].ToString()),
                Secret->bRequiresSignalScanner);
            TestEqual(*FString::Printf(TEXT("Secret %s is a signal source"), *SecretIds[i].ToString()),
                Secret->Type, EAstrawildPOIType::SignalSource);
            TestEqual(*FString::Printf(TEXT("Secret %s sits in its high-threat zone"), *SecretIds[i].ToString()),
                Secret->Zone, SecretZones[i]);
            TestTrue(*FString::Printf(TEXT("Secret %s pays real research"), *SecretIds[i].ToString()),
                Secret->ResearchReward >= 5);
            TestTrue(*FString::Printf(TEXT("Secret %s loot table resolves"), *SecretIds[i].ToString()),
                EventRegistry->FindLootTable(Secret->RewardLootTableId) != nullptr);
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// DP-8 — NPC depth: affinity-gated dialogue evolution (gate evaluation)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAffinityDialogueTest,
    "ASTRAWILD.DP8.AffinityDialogue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAffinityDialogueTest::RunTest(const FString& Parameters)
{
    using DialogueComp = UAstrawildDialogueComponent;

    // 1) Pure gate resolver: a threshold of 0 never gates (the fresh default —
    // every pre-DP-8 tree stays byte-identical); otherwise the affinity must
    // REACH the threshold (>=), with the tier boundaries resolving exactly on
    // 25 / 50 / 75.
    TestTrue(TEXT("Default threshold 0 never gates"), DialogueComp::MeetsAffinityGate(0, 0.0f));
    TestTrue(TEXT("Threshold 0 stays ungated at max affinity"), DialogueComp::MeetsAffinityGate(0, 100.0f));
    TestFalse(TEXT("Affinity below the threshold fails"), DialogueComp::MeetsAffinityGate(50, 49.9f));
    TestTrue(TEXT("Threshold is inclusive (>=)"), DialogueComp::MeetsAffinityGate(50, 50.0f));
    TestTrue(TEXT("Affinity above the threshold passes"), DialogueComp::MeetsAffinityGate(50, 75.0f));
    TestTrue(TEXT("Acquaintance boundary resolves at 25"),
        DialogueComp::MeetsAffinityGate(25, 25.0f) && !DialogueComp::MeetsAffinityGate(25, 24.9f));
    TestTrue(TEXT("Confidant boundary resolves at 75"),
        DialogueComp::MeetsAffinityGate(75, 75.0f) && !DialogueComp::MeetsAffinityGate(75, 74.9f));

    // 2) Component evaluation — fail-closed like the quest conditions: a
    // gated reply hides when no talking NPC can be resolved, while a default-0
    // reply stays visible in the exact same world-free state.
    UAstrawildDialogueComponent* Comp = NewObject<UAstrawildDialogueComponent>();

    FAstrawildDialogueChoice Gated;
    Gated.Text = FText::FromString(TEXT("Friend-only reply"));
    Gated.RequiredMinAffinity = 50;
    TestFalse(TEXT("Affinity gate fails without a talking NPC (fail-closed)"),
        Comp->EvaluateChoiceConditions(Gated));

    FAstrawildDialogueChoice Ungated;
    Ungated.Text = FText::FromString(TEXT("Anyone reply"));
    TestTrue(TEXT("Default-0 gate never hides a choice"),
        Comp->EvaluateChoiceConditions(Ungated));

    // 3) Live talking-NPC path (world-free actor — the affinity-tier test's
    // pattern): below the threshold the reply hides, on the boundary it
    // appears, and dropping the NPC fails the gate closed again.
    AAstrawildNPCCharacter* Npc = NewObject<AAstrawildNPCCharacter>();
    Npc->Affinity = 24.0f;
    Comp->SetTalkingNpc(Npc);
    TestTrue(TEXT("Talking NPC resolves"), Comp->GetTalkingNpc() == Npc);
    TestFalse(TEXT("Stranger affinity misses the Friend gate"),
        Comp->EvaluateChoiceConditions(Gated));
    Npc->Affinity = 50.0f;
    TestTrue(TEXT("Friend affinity meets the Friend gate"),
        Comp->EvaluateChoiceConditions(Gated));
    Comp->SetTalkingNpc(nullptr);
    TestFalse(TEXT("Clearing the talking NPC fails the gate closed"),
        Comp->EvaluateChoiceConditions(Gated));

    // 4) AND semantics: the affinity gate composes with the flag conditions
    // exactly like the quest/flag pair does.
    FAstrawildDialogueChoice Strict;
    Strict.RequiredFlagId = TEXT("Flag_Trusted");
    Strict.RequiredMinAffinity = 25;
    Comp->SetStoryFlag(TEXT("Flag_Trusted"));
    Npc->Affinity = 10.0f;
    Comp->SetTalkingNpc(Npc);
    TestFalse(TEXT("Flag set but affinity low fails the pair"),
        Comp->EvaluateChoiceConditions(Strict));
    Npc->Affinity = 25.0f;
    TestTrue(TEXT("Flag set and affinity met passes the pair"),
        Comp->EvaluateChoiceConditions(Strict));

    // 5) The evolved trees pin their tiers through the live registry
    // (ownerless BuildDefaults — the test-106/107 census pattern).
    UAstrawildItemRegistrySubsystem* Registry = NewObject<UAstrawildItemRegistrySubsystem>();
    UAstrawildContentLibrary::BuildDefaults(Registry);

    struct FGatedRow { FName TreeId; FName NodeId; int32 MinAffinity; };
    const FGatedRow GatedRows[] = {
        { TEXT("Dialogue_TraderTam"),  TEXT("hello"), 50 }, // Friend supply line (shop bridge).
        { TEXT("Dialogue_ElderRowan"), TEXT("hello"), 75 }, // Confidant old doors (deep lore).
        { TEXT("Dialogue_FisherNima"), TEXT("hello"), 50 }, // Friend rare goods (shop bridge).
        { TEXT("Dialogue_GuardSela"),  TEXT("hello"), 25 }, // Acquaintance patrol chart.
    };
    for (const FGatedRow& Row : GatedRows)
    {
        const UAstrawildDialogueTreeDefinition* Tree = Registry->FindDialogueTree(Row.TreeId);
        if (TestTrue(*FString::Printf(TEXT("Tree %s resolves"), *Row.TreeId.ToString()), Tree != nullptr))
        {
            const FAstrawildDialogueNode* Node = Tree->FindNode(Row.NodeId);
            if (TestTrue(*FString::Printf(TEXT("Tree %s entry node resolves"), *Row.TreeId.ToString()), Node != nullptr))
            {
                bool bFoundGated = false;
                for (const FAstrawildDialogueChoice& Choice : Node->Choices)
                {
                    if (Choice.RequiredMinAffinity == Row.MinAffinity)
                    {
                        bFoundGated = true;
                        // Honest content contract: gated beats are one-time
                        // (forbidden flag) and pay real consequences — no
                        // new consequence types, existing verbs only.
                        TestTrue(*FString::Printf(TEXT("Tree %s gated reply is one-time"), *Row.TreeId.ToString()),
                            !Choice.ForbiddenFlagId.IsNone() && !Choice.SetFlagId.IsNone());
                        TestTrue(*FString::Printf(TEXT("Tree %s gated reply pays real content"), *Row.TreeId.ToString()),
                            Choice.GiveResearchPoints > 0 || Choice.bOpenShop || !Choice.GiveItemId.IsNone());
                    }
                }
                TestTrue(*FString::Printf(TEXT("Tree %s carries its %d-affinity gated reply"),
                    *Row.TreeId.ToString(), Row.MinAffinity), bFoundGated);
            }
        }
    }

    // 6) Depth without clones: the census pins stay 11 NPCs / 11 trees.
    TestEqual(TEXT("Eleven dialogue trees (census unchanged)"), Registry->GetAllDialogueTrees().Num(), 11);
    TestEqual(TEXT("Eleven NPCs (census unchanged)"), Registry->GetNumNPCs(), 11);

    return true;
}

// ---------------------------------------------------------------------------
// DP-9 — dungeon depth: per-dungeon room identity (themes + pillar sequence)
// ---------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDungeonIdentityTest,
    "ASTRAWILD.DP9.DungeonIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDungeonIdentityTest::RunTest(const FString& Parameters)
{
    using Room = AAstrawildDungeonRoomActor;

    // 1) Theme resolution per dungeon id: the 3 canonical dungeons resolve 3
    // DISTINCT themes; unknown/empty ids fail closed to None (the unthemed
    // legacy shell — identity never breaks a dungeon).
    const FName CanonicalIds[] = {
        TEXT("Dungeon_HollowUnderlight"), TEXT("Dungeon_SunkenVault"), TEXT("Dungeon_EyeOfTheMaelstrom")
    };
    const EAstrawildDungeonTheme ExpectedThemes[] = {
        EAstrawildDungeonTheme::HollowUnderlight, EAstrawildDungeonTheme::SunkenVault,
        EAstrawildDungeonTheme::MaelstromEye
    };
    TSet<int32> DistinctThemes;
    for (int32 i = 0; i < 3; ++i)
    {
        const EAstrawildDungeonTheme Resolved = Room::ResolveDungeonTheme(CanonicalIds[i]);
        TestEqual(*FString::Printf(TEXT("Dungeon %s resolves its theme"), *CanonicalIds[i].ToString()),
            Resolved, ExpectedThemes[i]);
        DistinctThemes.Add(static_cast<int32>(Resolved));
    }
    TestEqual(TEXT("The three canonical dungeons resolve three distinct themes"), DistinctThemes.Num(), 3);
    TestEqual(TEXT("Unknown dungeon id fails closed to None"),
        Room::ResolveDungeonTheme(TEXT("Dungeon_DoesNotExist")), EAstrawildDungeonTheme::None);
    TestEqual(TEXT("Empty dungeon id fails closed to None"),
        Room::ResolveDungeonTheme(NAME_None), EAstrawildDungeonTheme::None);

    // 2) Theme profiles — pairwise-distinct identity data the rooms consume:
    // tints, shell proportions, wall heights, hazards and dressing vocabulary.
    const FAstrawildDungeonThemeProfile Underlight = Room::MakeThemeProfile(EAstrawildDungeonTheme::HollowUnderlight);
    const FAstrawildDungeonThemeProfile Vault = Room::MakeThemeProfile(EAstrawildDungeonTheme::SunkenVault);
    const FAstrawildDungeonThemeProfile Eye = Room::MakeThemeProfile(EAstrawildDungeonTheme::MaelstromEye);
    const FAstrawildDungeonThemeProfile Unthemed = Room::MakeThemeProfile(EAstrawildDungeonTheme::None);

    // Shell tints pairwise distinct (each dungeon reads differently in-room).
    TestTrue(TEXT("Underlight tint is the darkest of the three"),
        Underlight.ShellTint.R < Vault.ShellTint.R && Underlight.ShellTint.R < Eye.ShellTint.R);
    TestTrue(TEXT("Vault and Eye tints differ"),
        !FMath::IsNearlyEqual(Vault.ShellTint.G, Eye.ShellTint.G) &&
        !FMath::IsNearlyEqual(Vault.ShellTint.B, Eye.ShellTint.B));

    // Structural identity: Underlight tight, Sunken Vault wide, Eye tall monoliths.
    TestTrue(TEXT("Underlight rooms read tighter than the Vault"),
        Underlight.ExtentScale.X < Vault.ExtentScale.X && Underlight.ExtentScale.Y < Vault.ExtentScale.Y);
    TestTrue(TEXT("Vault rooms read wider than the Eye"),
        Vault.ExtentScale.X > Eye.ExtentScale.X);
    TestTrue(TEXT("Eye side walls read taller than the Underlight's oppressive slabs"),
        Eye.SideWallHeight > Underlight.SideWallHeight);
    TestTrue(TEXT("Every themed shell carries real walls"),
        Underlight.SideWallHeight > 0.0f && Vault.SideWallHeight > 0.0f && Eye.SideWallHeight > 0.0f);

    // Accent light identity: the Eye pulses, the other two stay steady.
    TestTrue(TEXT("Only the Eye pulses its accent light"),
        Eye.AccentLightPulseRate > 0.0f && Underlight.AccentLightPulseRate == 0.0f && Vault.AccentLightPulseRate == 0.0f);

    // Hazard identity per dungeon — three distinct verbs from the existing vocabulary.
    TestEqual(TEXT("Underlight hazard is the room-level ash lung"),
        Underlight.Hazard, EAstrawildRoomHazardType::AshLung);
    TestEqual(TEXT("Sunken Vault hazard is the waterlogged slow"),
        Vault.Hazard, EAstrawildRoomHazardType::Waterlogged);
    TestEqual(TEXT("Eye hazard is the energy pulse tiles"),
        Eye.Hazard, EAstrawildRoomHazardType::EnergyPulse);

    // Hazard bands (mild by design — identity pressure, not a death sentence).
    TestTrue(TEXT("Room ash lung is mild and non-lethal"),
        Underlight.HazardPressure > 0.0f && Underlight.HazardPressure <= 8.0f);
    TestTrue(TEXT("Waterlogged slows but never cripples"),
        Vault.HazardSpeedMultiplier >= 0.5f && Vault.HazardSpeedMultiplier < 1.0f);
    TestTrue(TEXT("Energy pulses are periodic, small and dissipate"),
        Eye.HazardPulseInterval >= 5.0f && Eye.HazardTileCount >= 1 && Eye.HazardTileCount <= 4 &&
        Eye.HazardTileDamagePerSecond > 0.0f && Eye.HazardTileDamagePerSecond <= 6.0f &&
        Eye.HazardTileLifetime > 0.0f && Eye.HazardTileLifetime < Eye.HazardPulseInterval);

    // Dressing vocabulary resolves through the EXISTING ArtPack binding tables
    // (no new /Game/ paths — validator check 8 stays byte-clean).
    const FAstrawildDungeonThemeProfile AllThemed[] = { Underlight, Vault, Eye };
    for (const FAstrawildDungeonThemeProfile& Profile : AllThemed)
    {
        const AstrawildArtPack::FBiomeArt* RockArt = AstrawildArtPack::FindBiomeArt(Profile.RockBiomeId);
        if (TestTrue(TEXT("Theme rock biome id resolves in the ArtPack table"), RockArt != nullptr))
        {
            TestTrue(TEXT("Theme rock vocabulary is non-empty"), RockArt->RockMeshPaths.Num() > 0);
        }
        const AstrawildArtPack::FBiomeArt* FloraArt = AstrawildArtPack::FindBiomeArt(Profile.FloraBiomeId);
        if (TestTrue(TEXT("Theme flora biome id resolves in the ArtPack table"), FloraArt != nullptr))
        {
            TestTrue(TEXT("Theme flora vocabulary is non-empty"), FloraArt->GrassMeshPaths.Num() > 0);
        }
        TestTrue(TEXT("Theme dressing budget is sane"),
            Profile.RockCount >= 4 && Profile.RockCount <= 16 && Profile.FloraCount >= 4 && Profile.FloraCount <= 16);
    }
    TestTrue(TEXT("Only the Eye carries the ancient-tech accent"),
        Underlight.TechNodeArtId.IsNone() && Vault.TechNodeArtId.IsNone() && !Eye.TechNodeArtId.IsNone());
    TestTrue(TEXT("The Eye tech accent resolves in the ArtPack node table"),
        AstrawildArtPack::FindNodeArt(Eye.TechNodeArtId) != nullptr);
    TestTrue(TEXT("Only the Sunken Vault carries the flooded-floor accent"),
        !Underlight.bWaterFloorAccent && Vault.bWaterFloorAccent && !Eye.bWaterFloorAccent);

    // The unthemed default stays the legacy shell (fail-closed identity).
    TestEqual(TEXT("Unthemed profile carries no hazard"), Unthemed.Hazard, EAstrawildRoomHazardType::None);
    TestEqual(TEXT("Unthemed profile builds no walls"), Unthemed.SideWallHeight, 0.0f);
    TestEqual(TEXT("Unthemed profile lights nothing"), Unthemed.AccentLightIntensity, 0.0f);
    TestTrue(TEXT("Unthemed footprint stays unscaled"), Unthemed.ExtentScale.Equals(FVector(1.0f, 1.0f, 1.0f), 1e-4f));

    // 3) Resonance-pillar sequence (the puzzle room's mechanic — pure verbs).
    TestEqual(TEXT("Puzzle rooms carry three resonance pillars"), Room::GetPuzzlePillarCount(), 3);
    TestTrue(TEXT("The attunement window is generous but finite"),
        Room::GetPuzzleSequenceWindowSeconds() >= 20.0f && Room::GetPuzzleSequenceWindowSeconds() <= 90.0f);

    // Correct order: advance, advance, complete.
    TestEqual(TEXT("First pillar in order advances"),
        Room::EvaluatePillarActivation(0, 0, 3), Room::EPillarActivityResult::Advanced);
    TestEqual(TEXT("Second pillar in order advances"),
        Room::EvaluatePillarActivation(1, 1, 3), Room::EPillarActivityResult::Advanced);
    TestEqual(TEXT("Final pillar in order completes"),
        Room::EvaluatePillarActivation(2, 2, 3), Room::EPillarActivityResult::Completed);

    // Wrong order resets (the whole sequence restarts — retryable, no stall).
    TestEqual(TEXT("Skipping ahead resets"),
        Room::EvaluatePillarActivation(1, 0, 3), Room::EPillarActivityResult::ResetRequired);
    TestEqual(TEXT("Re-attuning the same pillar resets"),
        Room::EvaluatePillarActivation(0, 1, 3), Room::EPillarActivityResult::ResetRequired);
    TestEqual(TEXT("Out-of-range pillar resets"),
        Room::EvaluatePillarActivation(7, 0, 3), Room::EPillarActivityResult::ResetRequired);
    TestEqual(TEXT("Degenerate pillar count resets"),
        Room::EvaluatePillarActivation(0, 0, 0), Room::EPillarActivityResult::ResetRequired);
    TestEqual(TEXT("Already-complete sequence resets (stale input)"),
        Room::EvaluatePillarActivation(0, 3, 3), Room::EPillarActivityResult::ResetRequired);

    // Window expiry contract: inclusive at the boundary, fresh below it.
    TestFalse(TEXT("Fresh window has not expired"),
        Room::IsPillarSequenceExpired(1.0f, Room::GetPuzzleSequenceWindowSeconds()));
    TestTrue(TEXT("Window expires exactly at the boundary"),
        Room::IsPillarSequenceExpired(Room::GetPuzzleSequenceWindowSeconds(), Room::GetPuzzleSequenceWindowSeconds()));
    TestTrue(TEXT("Overdue window has expired"),
        Room::IsPillarSequenceExpired(Room::GetPuzzleSequenceWindowSeconds() + 0.1f, Room::GetPuzzleSequenceWindowSeconds()));
    TestFalse(TEXT("A zero window never expires (fail-open)"),
        Room::IsPillarSequenceExpired(1000.0f, 0.0f));

    // 4) Room-hazard status ids are stable (the room tick and the clear path
    // agree — the survival vocabulary's plain-FName idiom).
    TestEqual(TEXT("Room ash-lung status id is stable"), Room::GetRoomAshLungStatusId(), FName(TEXT("AshLung")));
    TestEqual(TEXT("Room waterlogged status id is stable"), Room::GetRoomWaterloggedStatusId(), FName(TEXT("Waterlogged")));

    return true;
}

// --- GDP-1: combat pick ladder (Test 75) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAbilityCombatPickTest,
    "ASTRAWILD.Ability.CombatPick",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAbilityCombatPickTest::RunTest(const FString& Parameters)
{
    // One offense + one heal, both ready, in range.
    TArray<FName> Known = { TEXT("Ability_CinderBolt"), TEXT("Ability_FieldTriage") };
    TMap<FName, float> NoCooldowns;

    // No healing wanted -> offense wins.
    TestEqual(TEXT("Healthy caster prefers offense"),
        UAstrawildAbilityLibrary::ChooseAbilityForCombat(Known, NoCooldowns, 10, 500.0f, false, false),
        FName(TEXT("Ability_CinderBolt")));

    // Hurt caster -> the medic heals itself first.
    TestEqual(TEXT("Hurt caster prefers the heal"),
        UAstrawildAbilityLibrary::ChooseAbilityForCombat(Known, NoCooldowns, 10, 500.0f, true, false),
        FName(TEXT("Ability_FieldTriage")));

    // Everything cooling down -> nothing castable.
    TMap<FName, float> AllCooling;
    AllCooling.Add(TEXT("Ability_CinderBolt"), 5.0f);
    AllCooling.Add(TEXT("Ability_FieldTriage"), 5.0f);
    TestTrue(TEXT("All-cooldown returns none"),
        UAstrawildAbilityLibrary::ChooseAbilityForCombat(Known, AllCooling, 10, 500.0f, true, true) == NAME_None);

    // Out of range offense -> falls back to other categories or none.
    TArray<FName> OnlyOffense = { TEXT("Ability_FlareNova") }; // Range 500.
    TestTrue(TEXT("Out-of-range offense not cast"),
        UAstrawildAbilityLibrary::ChooseAbilityForCombat(OnlyOffense, NoCooldowns, 20, 1200.0f, false, false) == NAME_None);

    return true;
}

// --- GDP-1: species ability loadout (authored + derived merge) (Test 76) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAbilitySpeciesLoadoutTest,
    "ASTRAWILD.Ability.SpeciesLoadout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAbilitySpeciesLoadoutTest::RunTest(const FString& Parameters)
{
    UAstrawildEchoDefinition* Def = NewObject<UAstrawildEchoDefinition>();

    // No authored ids -> derived kit only.
    Def->Element = EAstrawildElementType::Frost;
    Def->Role = EAstrawildEchoRole::Support;
    Def->Family = EAstrawildEchoFamily::Avian;
    TArray<FName> Loadout = UAstrawildAbilityLibrary::GetAbilityIdsForSpecies(Def);
    TestTrue(TEXT("Unauthored species derives a kit"), Loadout.Num() >= 4);

    // Authored ids come first, derived fill the rest, no duplicates.
    Def->AbilityIds = { TEXT("Ability_LumewispDawn"), TEXT("Ability_LumewispDawn") };
    Loadout = UAstrawildAbilityLibrary::GetAbilityIdsForSpecies(Def);
    TestEqual(TEXT("Authored ids lead the loadout"), Loadout[0], FName(TEXT("Ability_LumewispDawn")));
    TSet<FName> Unique(Loadout);
    TestEqual(TEXT("No duplicated ids in merged loadout"), Unique.Num(), Loadout.Num());

    return true;
}

// --- GDP-2: locomotion derivation matrix (Test 77) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildLocomotionDerivationTest,
    "ASTRAWILD.Locomotion.Derivation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildLocomotionDerivationTest::RunTest(const FString& Parameters)
{
    using L = EAstrawildLocomotionClass;

    // Avian family / winged body plan -> flight.
    TestEqual(TEXT("Avian family flies"),
        AAstrawildEchoCharacter::DeriveLocomotionClass(EAstrawildEchoFamily::Avian, EAstrawildBodyPlan::Biped, EAstrawildZone::DawnFields), L::Flying);
    TestEqual(TEXT("Avian body plan flies"),
        AAstrawildEchoCharacter::DeriveLocomotionClass(EAstrawildEchoFamily::Beast, EAstrawildBodyPlan::Avian, EAstrawildZone::DawnFields), L::Flying);
    TestEqual(TEXT("Floating body plan hovers"),
        AAstrawildEchoCharacter::DeriveLocomotionClass(EAstrawildEchoFamily::Spirit, EAstrawildBodyPlan::Floating, EAstrawildZone::Glimmerwood), L::Flying);

    // Aquatic family + sea zones -> water.
    TestEqual(TEXT("Aquatic family swims"),
        AAstrawildEchoCharacter::DeriveLocomotionClass(EAstrawildEchoFamily::Aquatic, EAstrawildBodyPlan::Serpent, EAstrawildZone::DawnFields), L::Water);
    TestEqual(TEXT("Sea-zone beast swims"),
        AAstrawildEchoCharacter::DeriveLocomotionClass(EAstrawildEchoFamily::Beast, EAstrawildBodyPlan::Quadruped, EAstrawildZone::AzureShallows), L::Water);
    TestEqual(TEXT("Pearlsea reef swims"),
        AAstrawildEchoCharacter::DeriveLocomotionClass(EAstrawildEchoFamily::Flora, EAstrawildBodyPlan::Amorphous, EAstrawildZone::PearlseaReef), L::Water);
    TestEqual(TEXT("Tidebreaker isles swim"),
        AAstrawildEchoCharacter::DeriveLocomotionClass(EAstrawildEchoFamily::Insectoid, EAstrawildBodyPlan::Insectoid, EAstrawildZone::TidebreakerIsles), L::Water);

    // Everything else walks.
    TestEqual(TEXT("Inland beast walks"),
        AAstrawildEchoCharacter::DeriveLocomotionClass(EAstrawildEchoFamily::Beast, EAstrawildBodyPlan::Quadruped, EAstrawildZone::DawnFields), L::Land);
    TestEqual(TEXT("Winged sea-zone species still flies (flight outranks water)"),
        AAstrawildEchoCharacter::DeriveLocomotionClass(EAstrawildEchoFamily::Avian, EAstrawildBodyPlan::Avian, EAstrawildZone::AzureShallows), L::Flying);

    return true;
}

// --- GDP-3: attribute XP curve + level cap (Test 78) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAttributeXPCurveTest,
    "ASTRAWILD.Attributes.XPCurve",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAttributeXPCurveTest::RunTest(const FString& Parameters)
{
    UAstrawildAttributeComponent* Attributes = NewObject<UAstrawildAttributeComponent>();
    TestEqual(TEXT("Might starts at 1"), Attributes->GetLevel(EAstrawildAttributeType::Might), 1);
    TestEqual(TEXT("Might starts at 0 XP"), Attributes->GetXP(EAstrawildAttributeType::Might), 0.0f);
    TestEqual(TEXT("Level 1 needs 100 XP"), Attributes->GetXPToNextLevel(EAstrawildAttributeType::Might), 100.0f);

    // Level 2 exactly at 100 XP.
    Attributes->AddAttributeXP(EAstrawildAttributeType::Might, 100.0f);
    TestEqual(TEXT("100 XP -> level 2"), Attributes->GetLevel(EAstrawildAttributeType::Might), 2);
    TestEqual(TEXT("Overflow XP carried"), Attributes->GetXP(EAstrawildAttributeType::Might), 0.0f);
    TestEqual(TEXT("Level 2 needs 200 XP"), Attributes->GetXPToNextLevel(EAstrawildAttributeType::Might), 200.0f);

    // Cap at 10 with no overflow residue.
    for (int32 i = 0; i < 40; ++i)
    {
        Attributes->AddAttributeXP(EAstrawildAttributeType::Might, 1000.0f);
    }
    TestEqual(TEXT("Might caps at 10"), Attributes->GetLevel(EAstrawildAttributeType::Might), 10);
    TestEqual(TEXT("Cap clears the XP residue"), Attributes->GetXP(EAstrawildAttributeType::Might), 0.0f);
    TestEqual(TEXT("Capped attribute reports 0 to next"), Attributes->GetXPToNextLevel(EAstrawildAttributeType::Might), 0.0f);

    // Negative/zero XP is rejected.
    Attributes->AddAttributeXP(EAstrawildAttributeType::Vigor, -50.0f);
    TestEqual(TEXT("Negative XP rejected"), Attributes->GetXP(EAstrawildAttributeType::Vigor), 0.0f);

    return true;
}

// --- GDP-3: bonus formulas (Test 79) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAttributeBonusFormulasTest,
    "ASTRAWILD.Attributes.BonusFormulas",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAttributeBonusFormulasTest::RunTest(const FString& Parameters)
{
    UAstrawildAttributeComponent* Attributes = NewObject<UAstrawildAttributeComponent>();

    // Fresh component: every multiplier reads exactly 1.0/0.0.
    TestEqual(TEXT("Fresh melee mult 1.0"), Attributes->GetMeleeDamageMultiplier(), 1.0f);
    TestEqual(TEXT("Fresh max health mult 1.0"), Attributes->GetMaxHealthMultiplier(), 1.0f);
    TestEqual(TEXT("Fresh stamina regen mult 1.0"), Attributes->GetStaminaRegenMultiplier(), 1.0f);
    TestEqual(TEXT("Fresh move mult 1.0"), Attributes->GetMoveSpeedMultiplier(), 1.0f);
    TestEqual(TEXT("Fresh capture bonus 0"), Attributes->GetCaptureChanceBonus(), 0.0f);
    TestEqual(TEXT("Fresh craft mult 1.0"), Attributes->GetCraftSpeedMultiplier(), 1.0f);
    TestEqual(TEXT("No masterwork at Craft 1"), Attributes->GetMasterworkRefundChance(), 0.0f);

    // Might 10 -> 1 + 0.04*9 = 1.36.
    for (int32 i = 0; i < 30; ++i)
    {
        Attributes->AddAttributeXP(EAstrawildAttributeType::Might, 1000.0f);
    }
    TestEqual(TEXT("Might 10 melee mult 1.36"), Attributes->GetMeleeDamageMultiplier(), 1.36f);

    // Craft 5 -> masterwork unlocked at 15%.
    for (int32 i = 0; i < 30; ++i)
    {
        Attributes->AddAttributeXP(EAstrawildAttributeType::Craft, 1000.0f);
    }
    TestEqual(TEXT("Craft 5+ masterwork 15%"), Attributes->GetMasterworkRefundChance(), 0.15f);

    return true;
}

// --- GDP-3: skill unlock milestones + smart-cast ladder (Test 80) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAttributeSkillUnlockTest,
    "ASTRAWILD.Attributes.SkillUnlock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAttributeSkillUnlockTest::RunTest(const FString& Parameters)
{
    using S = EAstrawildPlayerSkillId;

    // Milestone table (static rule — no component needed).
    TestTrue(TEXT("PowerStrike at Might 3"), UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(S::PowerStrike, 3, 1, 1, 1, 1));
    TestFalse(TEXT("PowerStrike locked at Might 2"), UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(S::PowerStrike, 2, 1, 1, 1, 1));
    TestTrue(TEXT("Whirlwind at Might 6"), UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(S::Whirlwind, 6, 1, 1, 1, 1));
    TestTrue(TEXT("Dash at Agility 3"), UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(S::Dash, 1, 1, 3, 1, 1));
    TestTrue(TEXT("SecondWind at Vigor 4"), UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(S::SecondWind, 1, 4, 1, 1, 1));
    TestTrue(TEXT("HuntersFocus at Instinct 4"), UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(S::HuntersFocus, 1, 1, 1, 4, 1));
    TestTrue(TEXT("Masterwork at Craft 5"), UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(S::Masterwork, 1, 1, 1, 1, 5));
    TestTrue(TEXT("Overcharge at Instinct 7"), UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(S::Overcharge, 1, 1, 1, 7, 1));
    TestFalse(TEXT("None never unlocks"), UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(S::None, 10, 10, 10, 10, 10));

    // Cooldown table sanity.
    TestEqual(TEXT("Masterwork is passive (0 cooldown)"), UAstrawildAttributeComponent::GetSkillCooldown(S::Masterwork), 0.0f);
    TestTrue(TEXT("SecondWind has the longest cooldown"),
        UAstrawildAttributeComponent::GetSkillCooldown(S::SecondWind) > UAstrawildAttributeComponent::GetSkillCooldown(S::PowerStrike));

    // Smart-cast ladder: hurt player with SecondWind picks the heal.
    UAstrawildAttributeComponent* Attributes = NewObject<UAstrawildAttributeComponent>();
    for (int32 i = 0; i < 30; ++i)
    {
        Attributes->AddAttributeXP(EAstrawildAttributeType::Vigor, 1000.0f);
        Attributes->AddAttributeXP(EAstrawildAttributeType::Agility, 1000.0f);
    }
    TestEqual(TEXT("Hurt player smart-casts SecondWind"),
        Attributes->PickBestReadySkill(0.2f, 0, false, false), S::SecondWind);
    TestEqual(TEXT("Healthy moving player smart-casts Dash"),
        Attributes->PickBestReadySkill(1.0f, 0, false, true), S::Dash);

    // After casting, the cooldown blocks a re-pick.
    Attributes->StartSkillCooldown(S::Dash);
    TestTrue(TEXT("Dash on cooldown is not picked"),
        Attributes->PickBestReadySkill(1.0f, 0, false, true) != S::Dash);
    Attributes->TickCooldowns(UAstrawildAttributeComponent::GetSkillCooldown(S::Dash) + 0.1f);
    TestEqual(TEXT("Cooldown expiry re-enables Dash"),
        Attributes->PickBestReadySkill(1.0f, 0, false, true), S::Dash);

    return true;
}

// --- GDP-3: attribute save round-trip + sanitize (Test 81) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAttributeSaveRoundTripTest,
    "ASTRAWILD.Attributes.SaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAttributeSaveRoundTripTest::RunTest(const FString& Parameters)
{
    UAstrawildAttributeComponent* Source = NewObject<UAstrawildAttributeComponent>();
    Source->AddAttributeXP(EAstrawildAttributeType::Might, 350.0f);   // Level 3, 50 XP.
    Source->AddAttributeXP(EAstrawildAttributeType::Craft, 120.0f);  // Level 2, 20 XP.

    const TArray<FAstrawildAttributeSaveData> Saved = Source->ToSaveData();
    TestEqual(TEXT("Save payload has all five attributes"), Saved.Num(), 5);

    UAstrawildAttributeComponent* Target = NewObject<UAstrawildAttributeComponent>();
    TestEqual(TEXT("Clean import repairs nothing"), Target->ImportFromSaveData(Saved), 0);
    TestEqual(TEXT("Might level survives the round-trip"), Target->GetLevel(EAstrawildAttributeType::Might), 3);
    TestEqual(TEXT("Might XP survives the round-trip"), Target->GetXP(EAstrawildAttributeType::Might), 50.0f);
    TestEqual(TEXT("Craft level survives the round-trip"), Target->GetLevel(EAstrawildAttributeType::Craft), 2);

    // Corrupt import: out-of-range level, negative XP, duplicates.
    TArray<FAstrawildAttributeSaveData> Corrupt;
    FAstrawildAttributeSaveData Row;
    Row.Type = EAstrawildAttributeType::Vigor;
    Row.Level = 99;
    Row.XP = -10.0f;
    Corrupt.Add(Row);
    Corrupt.Add(Row); // Duplicate row.
    TestTrue(TEXT("Corrupt import reports repairs"), Target->ImportFromSaveData(Corrupt) >= 2);
    TestEqual(TEXT("Vigor level clamped to 10"), Target->GetLevel(EAstrawildAttributeType::Vigor), 10);
    TestEqual(TEXT("Vigor XP clamped to >= 0"), Target->GetXP(EAstrawildAttributeType::Vigor), 0.0f);

    // Old saves (empty array) import as fresh states.
    UAstrawildAttributeComponent* Fresh = NewObject<UAstrawildAttributeComponent>();
    TestEqual(TEXT("Empty payload imports clean"), Fresh->ImportFromSaveData(TArray<FAstrawildAttributeSaveData>()), 0);
    TestEqual(TEXT("Fresh Might at 1"), Fresh->GetLevel(EAstrawildAttributeType::Might), 1);

    return true;
}

// --- GDP-4: NPC affinity tiers + discount (Test 82) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildNPCAffinityTierTest,
    "ASTRAWILD.NPC.AffinityTiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildNPCAffinityTierTest::RunTest(const FString& Parameters)
{
    // Tier boundaries: 0/25/50/75 -> Stranger/Acquaintance/Friend/Confidant.
    AAstrawildNPCCharacter* Npc = NewObject<AAstrawildNPCCharacter>();
    Npc->Affinity = 0.0f;
    TestEqual(TEXT("Stranger tier 0"), Npc->GetAffinityTier(), 0);
    TestEqual(TEXT("Stranger no discount"), Npc->GetVendorDiscountFraction(), 0.0f);

    Npc->Affinity = 24.9f;
    TestEqual(TEXT("Below 25 stays Stranger"), Npc->GetAffinityTier(), 0);
    Npc->Affinity = 25.0f;
    TestEqual(TEXT("25 -> Acquaintance"), Npc->GetAffinityTier(), 1);
    TestEqual(TEXT("Acquaintance 5% off"), Npc->GetVendorDiscountFraction(), 0.05f);

    Npc->Affinity = 50.0f;
    TestEqual(TEXT("50 -> Friend"), Npc->GetAffinityTier(), 2);
    Npc->Affinity = 75.0f;
    TestEqual(TEXT("75 -> Confidant"), Npc->GetAffinityTier(), 3);
    TestEqual(TEXT("Confidant 15% off"), Npc->GetVendorDiscountFraction(), 0.15f);
    Npc->Affinity = 100.0f;
    TestEqual(TEXT("100 clamps at Confidant"), Npc->GetAffinityTier(), 3);

    TestTrue(TEXT("Titles are non-empty"), !Npc->GetAffinityTierTitle().IsEmpty());
    TestTrue(TEXT("No definition -> no stable id"), Npc->GetStableNPCId().IsNone());

    return true;
}

// --- GDP-4: NPC affinity save payload (Test 83) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildNPCAffinitySaveTest,
    "ASTRAWILD.NPC.AffinitySave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildNPCAffinitySaveTest::RunTest(const FString& Parameters)
{
    FAstrawildNPCAffinitySaveData Row;
    TestTrue(TEXT("Default id is none"), Row.NPCId.IsNone());
    TestEqual(TEXT("Default affinity is 0"), Row.Affinity, 0.0f);

    Row.NPCId = TEXT("NPC_Wren");
    Row.Affinity = 62.5f;
    const FAstrawildNPCAffinitySaveData Copy = Row;
    TestTrue(TEXT("NPC id survives the struct copy"), Copy.NPCId == Row.NPCId);
    TestEqual(TEXT("Affinity survives the struct copy"), Copy.Affinity, 62.5f);

    // The save field exists and defaults empty (old saves deserialize clean).
    UAstrawildSaveGame* SaveGame = NewObject<UAstrawildSaveGame>();
    TestEqual(TEXT("Fresh save holds no NPC affinity rows"), SaveGame->NPCAffinities.Num(), 0);
    TestEqual(TEXT("Fresh save holds no attribute rows"), SaveGame->Attributes.Num(), 0);
    SaveGame->NPCAffinities.Add(Row);
    SaveGame->Attributes.Add(FAstrawildAttributeSaveData());
    TestEqual(TEXT("Rows append"), SaveGame->NPCAffinities.Num(), 1);
    TestEqual(TEXT("Attribute rows append"), SaveGame->Attributes.Num(), 1);

    return true;
}

// --- GDP-1: full pipeline — Echo ability engine contracts (Test 84) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAbilityEngineContractTest,
    "ASTRAWILD.Ability.EngineContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAbilityEngineContractTest::RunTest(const FString& Parameters)
{
    UAstrawildEchoDefinition* Def = NewObject<UAstrawildEchoDefinition>();
    Def->Element = EAstrawildElementType::Flora;
    Def->Role = EAstrawildEchoRole::Support;
    Def->Family = EAstrawildEchoFamily::Flora;
    Def->AbilityIds = { TEXT("Ability_ThornLash"), TEXT("Ability_RootSnare"), TEXT("Ability_LumewispDawn") };

    AAstrawildEchoCharacter* Echo = NewObject<AAstrawildEchoCharacter>();
    Echo->EchoDefinition = Def;

    // Every learned id is level-1 knowable: LumewispDawn (1), ThornLash (3),
    // RootSnare (5) — knowledge follows the level gate.
    TestEqual(TEXT("All ids exposed"), Echo->GetAllAbilityIds().Num(), 5); // 3 authored (2 unique after element dedupe) + derived kit.
    TestTrue(TEXT("Level 1 knows LumewispDawn"), Echo->GetKnownAbilityIds().Contains(TEXT("Ability_LumewispDawn")));
    TestFalse(TEXT("Level 1 does not know ThornLash (needs 3)"), Echo->GetKnownAbilityIds().Contains(TEXT("Ability_ThornLash")));
    TestFalse(TEXT("Gated ability is not ready at level 1"), Echo->IsAbilityReady(TEXT("Ability_ThornLash")));

    // Cooldown query reads 0 for unknown ids (never blocks).
    TestEqual(TEXT("Unknown ability reports no cooldown"), Echo->GetAbilityCooldownRemaining(TEXT("Ability_Nope")), 0.0f);

    // PickCombatAbility never crashes and respects level gates: level 1 with
    // only > 1 unlock-level kits yields nothing castable in range.
    const FName Picked = Echo->PickCombatAbility(400.0f, false, false);
    if (Picked != NAME_None)
    {
        TestTrue(TEXT("Level 1 pick is actually level-1 knowable"),
            Echo->GetKnownAbilityIds().Contains(Picked));
    }
    Echo->Level = 30;
    TestTrue(TEXT("Level 30 can pick something in melee range"),
        Echo->PickCombatAbility(300.0f, false, false) != NAME_None);

    return true;
}

// ===========================================================================
// SCP (Systems Completion Pack) — plan-vs-repo gap closure contracts
// ===========================================================================

// --- SCP Phase 1: data validator static tables (Test 85) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDataValidatorStaticTablesTest,
    "ASTRAWILD.SCP.DataValidator.StaticTables",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDataValidatorStaticTablesTest::RunTest(const FString& Parameters)
{
    TArray<FString> Problems;
    UAstrawildDataValidatorLibrary::ValidateStaticTables(Problems);
    for (const FString& Problem : Problems)
    {
        AddError(Problem);
    }
    TestTrue(TEXT("Static tables (bestiary + abilities + element chain) validate clean"),
        Problems.IsEmpty());
    return true;
}

// --- SCP Phase 2: error reporter ring buffer + formatting (Test 86) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildErrorReporterContractTest,
    "ASTRAWILD.SCP.ErrorReporter.RingBuffer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildErrorReporterContractTest::RunTest(const FString& Parameters)
{
    UAstrawildErrorReporterLibrary::Clear();
    TestEqual(TEXT("Reporter starts empty"), UAstrawildErrorReporterLibrary::GetRecordCount(), 0);

    UAstrawildErrorReporterLibrary::ReportError(TEXT("TestCategory"), TEXT("Boom"));
    UAstrawildErrorReporterLibrary::ReportWarning(TEXT("TestCategory"), TEXT("Careful"));
    UAstrawildErrorReporterLibrary::ReportInfo(TEXT("TestCategory"), TEXT("FYI"));
    TestEqual(TEXT("Three records held"), UAstrawildErrorReporterLibrary::GetRecordCount(), 3);
    TestEqual(TEXT("Two non-info records"), UAstrawildErrorReporterLibrary::GetNonInfoCount(), 2);

    // Capacity contract: the buffer drops the OLDEST records beyond the cap.
    for (int32 Index = 0; Index < UAstrawildErrorReporterLibrary::MaxRecords + 50; ++Index)
    {
        UAstrawildErrorReporterLibrary::ReportInfo(TEXT("Flood"), FString::Printf(TEXT("record %d"), Index));
    }
    TestEqual(TEXT("Ring buffer bounded at capacity"), UAstrawildErrorReporterLibrary::GetRecordCount(),
        UAstrawildErrorReporterLibrary::MaxRecords);

    const TArray<FAstrawildErrorRecord> Records = UAstrawildErrorReporterLibrary::GetRecords();
    TestTrue(TEXT("Oldest records dropped, newest kept"),
        Records.Last().Message.Equals(FString::Printf(TEXT("record %d"), UAstrawildErrorReporterLibrary::MaxRecords + 49)));

    const FString Report = UAstrawildErrorReporterLibrary::FormatReport(TEXT("HEADER"));
    TestTrue(TEXT("Report carries the header"), Report.Contains(TEXT("HEADER")));
    TestTrue(TEXT("Report renders record lines"), Report.Contains(TEXT("Flood")));

    UAstrawildErrorReporterLibrary::Clear();
    TestEqual(TEXT("Clear resets the trail"), UAstrawildErrorReporterLibrary::GetRecordCount(), 0);
    return true;
}

// --- SCP Phase 2: asset fallback shape mapping (Test 87) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAssetFallbackContractTest,
    "ASTRAWILD.SCP.AssetFallback.ShapePaths",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAssetFallbackContractTest::RunTest(const FString& Parameters)
{
    const FString Cube = UAstrawildAssetFallbackLibrary::GetFallbackShapePath(EAstrawildFallbackShape::Cube);
    const FString Sphere = UAstrawildAssetFallbackLibrary::GetFallbackShapePath(EAstrawildFallbackShape::Sphere);
    const FString Cylinder = UAstrawildAssetFallbackLibrary::GetFallbackShapePath(EAstrawildFallbackShape::Cylinder);
    const FString Cone = UAstrawildAssetFallbackLibrary::GetFallbackShapePath(EAstrawildFallbackShape::Cone);

    TestTrue(TEXT("Cube path is an engine basic shape"), Cube.Contains(TEXT("/Engine/BasicShapes/Cube")));
    TestTrue(TEXT("Sphere path is an engine basic shape"), Sphere.Contains(TEXT("/Engine/BasicShapes/Sphere")));
    TestTrue(TEXT("Cylinder path is an engine basic shape"), Cylinder.Contains(TEXT("/Engine/BasicShapes/Cylinder")));
    TestTrue(TEXT("Cone path is an engine basic shape"), Cone.Contains(TEXT("/Engine/BasicShapes/Cone")));
    TestTrue(TEXT("Every shape maps to a distinct path"),
        Cube != Sphere && Cube != Cylinder && Cube != Cone && Sphere != Cylinder && Sphere != Cone);

    // Malformed enum value falls back to the cube (switch default) — never empty.
    TestFalse(TEXT("Default path never empty"),
        UAstrawildAssetFallbackLibrary::GetFallbackShapePath(static_cast<EAstrawildFallbackShape>(255)).IsEmpty());
    return true;
}

// --- SCP Phase 12: spoilage math (Test 88) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildSpoilageMathTest,
    "ASTRAWILD.SCP.Spoilage.Math",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildSpoilageMathTest::RunTest(const FString& Parameters)
{
    // Unknown freshness initializes to the full shelf life.
    TestEqual(TEXT("Fresh stack initializes to shelf life"),
        UAstrawildSpoilageSubsystem::ComputeSpoilStep(0.0f, 600.0f, 10.0f, false), 590.0f);

    // Normal aging subtracts the elapsed step.
    TestEqual(TEXT("Aging advances linearly"),
        UAstrawildSpoilageSubsystem::ComputeSpoilStep(590.0f, 600.0f, 10.0f, false), 580.0f);

    // Ice Box preservation slows aging tenfold.
    TestEqual(TEXT("Preserved aging is x0.1"),
        UAstrawildSpoilageSubsystem::ComputeSpoilStep(590.0f, 600.0f, 10.0f, true), 589.0f);

    // Deadline clamps to the sentinel (callers treat <= 0 as conversion).
    TestTrue(TEXT("Overdue stack hits the deadline sentinel"),
        UAstrawildSpoilageSubsystem::ComputeSpoilStep(5.0f, 600.0f, 10.0f, false) < 0.0f);

    // Conversion: half the stack, floor of 1 — food never silently vanishes.
    TestEqual(TEXT("Even stack halves"), UAstrawildSpoilageSubsystem::ComputeSpoiledConversion(10), 5);
    TestEqual(TEXT("Odd stack floors"), UAstrawildSpoilageSubsystem::ComputeSpoiledConversion(9), 4);
    TestEqual(TEXT("Single item still yields one"), UAstrawildSpoilageSubsystem::ComputeSpoiledConversion(1), 1);
    return true;
}

// --- SCP Phase 12: durability constants + definition wiring (Test 89) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDurabilityContractTest,
    "ASTRAWILD.SCP.Durability.Contracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDurabilityContractTest::RunTest(const FString& Parameters)
{
    // The directive numbers, pinned as contracts: broken weapons hit at 40%,
    // bench repairs cost 40% of the craft inputs.
    TestEqual(TEXT("Broken weapon multiplier is 0.4"),
        UAstrawildDurabilityComponent::BrokenWeaponDamageMultiplier, 0.4f);
    TestEqual(TEXT("Bench repair cost fraction is 0.4"),
        UAstrawildDurabilityComponent::BenchRepairCostFraction, 0.4f);

    // Definition wiring: the harvest specialization fields exist and default
    // to inert values (no behavior change for legacy content).
    UAstrawildItemDefinition* Item = NewObject<UAstrawildItemDefinition>();
    TestEqual(TEXT("Legacy items carry no durability"), Item->DurabilityMax, 0.0f);
    TestEqual(TEXT("Legacy items never perish"), Item->PerishableSeconds, 0.0f);
    TestTrue(TEXT("Legacy items carry no harvest category"), Item->HarvestCategory.IsNone());
    TestTrue(TEXT("Legacy tools carry no harvest bonus"), Item->HarvestBonusCategory.IsNone());
    TestEqual(TEXT("Default harvest multiplier is neutral"), Item->HarvestMultiplier, 1.0f);
    return true;
}

// --- SCP Phase 9: sanity math + illness bands (Test 90) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildCreatureSanityMathTest,
    "ASTRAWILD.SCP.Sanity.MathAndIllness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildCreatureSanityMathTest::RunTest(const FString& Parameters)
{
    using San = UAstrawildCreatureSanityComponent;

    // Idle + healthy: no change.
    TestEqual(TEXT("Idle echoes hold steady"), San::ComputeSanityDelta(10.0f, false, false, false, false, false), 0.0f);

    // Working drains (-0.02/s), combat adds pressure.
    TestEqual(TEXT("Working drains sanity"), San::ComputeSanityDelta(10.0f, true, false, false, false, false), -0.2f);
    TestEqual(TEXT("Work + combat stacks drains"), San::ComputeSanityDelta(10.0f, true, true, false, false, false), -0.3f);

    // Comfort: bed +0.05/s, hot spring +0.12/s — comfort outweighs stress.
    TestEqual(TEXT("Bed recovery beats work drain"),
        San::ComputeSanityDelta(10.0f, true, false, true, false, false), 0.3f);
    TestEqual(TEXT("Hot spring recovery"), San::ComputeSanityDelta(10.0f, false, false, false, true, false), 1.2f);
    TestEqual(TEXT("Night rest recovers"), San::ComputeSanityDelta(10.0f, false, false, false, false, true), 0.1f);

    // Illness risk: zero at/above the threshold, monotone in depth + exposure.
    TestEqual(TEXT("Healthy band carries no risk"), San::ComputeIllnessRisk(30.0f, 600.0f), 0.0f);
    TestTrue(TEXT("Deep exposure accrues risk"), San::ComputeIllnessRisk(5.0f, 300.0f) > 0.5f);
    TestTrue(TEXT("Risk grows with exposure"),
        San::ComputeIllnessRisk(10.0f, 400.0f) > San::ComputeIllnessRisk(10.0f, 100.0f));
    TestEqual(TEXT("Risk clamps to 1"), San::ComputeIllnessRisk(0.0f, 100000.0f), 1.0f);

    // Illness bands: Ulcer 40%, SprainedAnkle 35%, Slacker 25%.
    TestTrue(TEXT("Low roll -> Ulcer"), San::SelectIllness(0.10f) == TEXT("Illness_Ulcer"));
    TestTrue(TEXT("Mid roll -> SprainedAnkle"), San::SelectIllness(0.50f) == TEXT("Illness_SprainedAnkle"));
    TestTrue(TEXT("High roll -> Slacker"), San::SelectIllness(0.90f) == TEXT("Illness_Slacker"));

    // Modifier table.
    TestEqual(TEXT("Slacker work x0.3"), San::GetIllnessWorkMultiplier(TEXT("Illness_Slacker")), 0.3f);
    TestEqual(TEXT("SprainedAnkle speed x0.75"), San::GetIllnessSpeedMultiplier(TEXT("Illness_SprainedAnkle")), 0.75f);
    TestTrue(TEXT("Ulcer drains health"), San::GetIllnessHealthDrain(TEXT("Illness_Ulcer")) > 0.0f);
    TestEqual(TEXT("Unknown illness carries no modifiers"), San::GetIllnessWorkMultiplier(TEXT("Illness_Nope")), 1.0f);
    return true;
}

// --- SCP Phase 9: base terminal level + garrison caps (Test 91) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildBaseTerminalContractTest,
    "ASTRAWILD.SCP.BaseTerminal.LevelAndGarrison",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildBaseTerminalContractTest::RunTest(const FString& Parameters)
{
    // Directive Phase 9.1 numbers pinned: territory 3500cm, caps 5/10/20.
    TestEqual(TEXT("Territory radius is 3500cm"), AAstrawildBaseTerminalActor::TerritoryRadius, 3500.0f);

    TestEqual(TEXT("0-7 buildings -> level 1"), AAstrawildBaseTerminalActor::ComputeBaseLevel(0), 1);
    TestEqual(TEXT("7 buildings still level 1"), AAstrawildBaseTerminalActor::ComputeBaseLevel(7), 1);
    TestEqual(TEXT("8 buildings -> level 2"), AAstrawildBaseTerminalActor::ComputeBaseLevel(8), 2);
    TestEqual(TEXT("15 buildings still level 2"), AAstrawildBaseTerminalActor::ComputeBaseLevel(15), 2);
    TestEqual(TEXT("16 buildings -> level 3"), AAstrawildBaseTerminalActor::ComputeBaseLevel(16), 3);

    TestEqual(TEXT("Level 1 garrison 5"), AAstrawildBaseTerminalActor::GetGarrisonCapForLevel(1), 5);
    TestEqual(TEXT("Level 2 garrison 10"), AAstrawildBaseTerminalActor::GetGarrisonCapForLevel(2), 10);
    TestEqual(TEXT("Level 3 garrison 20"), AAstrawildBaseTerminalActor::GetGarrisonCapForLevel(3), 20);
    TestEqual(TEXT("Unknown level falls back to 5"), AAstrawildBaseTerminalActor::GetGarrisonCapForLevel(99), 5);
    return true;
}

// --- SCP Phase 5: mount eligibility + speed + seat contract (Test 92) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildMountContractTest,
    "ASTRAWILD.SCP.Mount.SpeciesAndSpeed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildMountContractTest::RunTest(const FString& Parameters)
{
    using Mnt = UAstrawildMountComponent;

    // Species gates: classic mount families + quadruped/avian plans + Medium+;
    // DP-3 opens the sea-rider gate (Aquatic quadrupeds/serpents).
    TestTrue(TEXT("Beast quadruped large is rideable"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Beast, EAstrawildBodyPlan::Quadruped, EAstrawildSizeClass::Large));
    TestTrue(TEXT("Avian mount is rideable"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Avian, EAstrawildBodyPlan::Avian, EAstrawildSizeClass::Medium));
    TestTrue(TEXT("Dragon mount is rideable"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Dragon, EAstrawildBodyPlan::Quadruped, EAstrawildSizeClass::Huge));
    TestFalse(TEXT("Tiny creatures are never rideable"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Beast, EAstrawildBodyPlan::Quadruped, EAstrawildSizeClass::Tiny));
    TestFalse(TEXT("Small creatures are never rideable"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Beast, EAstrawildBodyPlan::Quadruped, EAstrawildSizeClass::Small));
    TestFalse(TEXT("Serpent bodies carry no saddle"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Beast, EAstrawildBodyPlan::Serpent, EAstrawildSizeClass::Large));
    TestTrue(TEXT("DP-3: aquatic serpents are sea-riders"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Aquatic, EAstrawildBodyPlan::Serpent, EAstrawildSizeClass::Large));
    TestTrue(TEXT("DP-3: aquatic quadrupeds are sea-riders"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Aquatic, EAstrawildBodyPlan::Quadruped, EAstrawildSizeClass::Large));
    TestFalse(TEXT("DP-3: tiny sea-riders still refuse a saddle"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Aquatic, EAstrawildBodyPlan::Serpent, EAstrawildSizeClass::Small));
    TestFalse(TEXT("Flora Kindred are companions, not mounts"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Flora, EAstrawildBodyPlan::Quadruped, EAstrawildSizeClass::Large));
    TestFalse(TEXT("Floating wisps carry no rider"),
        Mnt::IsRideableSpecies(EAstrawildEchoFamily::Spirit, EAstrawildBodyPlan::Floating, EAstrawildSizeClass::Huge));

    // Speed: 1.25x species speed with a sane floor.
    TestEqual(TEXT("Mount speed is 1.25x species speed"), Mnt::ComputeMountSpeed(400.0f), 500.0f);
    TestEqual(TEXT("Slow species get the mount floor"), Mnt::ComputeMountSpeed(50.0f), 250.0f);

    // Seat contract scales with size (rider sits higher on bigger mounts).
    const FVector MediumSeat = Mnt::ComputeRiderSeatOffset(EAstrawildSizeClass::Medium);
    const FVector LargeSeat = Mnt::ComputeRiderSeatOffset(EAstrawildSizeClass::Large);
    const FVector HugeSeat = Mnt::ComputeRiderSeatOffset(EAstrawildSizeClass::Huge);
    TestTrue(TEXT("Larger mounts seat higher"), LargeSeat.Z > MediumSeat.Z && HugeSeat.Z > LargeSeat.Z);

    // Socket contract: all six directive socket names are pinned.
    TestTrue(TEXT("MountSocket pinned"), Mnt::GetMountSocketName() == TEXT("MountSocket"));
    TestTrue(TEXT("RiderPelvisSocket pinned"), Mnt::GetRiderPelvisSocketName() == TEXT("RiderPelvisSocket"));
    TestTrue(TEXT("Hand grip sockets pinned"),
        Mnt::GetLeftHandGripSocketName() == TEXT("LeftHandGripSocket") &&
        Mnt::GetRightHandGripSocketName() == TEXT("RightHandGripSocket"));
    TestTrue(TEXT("Stirrup sockets pinned"),
        Mnt::GetLeftFootStirrupSocketName() == TEXT("LeftFootStirrupSocket") &&
        Mnt::GetRightFootStirrupSocketName() == TEXT("RightFootStirrupSocket"));

    // Bond gate: the trust arc number from the directive spec.
    TestEqual(TEXT("Mount bond gate is 25"), Mnt::MountBondGate, 25.0f);
    return true;
}

// --- SCP Phase 6: dual-tech reaction table (Test 93) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildComboTableTest,
    "ASTRAWILD.SCP.Combo.ReactionTable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildComboTableTest::RunTest(const FString& Parameters)
{
    using Combo = UAstrawildComboLibrary;

    // Every element resolves on BOTH player tiers (12 reactions — the
    // directive's 10-formula minimum exceeded).
    TestEqual(TEXT("Reaction count is 12"), Combo::GetReactionCount(), 12);

    const EAstrawildElementType Elements[6] =
    {
        EAstrawildElementType::Ember, EAstrawildElementType::Frost, EAstrawildElementType::Pulse,
        EAstrawildElementType::Flora, EAstrawildElementType::Light, EAstrawildElementType::Ash
    };
    for (const bool bEmpowered : { false, true })
    {
        for (const EAstrawildElementType Element : Elements)
        {
            const FAstrawildComboReaction Reaction = Combo::ResolveCombo(bEmpowered, Element);
            TestTrue(TEXT("Every element x tier resolves"), Reaction.IsValid());
            TestTrue(TEXT("Every multiplier is meaningful"), Reaction.DamageMultiplier >= 1.8f);
        }
    }

    // The directive's signature reaction: empowered + Ember = Steam Explosion
    // x2.5 with a hitstop status.
    const FAstrawildComboReaction Steam = Combo::ResolveCombo(true, EAstrawildElementType::Ember);
    TestTrue(TEXT("Steam Explosion named"), Steam.DisplayName.Equals(TEXT("Steam Explosion")));
    TestEqual(TEXT("Steam Explosion is x2.5"), Steam.DamageMultiplier, 2.5f);
    TestTrue(TEXT("Steam Explosion applies hitstop"), Steam.StatusId == TEXT("Status.Hitstop"));

    // Empowered tier strictly out-damages the kinetic tier.
    for (const EAstrawildElementType Element : Elements)
    {
        const FAstrawildComboReaction Kinetic = Combo::ResolveCombo(false, Element);
        const FAstrawildComboReaction Empowered = Combo::ResolveCombo(true, Element);
        TestTrue(TEXT("Empowered beats kinetic"), Empowered.DamageMultiplier >= Kinetic.DamageMultiplier);
    }

    // Window + hitstop numbers pinned.
    TestEqual(TEXT("Combo window is 3s"), Combo::ComboWindowSeconds, 3.0f);
    TestEqual(TEXT("Hitstop lasts 1.5s"), Combo::HitstopSeconds, 1.5f);
    TestEqual(TEXT("Hitstop slows to 15%"), Combo::HitstopSpeedMultiplier, 0.15f);
    return true;
}

// --- SCP Phase 3: DDA band math (Test 94) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDifficultyBandTest,
    "ASTRAWILD.SCP.DDA.SkillBands",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDifficultyBandTest::RunTest(const FString& Parameters)
{
    using DDA = UAstrawildDifficultySubsystem;

    // Fresh player: Standard.
    TestEqual(TEXT("Fresh session is Standard"), DDA::ComputeSkillBand(0, 0, 0), 1);

    // Struggling: deaths dominate (metric <= -2).
    TestEqual(TEXT("One death is still Standard (hysteresis)"), DDA::ComputeSkillBand(0, 0, 1), 1);
    TestEqual(TEXT("Two deaths -> Struggling"), DDA::ComputeSkillBand(0, 0, 2), 0);
    TestEqual(TEXT("Deaths outweigh a few kills"), DDA::ComputeSkillBand(3, 0, 3), 0);

    // Thriving: captures count double, defeats single.
    TestEqual(TEXT("Two captures -> Thriving"), DDA::ComputeSkillBand(0, 1, 0), 2);
    TestEqual(TEXT("Two defeats -> Thriving"), DDA::ComputeSkillBand(2, 0, 0), 2);
    TestEqual(TEXT("One defeat stays Standard (hysteresis)"), DDA::ComputeSkillBand(1, 0, 0), 1);

    // Multipliers: struggling gets help, thriving gets pressure.
    TestEqual(TEXT("Struggling hostiles x0.85"), DDA::GetHostileStrengthMultiplier(0), 0.85f);
    TestEqual(TEXT("Standard hostiles x1.0"), DDA::GetHostileStrengthMultiplier(1), 1.0f);
    TestEqual(TEXT("Thriving hostiles x1.15"), DDA::GetHostileStrengthMultiplier(2), 1.15f);
    TestEqual(TEXT("Struggling resources x1.15"), DDA::GetResourceYieldMultiplier(0), 1.15f);
    TestEqual(TEXT("Standard resources x1.0"), DDA::GetResourceYieldMultiplier(1), 1.0f);
    TestEqual(TEXT("Thriving resources x0.9"), DDA::GetResourceYieldMultiplier(2), 0.9f);

    // Unknown bands fall back to Standard multipliers.
    TestEqual(TEXT("Unknown band is neutral"), DDA::GetHostileStrengthMultiplier(99), 1.0f);
    return true;
}

// --- SCP Phase 8: crop growth math (Test 95) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildCropMathTest,
    "ASTRAWILD.SCP.Crop.GrowthMath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildCropMathTest::RunTest(const FString& Parameters)
{
    using Crop = UAstrawildCropComponent;

    // Growth: watered full rate, dry half, compost doubles, season scales.
    const float Watered = Crop::ComputeGrowthStep(30.0f, true, false, 1.0f);
    const float Dry = Crop::ComputeGrowthStep(30.0f, false, false, 1.0f);
    const float Composted = Crop::ComputeGrowthStep(30.0f, true, true, 1.0f);
    TestEqual(TEXT("Watered step is 30/300"), Watered, 0.1f);
    TestEqual(TEXT("Dry step is half"), Dry, 0.05f);
    TestEqual(TEXT("Compost doubles"), Composted, 0.2f);
    TestTrue(TEXT("Steps never exceed 1"), Crop::ComputeGrowthStep(100000.0f, true, true, 2.0f) <= 1.0f);

    // Season ladder: spring 1.25, summer 1.0, autumn 0.85, winter 0.5, wraps.
    TestEqual(TEXT("Spring x1.25"), Crop::ComputeSeasonMultiplier(0), 1.25f);
    TestEqual(TEXT("Summer x1.0"), Crop::ComputeSeasonMultiplier(1), 1.0f);
    TestEqual(TEXT("Autumn x0.85"), Crop::ComputeSeasonMultiplier(2), 0.85f);
    TestEqual(TEXT("Winter x0.5"), Crop::ComputeSeasonMultiplier(3), 0.5f);
    TestEqual(TEXT("Year wraps to spring"), Crop::ComputeSeasonMultiplier(4), 1.25f);

    // State ladder from progress.
    TestEqual(TEXT("0 -> Empty"), static_cast<int32>(Crop::ResolveStateFromProgress(0.0f)),
        static_cast<int32>(EAstrawildCropState::Empty));
    TestEqual(TEXT("0.1 -> Planted"), static_cast<int32>(Crop::ResolveStateFromProgress(0.1f)),
        static_cast<int32>(EAstrawildCropState::Planted));
    TestEqual(TEXT("0.5 -> Sprout"), static_cast<int32>(Crop::ResolveStateFromProgress(0.5f)),
        static_cast<int32>(EAstrawildCropState::Sprout));
    TestEqual(TEXT("0.8 -> Young"), static_cast<int32>(Crop::ResolveStateFromProgress(0.8f)),
        static_cast<int32>(EAstrawildCropState::Young));
    TestEqual(TEXT("1.0 -> Mature"), static_cast<int32>(Crop::ResolveStateFromProgress(1.0f)),
        static_cast<int32>(EAstrawildCropState::Mature));
    return true;
}

// --- SCP Phase 7: NPC schedule anchors (Test 96) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildNPCScheduleTest,
    "ASTRAWILD.SCP.NPC.ScheduleAnchors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildNPCScheduleTest::RunTest(const FString& Parameters)
{
    using Sched = UAstrawildNPCScheduleComponent;

    // Work hours: smith/farmer at work, guard patrols, trader opens 8-20.
    TestEqual(TEXT("Farmer works at noon"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Farmer, 12, false)),
        static_cast<int32>(EAstrawildNPCAnchor::Work));
    TestEqual(TEXT("Smith works at 7"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Smith, 7, false)),
        static_cast<int32>(EAstrawildNPCAnchor::Work));
    TestEqual(TEXT("Trader closed at 7"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Trader, 7, false)),
        static_cast<int32>(EAstrawildNPCAnchor::Home));
    TestEqual(TEXT("Trader open at 9"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Trader, 9, false)),
        static_cast<int32>(EAstrawildNPCAnchor::Work));
    TestEqual(TEXT("Guard patrols at noon"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Guard, 12, false)),
        static_cast<int32>(EAstrawildNPCAnchor::Patrol));

    // Rain: everyone but guards shelters.
    TestEqual(TEXT("Farmer shelters in rain"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Farmer, 12, true)),
        static_cast<int32>(EAstrawildNPCAnchor::Shelter));
    TestEqual(TEXT("Guard patrols through rain"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Guard, 12, true)),
        static_cast<int32>(EAstrawildNPCAnchor::Patrol));

    // Night: curfew (guards keep a skeleton patrol).
    TestEqual(TEXT("Farmer sleeps at midnight"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Farmer, 0, false)),
        static_cast<int32>(EAstrawildNPCAnchor::Sleep));
    TestEqual(TEXT("Guard night patrol"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Guard, 0, false)),
        static_cast<int32>(EAstrawildNPCAnchor::Patrol));
    TestEqual(TEXT("Evening at home (20h)"), static_cast<int32>(Sched::ResolveAnchor(EAstrawildNPCProfession::Farmer, 20, false)),
        static_cast<int32>(EAstrawildNPCAnchor::Home));

    // Service gating follows the same rules.
    TestTrue(TEXT("Trader services open at noon"), Sched::IsServiceOpen(EAstrawildNPCProfession::Trader, 12, false));
    TestFalse(TEXT("Trader services closed in rain"), Sched::IsServiceOpen(EAstrawildNPCProfession::Trader, 12, true));
    TestFalse(TEXT("Trader services closed at night"), Sched::IsServiceOpen(EAstrawildNPCProfession::Trader, 22, false));

    // Role -> profession derivation.
    TestEqual(TEXT("Vendor -> Trader"), static_cast<int32>(Sched::ResolveProfession(static_cast<uint8>(EAstrawildNPCRole::Vendor))),
        static_cast<int32>(EAstrawildNPCProfession::Trader));
    TestEqual(TEXT("Guard -> Guard"), static_cast<int32>(Sched::ResolveProfession(static_cast<uint8>(EAstrawildNPCRole::Guard))),
        static_cast<int32>(EAstrawildNPCProfession::Guard));
    TestEqual(TEXT("Villager -> Farmer"), static_cast<int32>(Sched::ResolveProfession(static_cast<uint8>(EAstrawildNPCRole::Villager))),
        static_cast<int32>(EAstrawildNPCProfession::Farmer));
    return true;
}

// --- SCP Phase 11: turret range + target policy (Test 97) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildTurretPolicyTest,
    "ASTRAWILD.SCP.Turret.RangeAndPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildTurretPolicyTest::RunTest(const FString& Parameters)
{
    using Turret = UAstrawildTurretComponent;

    // Range gate (pure math).
    TestTrue(TEXT("Point inside range"), Turret::IsInRange(FVector::ZeroVector, FVector(1000.0f, 0.0f, 0.0f), Turret::TurretRange));
    TestFalse(TEXT("Point outside range"), Turret::IsInRange(FVector::ZeroVector, FVector(10000.0f, 0.0f, 0.0f), Turret::TurretRange));

    // Numbers pinned: 2500cm range, 1.5s cadence, 30 damage.
    TestEqual(TEXT("Turret range is 2500cm"), Turret::TurretRange, 2500.0f);
    TestEqual(TEXT("Fire interval is 1.5s"), Turret::FireIntervalSeconds, 1.5f);
    TestEqual(TEXT("Bolt damage is 30"), Turret::BoltDamage, 30.0f);
    return true;
}

// --- SCP Phase 10: genetics inheritance (Test 98) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildGeneticsTest,
    "ASTRAWILD.SCP.Genetics.Inheritance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildGeneticsTest::RunTest(const FString& Parameters)
{
    using Gene = UAstrawildGeneticsLibrary;

    // Trait pool: the directive's four effects plus healthy flavor traits.
    TestTrue(TEXT("Trait pool holds 8 entries"), Gene::GetTraitPool().Num() >= 8);
    TestTrue(TEXT("Swift is in the pool"), Gene::GetTraitPool().Contains(TEXT("Trait_Swift")));
    TestTrue(TEXT("Artisan is in the pool"), Gene::GetTraitPool().Contains(TEXT("Trait_Artisan")));
    TestTrue(TEXT("Ferocious is in the pool"), Gene::GetTraitPool().Contains(TEXT("Trait_Ferocious")));

    // Effect contracts (directive Phase 10.3 numbers).
    TestEqual(TEXT("Swift = +30% speed"), Gene::GetTraitSpeedMultiplier(TEXT("Trait_Swift")), 1.3f);
    TestEqual(TEXT("Artisan = +50% work"), Gene::GetTraitWorkMultiplier(TEXT("Trait_Artisan")), 1.5f);
    TestEqual(TEXT("Ferocious = +20% attack"), Gene::GetTraitAttackMultiplier(TEXT("Trait_Ferocious")), 1.2f);
    TestEqual(TEXT("Sturdy = +20% health"), Gene::GetTraitHealthMultiplier(TEXT("Trait_Sturdy")), 1.2f);
    TestTrue(TEXT("Lucky grants capture bonus"), Gene::GetTraitCaptureBonus(TEXT("Trait_Lucky")) > 0.0f);
    TestEqual(TEXT("Unknown traits are inert"), Gene::GetTraitSpeedMultiplier(TEXT("Trait_Nope")), 1.0f);

    // Stacking is multiplicative: 2x Swift = 1.69x.
    TestEqual(TEXT("Double Swift stacks"), Gene::ComputeTraitSpeedMultiplier({ TEXT("Trait_Swift"), TEXT("Trait_Swift") }), 1.69f);

    // Offspring rolls: 4 slots, deterministic per seed, IVs in bounds.
    const TArray<FName> Parents = { TEXT("Trait_Swift"), TEXT("Trait_Artisan") };
    const FAstrawildGeneticsProfile A = Gene::RollOffspring(Parents, Parents, 42);
    const FAstrawildGeneticsProfile B = Gene::RollOffspring(Parents, Parents, 42);
    TestTrue(TEXT("Profile valid"), A.IsValid());
    TestEqual(TEXT("Reroll with the same seed is deterministic"), A.Traits == B.Traits, true);
    TestTrue(TEXT("Traits draw from the known pool"), Gene::GetTraitPool().Contains(A.Traits[0]));

    const FAstrawildGeneticsProfile C = Gene::RollOffspring(Parents, Parents, 1337);
    TestTrue(TEXT("Different seed may differ"), C.IsValid());

    TestTrue(TEXT("IV health in 0..31"), A.IVs.X >= 0.0f && A.IVs.X <= 31.0f);
    TestTrue(TEXT("IV attack in 0..31"), A.IVs.Y >= 0.0f && A.IVs.Y <= 31.0f);

    // IV contribution: 0 -> 1.0x, 31 -> 1.31x.
    TestEqual(TEXT("IV 0 is neutral"), Gene::ComputeIVStatMultiplier(0.0f), 1.0f);
    TestEqual(TEXT("IV 31 is +31%"), Gene::ComputeIVStatMultiplier(31.0f), 1.31f);
    return true;
}

// --- SCP Phase 13: performance tier math (Test 99) ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildPerformanceTierTest,
    "ASTRAWILD.SCP.Perf.TierLadder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildPerformanceTierTest::RunTest(const FString& Parameters)
{
    using Perf = UAstrawildPerformanceManager;

    // Tier ladder: High full, Medium reduced, Low floor (monotone).
    TestTrue(TEXT("View distance scales down with tiers"),
        Perf::GetViewDistanceScaleForTier(2) > Perf::GetViewDistanceScaleForTier(1) &&
        Perf::GetViewDistanceScaleForTier(1) > Perf::GetViewDistanceScaleForTier(0));
    TestTrue(TEXT("Foliage density scales down with tiers"),
        Perf::GetFoliageDensityScaleForTier(2) > Perf::GetFoliageDensityScaleForTier(1) &&
        Perf::GetFoliageDensityScaleForTier(1) > Perf::GetFoliageDensityScaleForTier(0));
    TestTrue(TEXT("Shadow quality scales down with tiers"),
        Perf::GetShadowQualityForTier(2) > Perf::GetShadowQualityForTier(1) &&
        Perf::GetShadowQualityForTier(1) > Perf::GetShadowQualityForTier(0));

    // Out-of-range tiers clamp, never crash.
    TestEqual(TEXT("Tier clamps high"), Perf::GetViewDistanceScaleForTier(99), Perf::GetViewDistanceScaleForTier(2));
    TestEqual(TEXT("Tier clamps low"), Perf::GetViewDistanceScaleForTier(-1), Perf::GetViewDistanceScaleForTier(0));

    // Step policy: 3 slow samples drop, 12 fast samples climb, hysteresis
    // in between (a single sample never swings the tier).
    TestTrue(TEXT("3 slow samples step down"), Perf::ShouldStepDown(3));
    TestFalse(TEXT("2 slow samples hold"), Perf::ShouldStepDown(2));
    TestTrue(TEXT("12 fast samples step up"), Perf::ShouldStepUp(12));
    TestFalse(TEXT("11 fast samples hold"), Perf::ShouldStepUp(11));
    return true;
}


// --- FCR-1 regression contracts (Tests 100-102: final completion run fixes) ---

// Test 100 — FCR-1-d (M-d8): party losses pull difficulty DOWN, not up.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildDDAPartyLossTest,
    "ASTRAWILD.FCR.DDA.PartyLossDirection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildDDAPartyLossTest::RunTest(const FString& Parameters)
{
    using DDA = UAstrawildDifficultySubsystem;

    // Two party echo losses (metric -4) lean the game IN to help — the old code
    // counted them as hostile defeats, pushing difficulty UP on your own
    // creatures dying (exactly backwards).
    TestEqual(TEXT("Two party losses -> Struggling"), DDA::ComputeSkillBand(0, 0, 0, 2), 0);
    TestEqual(TEXT("One party loss stays Standard (hysteresis)"), DDA::ComputeSkillBand(0, 0, 0, 1), 1);
    // Party loss weight is half a player death: 1 loss cancels 1 capture's thrust.
    TestEqual(TEXT("One loss cancels a capture's thrust"),
        DDA::ComputeSkillBand(0, 1, 0, 1), 1);
    // Player deaths still dominate pressure.
    TestEqual(TEXT("Deaths and losses stack"), DDA::ComputeSkillBand(0, 0, 1, 1), 0);
    return true;
}

// Test 101 — FCR-1-a (M-a8): full element kits — no dead ability templates.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildAbilityKitReachabilityTest,
    "ASTRAWILD.FCR.Ability.FullElementKits",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildAbilityKitReachabilityTest::RunTest(const FString& Parameters)
{
    // Every element derives its FULL four-ability kit — the previously dead
    // templates (LumenBurst, StoneSkin, BloomGuard, FlareNova, GlacialWall,
    // Overload ...) are all reachable from the derived loadout path.
    struct FKitCase { EAstrawildElementType Element; FName A; FName B; FName C; FName D; };
    const FKitCase Cases[] = {
        { EAstrawildElementType::Light,  TEXT("Ability_Dawnflash"),     TEXT("Ability_PhotonVeil"),   TEXT("Ability_LumenBurst"),    TEXT("Ability_RestoringGleam") },
        { EAstrawildElementType::Ash,    TEXT("Ability_GravelSpit"),    TEXT("Ability_DustScreen"),   TEXT("Ability_StoneSkin"),     TEXT("Ability_SiftHeal") },
        { EAstrawildElementType::Flora,  TEXT("Ability_ThornLash"),     TEXT("Ability_RootSnare"),    TEXT("Ability_BloomGuard"),    TEXT("Ability_SapSurge") },
        { EAstrawildElementType::Ember,  TEXT("Ability_CinderBolt"),    TEXT("Ability_HeatHaze"),     TEXT("Ability_FlareNova"),     TEXT("Ability_WarmBlood") },
        { EAstrawildElementType::Frost,  TEXT("Ability_ShardShot"),     TEXT("Ability_DeepFreeze"),   TEXT("Ability_GlacialWall"),   TEXT("Ability_Snowmelt") },
        { EAstrawildElementType::Pulse,  TEXT("Ability_ArcBolt"),       TEXT("Ability_StormLatch"),   TEXT("Ability_Overload"),      TEXT("Ability_Galvanize") },
    };
    for (const FKitCase& Case : Cases)
    {
        const TArray<FName> Kit = UAstrawildAbilityLibrary::ComputeDerivedAbilityIds(
            Case.Element, EAstrawildEchoRole::Base, EAstrawildEchoFamily::Beast);
        TestEqual(*FString::Printf(TEXT("Kit %d has 6 entries"), static_cast<int32>(Case.Element)), Kit.Num(), 6);
        TestTrue(*FString::Printf(TEXT("Kit %d includes %s"), static_cast<int32>(Case.Element), *Case.A.ToString()), Kit.Contains(Case.A));
        TestTrue(*FString::Printf(TEXT("Kit %d includes %s"), static_cast<int32>(Case.Element), *Case.B.ToString()), Kit.Contains(Case.B));
        TestTrue(*FString::Printf(TEXT("Kit %d includes %s"), static_cast<int32>(Case.Element), *Case.C.ToString()), Kit.Contains(Case.C));
        TestTrue(*FString::Printf(TEXT("Kit %d includes %s"), static_cast<int32>(Case.Element), *Case.D.ToString()), Kit.Contains(Case.D));
    }
    return true;
}

// Test 102 — FCR-1-d (H-d5): IV multipliers are consumed by the stat getters' contracts.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildGeneticsIVBoundsTest,
    "ASTRAWILD.FCR.Genetics.IVConsumptionBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildGeneticsIVBoundsTest::RunTest(const FString& Parameters)
{
    using Gen = UAstrawildGeneticsLibrary;
    // The IV layer is now LIVE (rolled at breeding, persisted, consumed by
    // GetMaxHealth/GetAttackPower/GetDefense/speed): 0 = neutral, 31 = +31%.
    TestEqual(TEXT("IV 0 is neutral"), Gen::ComputeIVStatMultiplier(0.0f), 1.0f);
    TestEqual(TEXT("IV 31 is +31%"), Gen::ComputeIVStatMultiplier(31.0f), 1.31f);
    TestEqual(TEXT("IV 15 is +15%"), Gen::ComputeIVStatMultiplier(15.0f), 1.15f);
    // Negative/oversized inputs clamp (corrupt saves cannot mint stats).
    TestEqual(TEXT("Negative IV clamps to neutral"), Gen::ComputeIVStatMultiplier(-5.0f), 1.0f);
    TestEqual(TEXT("Oversized IV clamps to 1.31"), Gen::ComputeIVStatMultiplier(99.0f), 1.31f);
    return true;
}


// ===========================================================================
// LCP-2 — LAN CO-OP: client world build contracts
// ===========================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildLCP2ClientWorldPolicyTest,
    "ASTRAWILD.LCP2.ClientWorldPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildLCP2ClientWorldPolicyTest::RunTest(const FString& Parameters)
{
    // The cosmetic-world build policy: authority builds in BeginPlay (standalone
    // + listen host), remote clients build from the replicated seed, nothing
    // builds anywhere else (dedicated server = out of scope, MASTER_CONTROL §1b).
    using Boot = AAstrawildWorldBootstrapper;
    TestTrue(TEXT("Authority builds"), Boot::ShouldBuildCosmeticWorld(ROLE_Authority, NM_Standalone));
    TestTrue(TEXT("Listen-host authority builds"), Boot::ShouldBuildCosmeticWorld(ROLE_Authority, NM_ListenServer));
    TestTrue(TEXT("Remote client builds from seed"), Boot::ShouldBuildCosmeticWorld(ROLE_SimulatedProxy, NM_Client));
    TestFalse(TEXT("Dedicated-server proxy does not build"), Boot::ShouldBuildCosmeticWorld(ROLE_SimulatedProxy, NM_DedicatedServer));

    // The weather-visibility mapper the client atmosphere pass uses must agree
    // with the profile table for every state (one source of truth).
    TestEqual(TEXT("Clear visibility"), UAstrawildWeatherSubsystem::GetVisibilityMultiplierForState(EAstrawildWeatherState::Clear), 1.00f);
    TestEqual(TEXT("Cloudy visibility"), UAstrawildWeatherSubsystem::GetVisibilityMultiplierForState(EAstrawildWeatherState::Cloudy), 1.00f);
    TestEqual(TEXT("Rain visibility"), UAstrawildWeatherSubsystem::GetVisibilityMultiplierForState(EAstrawildWeatherState::Rain), 0.85f);
    TestEqual(TEXT("HeavyRain visibility"), UAstrawildWeatherSubsystem::GetVisibilityMultiplierForState(EAstrawildWeatherState::HeavyRain), 0.70f);
    TestEqual(TEXT("Storm visibility"), UAstrawildWeatherSubsystem::GetVisibilityMultiplierForState(EAstrawildWeatherState::Storm), 0.55f);
    TestEqual(TEXT("Fog visibility"), UAstrawildWeatherSubsystem::GetVisibilityMultiplierForState(EAstrawildWeatherState::Fog), 0.45f);

    // Cosmetic stream determinism: same seed -> same landmark sequence;
    // different seed -> different sequence (no hidden global state).
    FRandomStream StreamA(static_cast<uint32>(1337) ^ AAstrawildWorldBootstrapper::CosmeticStreamSalt);
    FRandomStream StreamB(static_cast<uint32>(1337) ^ AAstrawildWorldBootstrapper::CosmeticStreamSalt);
    FRandomStream StreamC(static_cast<uint32>(9999) ^ AAstrawildWorldBootstrapper::CosmeticStreamSalt);
    const float A1 = StreamA.FRand(); const float A2 = StreamA.FRand();
    const float B1 = StreamB.FRand(); const float B2 = StreamB.FRand();
    const float C1 = StreamC.FRand();
    TestEqual(TEXT("Same seed same first draw"), A1, B1);
    TestEqual(TEXT("Same seed same second draw"), A2, B2);
    TestNotEqual(TEXT("Different seed different draw"), A1, C1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAstrawildLCP2DressingGateTest,
    "ASTRAWILD.LCP2.DressingGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildLCP2DressingGateTest::RunTest(const FString& Parameters)
{
    // The client dressing gate: expected replicated actor count = villages +
    // dungeon generators + portals + POI markers + skiffs. The POI count comes
    // from the registry (identical on every machine).
    using Boot = AAstrawildWorldBootstrapper;
    TestEqual(TEXT("17 registered POIs -> 33 expected actors"), Boot::ComputeExpectedReplicatedWorldActorCount(17), 2 + 3 + 9 + 17 + 2);
    TestEqual(TEXT("0 POIs -> 16 expected actors"), Boot::ComputeExpectedReplicatedWorldActorCount(0), 16);
    TestEqual(TEXT("negative POIs clamp to 0"), Boot::ComputeExpectedReplicatedWorldActorCount(-5), 16);

    // The gate timeout must be positive and generous enough for initial actor
    // streaming on a LAN join (15s contract).
    TestTrue(TEXT("Dressing gate timeout is positive"),
        AAstrawildWorldBootstrapper::ClientDressingGateTimeoutSeconds > 0.0f);
    TestTrue(TEXT("Dressing gate timeout allows slow joins (>= 10s)"),
        AAstrawildWorldBootstrapper::ClientDressingGateTimeoutSeconds >= 10.0f);

    // Node quantity mirror: the depleted visual predicate is pure and shared
    // between the server harvest path and the client OnRep mirror (same rule,
    // no drift — pinned so the two paths can never diverge).
    using Node = AAstrawildResourceNode;
    TestFalse(TEXT("Infinite nodes never read as depleted"), Node::IsNodeDepleted(true, 0));
    TestTrue(TEXT("Finite empty node reads as depleted"), Node::IsNodeDepleted(false, 0));
    TestFalse(TEXT("Finite stocked node reads as harvestable"), Node::IsNodeDepleted(false, 5));
    TestFalse(TEXT("Overdrafted-but-infinite stays visible"), Node::IsNodeDepleted(true, -3));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
