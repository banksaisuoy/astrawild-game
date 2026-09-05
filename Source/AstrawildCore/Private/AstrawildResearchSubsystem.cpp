#include "AstrawildResearchSubsystem.h"

#include "AstrawildPlayerController.h" // LCP-5: unlock notifications
#include "AstrawildGameState.h" // LCP-5: mirror

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
    SyncMirrorToGameState(); // LCP-5: remote client screens stay current
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
    SyncMirrorToGameState();      // LCP-5: remote client mirrors
    NotifyPlayersResearchUnlocked(TechId); // LCP-5: every screen hears it

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

bool UAstrawildResearchSubsystem::ForceUnlockTech(const FName TechId)
{
    if (TechId.IsNone() || IsTechUnlocked(TechId))
    {
        return false;
    }

    const UAstrawildItemRegistrySubsystem* Registry = GetRegistryFromWorld();
    UAstrawildTechnologyDefinition* Tech = Registry ? Registry->FindTechnology(TechId) : nullptr;
    if (!Tech)
    {
        return false;
    }

    UnlockedTechIds.Add(TechId);
    OnTechUnlocked.Broadcast(TechId, Tech);
    OnResearchPointsChanged.Broadcast(ResearchPoints);
    SyncMirrorToGameState();      // LCP-5: remote client mirrors
    NotifyPlayersResearchUnlocked(TechId); // LCP-5: every screen hears it

    // Publish the same quest-facing event as TryUnlockTech (directive §25).
    if (UWorld* World = GetWorld())
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_TechUnlocked, nullptr, TechId, 1, FVector::ZeroVector);
        }
    }

    UE_LOG(LogAstrawildEconomy, Log, TEXT("Technology force-unlocked (dungeon reward): %s."), *TechId.ToString());
    return true;
}

TArray<FName> UAstrawildResearchSubsystem::GetUnlockedTechIds() const
{
    return UnlockedTechIds;
}

void UAstrawildResearchSubsystem::GrantStartingTechnologies()
{
    // Audit C-2: free root techs (cost 0, no prerequisites) unlock automatically so
    // basic crafting works from a fresh session. Runs server-side from the game mode.
    UAstrawildItemRegistrySubsystem* Registry = GetRegistryFromWorld();
    if (!Registry)
    {
        return;
    }

    int32 Granted = 0;
    for (const UAstrawildTechnologyDefinition* Tech : Registry->GetAllTechnologies())
    {
        if (!Tech || Tech->TechId.IsNone() || IsTechUnlocked(Tech->TechId))
        {
            continue;
        }
        if (Tech->ResearchCost <= 0 && Tech->PrerequisiteTechIds.IsEmpty())
        {
            // TryUnlockTech re-checks gates (points >= 0 passes) and publishes events.
            if (TryUnlockTech(Tech->TechId))
            {
                ++Granted;
            }
        }
    }

    if (Granted > 0)
    {
        UE_LOG(LogAstrawildEconomy, Log, TEXT("Granted %d starting technologies (free root nodes)."), Granted);
    }
    SyncMirrorToGameState(); // LCP-5: fresh-session mirror
}

FName UAstrawildResearchSubsystem::GetNextUnlockableTechId(int32& OutCost, FText& OutDisplayName) const
{
    OutCost = 0;
    OutDisplayName = FText::GetEmpty();

    const UAstrawildItemRegistrySubsystem* Registry = GetRegistryFromWorld();
    if (!Registry)
    {
        return NAME_None;
    }

    FName BestId = NAME_None;
    int32 BestCost = MAX_int32;
    for (const UAstrawildTechnologyDefinition* Tech : Registry->GetAllTechnologies())
    {
        if (!Tech || !CanUnlockTech(Tech->TechId))
        {
            continue;
        }
        if (Tech->ResearchCost < BestCost)
        {
            BestCost = Tech->ResearchCost;
            BestId = Tech->TechId;
            OutDisplayName = Tech->DisplayName;
        }
    }

    OutCost = (BestId != NAME_None) ? BestCost : 0;
    return BestId;
}

void UAstrawildResearchSubsystem::ExportForSave(FAstrawildResearchSaveData& OutData) const
{
    OutData.UnlockedTechIds = UnlockedTechIds;
    OutData.ResearchPoints = ResearchPoints;
}

void UAstrawildResearchSubsystem::ImportFromSave(const FAstrawildResearchSaveData& InData)
{
    // Final-audit M-3: sanitized import (mirrors the quest/roster policy — the
    // earlier hardening of this exact path was lost with the destroyed Final-Run
    // branch and never re-landed). Duplicates bloat the save and double-list the
    // research screen; negative RP verbatim would break every cost check below zero.
    UnlockedTechIds.Reset();
    for (const FName TechId : InData.UnlockedTechIds)
    {
        if (TechId.IsNone())
        {
            UE_LOG(LogAstrawildEconomy, Warning, TEXT("ImportFromSave: dropped tech entry with no id."));
            continue;
        }
        if (UnlockedTechIds.Contains(TechId))
        {
            UE_LOG(LogAstrawildEconomy, Warning, TEXT("ImportFromSave: duplicate tech %s — first entry wins."), *TechId.ToString());
            continue;
        }
        UnlockedTechIds.Add(TechId);
    }
    ResearchPoints = FMath::Max(0, InData.ResearchPoints);
    if (ResearchPoints != InData.ResearchPoints)
    {
        UE_LOG(LogAstrawildEconomy, Warning, TEXT("ImportFromSave: negative research points clamped to 0 (was %d)."), InData.ResearchPoints);
    }
    OnResearchPointsChanged.Broadcast(ResearchPoints);
    SyncMirrorToGameState(); // LCP-5: post-load mirror
}

void UAstrawildResearchSubsystem::SyncMirrorToGameState()
{
    // LCP-5: replicate the shared pool snapshot (host write → client mirror).
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return; // clients import through AAstrawildGameState::OnRep_ResearchMirror
    }
    if (AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
    {
        ExportForSave(GameState->ResearchMirror);
    }
}

void UAstrawildResearchSubsystem::NotifyPlayersResearchUnlocked(const FName TechId)
{
    // LCP-5: PART 18 feedback — research unlocks reach EVERY screen (host
    // toast + remote clients via the ClientNotify routing).
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    const FText Message = FText::FromString(FString::Printf(TEXT("Research unlocked: %s"), *TechId.ToString()));
    for (FConstControllerIterator It = World->GetControllerIterator(); It; ++It)
    {
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(*It))
        {
            PC->NotifyPlayer(Message);
        }
    }
}
