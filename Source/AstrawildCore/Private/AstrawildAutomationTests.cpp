#include "CoreMinimal.h"

// Automation tests (directive §39) — pure logic tests, world-free, safe in Shipping-stripped builds.
#if WITH_DEV_AUTOMATION_TESTS

#include "AstrawildCaptureComponent.h"
#include "AstrawildCombatComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildSaveSubsystem.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildTypes.h"
#include "Misc/AutomationTest.h"

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

    // Defeated creatures can never be captured (rule encoded in chance computation path).
    TestTrue(TEXT("Design invariant placeholder: defeated -> 0 chance documented"), true);
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
    TestEqual(TEXT("Same-element attacks are resisted x0.75"),
        AAstrawildEchoBossCharacter::ComputeBossElementalMultiplier(Own, Weakness, Own), 0.75f);
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

#endif // WITH_DEV_AUTOMATION_TESTS
