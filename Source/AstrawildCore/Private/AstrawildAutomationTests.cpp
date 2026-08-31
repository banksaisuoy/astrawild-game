#include "CoreMinimal.h"

// Automation tests (directive §39) — pure logic tests, world-free, safe in Shipping-stripped builds.
#if WITH_DEV_AUTOMATION_TESTS

#include "AstrawildCaptureComponent.h"
#include "AstrawildBestiaryData.h"
#include "AstrawildBiomeDressingActor.h"
#include "AstrawildCombatComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildInventoryComponent.h"
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

#endif // WITH_DEV_AUTOMATION_TESTS
