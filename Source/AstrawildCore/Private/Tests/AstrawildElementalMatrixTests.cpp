#if WITH_DEV_AUTOMATION_TESTS

#include "AstrawildTypes.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAstrawildElementalMatrixCompatibilityTest,
    "Astrawild.Systems.Elements.Compatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAstrawildElementalMatrixCompatibilityTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Neutral serialized value is stable"), static_cast<uint8>(EAstrawildElement::Neutral), static_cast<uint8>(0));
    TestEqual(TEXT("Solar serialized value is stable"), static_cast<uint8>(EAstrawildElement::Solar), static_cast<uint8>(1));
    TestEqual(TEXT("Torrent serialized value is stable"), static_cast<uint8>(EAstrawildElement::Torrent), static_cast<uint8>(2));
    TestEqual(TEXT("Geo serialized value is stable"), static_cast<uint8>(EAstrawildElement::Geo), static_cast<uint8>(3));
    TestEqual(TEXT("Aether legacy serialized value is stable"), static_cast<uint8>(EAstrawildElement::Aether), static_cast<uint8>(4));

    TArray<TPair<EAstrawildElement, EAstrawildElement>> AdvantageEdges;
    AdvantageEdges.Add(TPair<EAstrawildElement, EAstrawildElement>(EAstrawildElement::Abyssal, EAstrawildElement::Solar));
    AdvantageEdges.Add(TPair<EAstrawildElement, EAstrawildElement>(EAstrawildElement::Solar, EAstrawildElement::Glacial));
    AdvantageEdges.Add(TPair<EAstrawildElement, EAstrawildElement>(EAstrawildElement::Glacial, EAstrawildElement::Geo));
    AdvantageEdges.Add(TPair<EAstrawildElement, EAstrawildElement>(EAstrawildElement::Geo, EAstrawildElement::Volt));
    AdvantageEdges.Add(TPair<EAstrawildElement, EAstrawildElement>(EAstrawildElement::Volt, EAstrawildElement::Torrent));
    AdvantageEdges.Add(TPair<EAstrawildElement, EAstrawildElement>(EAstrawildElement::Torrent, EAstrawildElement::Abyssal));
    AdvantageEdges.Add(TPair<EAstrawildElement, EAstrawildElement>(EAstrawildElement::Solar, EAstrawildElement::Geo));
    AdvantageEdges.Add(TPair<EAstrawildElement, EAstrawildElement>(EAstrawildElement::Geo, EAstrawildElement::Torrent));
    AdvantageEdges.Add(TPair<EAstrawildElement, EAstrawildElement>(EAstrawildElement::Torrent, EAstrawildElement::Solar));

    for (const TPair<EAstrawildElement, EAstrawildElement>& Edge : AdvantageEdges)
    {
        const FString EdgeName = FString::Printf(TEXT("%s beats %s"), *UEnum::GetValueAsString(Edge.Key), *UEnum::GetValueAsString(Edge.Value));
        TestTrue(EdgeName + TEXT(" has advantage"), FMath::IsNearlyEqual(FAstrawildElementalMatrix::GetMultiplier(Edge.Key, Edge.Value), 1.75f));
        TestTrue(EdgeName + TEXT(" reverses to disadvantage"), FMath::IsNearlyEqual(FAstrawildElementalMatrix::GetMultiplier(Edge.Value, Edge.Key), 0.50f));
    }

    TArray<EAstrawildElement> NonNeutralElements;
    NonNeutralElements.Add(EAstrawildElement::Solar);
    NonNeutralElements.Add(EAstrawildElement::Torrent);
    NonNeutralElements.Add(EAstrawildElement::Geo);
    NonNeutralElements.Add(EAstrawildElement::Aether);
    NonNeutralElements.Add(EAstrawildElement::Volt);
    NonNeutralElements.Add(EAstrawildElement::Glacial);
    NonNeutralElements.Add(EAstrawildElement::Abyssal);
    NonNeutralElements.Add(EAstrawildElement::Astra);
    for (const EAstrawildElement Element : NonNeutralElements)
    {
        TestTrue(FString::Printf(TEXT("Same element resistance for %s"), *UEnum::GetValueAsString(Element)),
            FMath::IsNearlyEqual(FAstrawildElementalMatrix::GetMultiplier(Element, Element), 0.75f));
    }

    TestTrue(TEXT("Neutral matchup is neutral"), FMath::IsNearlyEqual(
        FAstrawildElementalMatrix::GetMultiplier(EAstrawildElement::Neutral, EAstrawildElement::Abyssal), 1.0f));
    TestTrue(TEXT("Empty defender affinity list is neutral"), FMath::IsNearlyEqual(
        FAstrawildElementalMatrix::GetMultiplier(EAstrawildElement::Abyssal, TArray<EAstrawildElement>()), 1.0f));

    TArray<EAstrawildElement> StackedDefenders;
    StackedDefenders.Add(EAstrawildElement::Solar);
    StackedDefenders.Add(EAstrawildElement::Solar);
    StackedDefenders.Add(EAstrawildElement::Solar);
    TestTrue(TEXT("Stacked defender multiplier is bounded"), FMath::IsWithinInclusive(
        FAstrawildElementalMatrix::GetMultiplier(EAstrawildElement::Abyssal, StackedDefenders), 0.25f, 2.50f));

    return true;
}

#endif
