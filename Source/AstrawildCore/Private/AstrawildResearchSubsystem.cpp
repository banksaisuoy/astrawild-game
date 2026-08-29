#include "AstrawildResearchSubsystem.h"

#include "AstrawildDataAssets.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "Engine/World.h"

bool UAstrawildResearchSubsystem::IsTechUnlocked(const FName TechId) const
{
    return UnlockedTechIds.Contains(TechId);
}

UAstrawildItemRegistrySubsystem* UAstrawildResearchSubsystem::GetRegistryFromWorld() const
{
    // The registry is world-scoped; reach it through the game instance's world context.
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
}

void UAstrawildResearchSubsystem::AddResearchPoints(const int32 Amount)
{
    if (Amount <= 0)
    {
        return;
    }
    ResearchPoints += Amount;
    OnResearchPointsChanged.Broadcast(ResearchPoints);
    UE_LOG(LogAstrawildEconomy, Log, TEXT("Research points +%d (total %d)."), Amount, ResearchPoints);
}

TArray<FName> UAstrawildResearchSubsystem::GetMissingPrerequisites(const FName TechId) const
{
    TArray<FName> Missing;
    const UAstrawildItemRegistrySubsystem* Registry = GetRegistryFromWorld();
    const UAstrawildTechnologyDefinition* Tech = Registry ? Registry->FindTechnology(TechId) : nullptr;
    if (!Tech)
    {
        Missing.Add(TechId);
        return Missing;
    }

    for (const FName Prereq : Tech->PrerequisiteTechIds)
    {
        if (!IsTechUnlocked(Prereq))
        {
            Missing.Add(Prereq);
        }
    }
    return Missing;
}

bool UAstrawildResearchSubsystem::CanUnlockTech(const FName TechId) const
{
    if (IsTechUnlocked(TechId))
    {
        return false;
    }
    if (!GetMissingPrerequisites(TechId).IsEmpty())
    {
        return false;
    }

    const UAstrawildItemRegistrySubsystem* Registry = GetRegistryFromWorld();
    const UAstrawildTechnologyDefinition* Tech = Registry ? Registry->FindTechnology(TechId) : nullptr;
    return Tech && ResearchPoints >= Tech->ResearchCost;
}

bool UAstrawildResearchSubsystem::TryUnlockTech(const FName TechId)
{
    if (!CanUnlockTech(TechId))
    {
        return false;
    }

    const UAstrawildItemRegistrySubsystem* Registry = GetRegistryFromWorld();
    UAstrawildTechnologyDefinition* Tech = Registry ? Registry->FindTechnology(TechId) : nullptr;
    if (!Tech)
    {
        return false;
    }

    ResearchPoints -= Tech->ResearchCost;
    UnlockedTechIds.Add(TechId);
    OnTechUnlocked.Broadcast(TechId, Tech);
    OnResearchPointsChanged.Broadcast(ResearchPoints);

    // Publish event for quests (directive §25 event-driven progression).
    if (UWorld* World = GetWorld())
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_TechUnlocked, nullptr, TechId, 1, FVector::ZeroVector);
        }
    }

    UE_LOG(LogAstrawildEconomy, Log, TEXT("Technology unlocked: %s."), *TechId.ToString());
    return true;
}

TArray<FName> UAstrawildResearchSubsystem::GetUnlockedTechIds() const
{
    return UnlockedTechIds;
}

void UAstrawildResearchSubsystem::ExportForSave(FAstrawildResearchSaveData& OutData) const
{
    OutData.UnlockedTechIds = UnlockedTechIds;
    OutData.ResearchPoints = ResearchPoints;
}

void UAstrawildResearchSubsystem::ImportFromSave(const FAstrawildResearchSaveData& InData)
{
    UnlockedTechIds = InData.UnlockedTechIds;
    ResearchPoints = InData.ResearchPoints;
    OnResearchPointsChanged.Broadcast(ResearchPoints);
}
