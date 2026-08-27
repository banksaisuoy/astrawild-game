#include "Components/AstrawildTechnologyComponent.h"

#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

UAstrawildTechnologyComponent::UAstrawildTechnologyComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAstrawildTechnologyComponent::AddResearchPoints(const int32 Amount)
{
    if (GetOwner() && !GetOwner()->HasAuthority())
    {
        return;
    }
    if (Amount > 0)
    {
        ResearchPoints = FMath::Max(0, ResearchPoints + Amount);
    }
}

bool UAstrawildTechnologyComponent::IsTechnologyUnlocked(const FGameplayTag& TechnologyTag) const
{
    return TechnologyTag.IsValid() && UnlockedTechnologyTags.Contains(TechnologyTag);
}

bool UAstrawildTechnologyComponent::CanUnlockTechnology(const FGameplayTag& TechnologyTag, FText& OutFailureReason) const
{
    OutFailureReason = FText::GetEmpty();
    const FAstrawildTechnologyNodeRow* Row = FindTechnologyRow(TechnologyTag);
    if (!Row)
    {
        OutFailureReason = FText::FromString(TEXT("Technology node is not configured."));
        return false;
    }
    if (IsTechnologyUnlocked(TechnologyTag))
    {
        OutFailureReason = FText::FromString(TEXT("Technology is already unlocked."));
        return false;
    }
    if (ResearchPoints < Row->ResearchCost)
    {
        OutFailureReason = FText::FromString(TEXT("Not enough research points."));
        return false;
    }
    for (const FGameplayTag& Prerequisite : Row->PrerequisiteTechnologyTags)
    {
        if (Prerequisite.IsValid() && !IsTechnologyUnlocked(Prerequisite))
        {
            OutFailureReason = FText::FromString(TEXT("A prerequisite technology is missing."));
            return false;
        }
    }
    return true;
}

bool UAstrawildTechnologyComponent::TryUnlockTechnology(const FGameplayTag& TechnologyTag)
{
    if (GetOwner() && !GetOwner()->HasAuthority())
    {
        OnTechnologyUnlockFailed.Broadcast(FText::FromString(TEXT("Technology unlock requires server authority.")));
        return false;
    }

    FText FailureReason;
    if (!CanUnlockTechnology(TechnologyTag, FailureReason))
    {
        OnTechnologyUnlockFailed.Broadcast(FailureReason);
        return false;
    }

    const FAstrawildTechnologyNodeRow* Row = FindTechnologyRow(TechnologyTag);
    if (!Row)
    {
        OnTechnologyUnlockFailed.Broadcast(FText::FromString(TEXT("Technology node disappeared during unlock.")));
        return false;
    }

    ResearchPoints = FMath::Max(0, ResearchPoints - FMath::Max(0, Row->ResearchCost));
    UnlockedTechnologyTags.AddUnique(TechnologyTag);
    OnTechnologyUnlocked.Broadcast(TechnologyTag);
    return true;
}

void UAstrawildTechnologyComponent::LoadTechnologyState(const TArray<FGameplayTag>& InUnlockedTags, const int32 InResearchPoints)
{
    UnlockedTechnologyTags.Reset();
    for (const FGameplayTag& Tag : InUnlockedTags)
    {
        if (Tag.IsValid() && !UnlockedTechnologyTags.Contains(Tag))
        {
            UnlockedTechnologyTags.Add(Tag);
        }
    }
    ResearchPoints = FMath::Max(0, InResearchPoints);
}

const FAstrawildTechnologyNodeRow* UAstrawildTechnologyComponent::FindTechnologyRow(const FGameplayTag& TechnologyTag) const
{
    if (!TechnologyTable || !TechnologyTag.IsValid())
    {
        return nullptr;
    }

    static const FString Context(TEXT("TechnologyLookup"));
    for (const TPair<FName, uint8*>& RowPair : TechnologyTable->GetRowMap())
    {
        const FAstrawildTechnologyNodeRow* Row = reinterpret_cast<const FAstrawildTechnologyNodeRow*>(RowPair.Value);
        if (Row && Row->TechnologyTag == TechnologyTag)
        {
            return Row;
        }
    }
    return nullptr;
}
