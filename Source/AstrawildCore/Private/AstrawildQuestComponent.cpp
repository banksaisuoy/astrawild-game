#include "AstrawildQuestComponent.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildResearchSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UAstrawildQuestComponent::UAstrawildQuestComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAstrawildQuestComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    if (UAstrawildEventBusSubsystem* EventBus = GetEventBus())
    {
        EventBus->OnGameplayEvent.AddDynamic(this, &UAstrawildQuestComponent::HandleGameplayEvent);
    }

    // Start the first quest when no quest state exists (new game).
    if (QuestStates.IsEmpty() && !StartingQuestId.IsNone())
    {
        StartQuest(StartingQuestId);
    }
}

void UAstrawildQuestComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UAstrawildEventBusSubsystem* EventBus = GetEventBus())
    {
        EventBus->OnGameplayEvent.RemoveAll(this);
    }
    Super::EndPlay(EndPlayReason);
}

UAstrawildItemRegistrySubsystem* UAstrawildQuestComponent::GetRegistry() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
}

UAstrawildEventBusSubsystem* UAstrawildQuestComponent::GetEventBus() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildEventBusSubsystem>() : nullptr;
}

AAstrawildPlayerCharacter* UAstrawildQuestComponent::GetPlayerCharacter() const
{
    const APlayerController* PC = Cast<APlayerController>(GetOwner());
    return PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
}

bool UAstrawildQuestComponent::StartQuest(const FName QuestId)
{
    if (IsQuestActive(QuestId) || IsQuestCompleted(QuestId))
    {
        return false;
    }

    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    UAstrawildQuestDefinition* Definition = Registry ? Registry->FindQuest(QuestId) : nullptr;
    if (!Definition)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("StartQuest: unknown quest %s."), *QuestId.ToString());
        return false;
    }

    FAstrawildQuestSaveData State;
    State.QuestId = QuestId;
    State.Objectives = Definition->Objectives;
    // Reset runtime progress carried by definition copies.
    for (FAstrawildQuestObjective& Objective : State.Objectives)
    {
        Objective.ProgressCount = 0;
    }
    State.bActive = true;
    State.bCompleted = false;

    QuestStates.Add(State);
    ActiveQuestId = QuestId;
    OnQuestStateChanged.Broadcast(QuestId, false);
    UE_LOG(LogAstrawild, Log, TEXT("Quest started: %s."), *QuestId.ToString());
    return true;
}

bool UAstrawildQuestComponent::IsQuestActive(const FName QuestId) const
{
    return ActiveQuestId == QuestId;
}

bool UAstrawildQuestComponent::IsQuestCompleted(const FName QuestId) const
{
    return CompletedQuestIds.Contains(QuestId);
}

TArray<FAstrawildQuestObjective> UAstrawildQuestComponent::GetActiveObjectives() const
{
    if (const FAstrawildQuestSaveData* State = QuestStates.FindByPredicate(
        [this](const FAstrawildQuestSaveData& Item) { return Item.QuestId == ActiveQuestId; }))
    {
        return State->Objectives;
    }
    return TArray<FAstrawildQuestObjective>();
}

FName UAstrawildQuestComponent::GetActiveQuestId() const
{
    return ActiveQuestId;
}

void UAstrawildQuestComponent::HandleGameplayEvent(const FAstrawildGameplayEvent& Event)
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    ApplyEventToQuest(Event);
}

void UAstrawildQuestComponent::ApplyEventToQuest(const FAstrawildGameplayEvent& Event)
{
    if (ActiveQuestId.IsNone())
    {
        return;
    }

    FAstrawildQuestSaveData* State = QuestStates.FindByPredicate(
        [this](const FAstrawildQuestSaveData& Item) { return Item.QuestId == ActiveQuestId && Item.bActive; });
    if (!State)
    {
        return;
    }

    bool bAnyProgress = false;
    bool bAllComplete = true;

    for (int32 i = 0; i < State->Objectives.Num(); ++i)
    {
        FAstrawildQuestObjective& Objective = State->Objectives[i];
        if (Objective.IsComplete())
        {
            continue;
        }

        bool bMatches = false;
        switch (Objective.Type)
        {
        case EAstrawildQuestObjectiveType::ReachLocation:
            // Batch 6: the dungeon portals are the first publishers of
            // Event.LocationReached — this matcher existed for neither until now.
            bMatches = Event.EventTag == TAG_Astrawild_Event_LocationReached && Event.TargetId == Objective.TargetId;
            break;
        case EAstrawildQuestObjectiveType::CollectItem:
            bMatches = Event.EventTag == TAG_Astrawild_Event_ItemCollected && Event.TargetId == Objective.TargetId;
            break;
        case EAstrawildQuestObjectiveType::CaptureEcho:
            bMatches = Event.EventTag == TAG_Astrawild_Event_EchoCaptured && Event.TargetId == Objective.TargetId;
            break;
        case EAstrawildQuestObjectiveType::DefeatCreature:
            bMatches = (Event.EventTag == TAG_Astrawild_Event_EchoDefeated || Event.EventTag == TAG_Astrawild_Event_HostileDefeated)
                && Event.TargetId == Objective.TargetId;
            break;
        case EAstrawildQuestObjectiveType::CraftRecipe:
            bMatches = Event.EventTag == TAG_Astrawild_Event_RecipeCrafted && Event.TargetId == Objective.TargetId;
            break;
        case EAstrawildQuestObjectiveType::PlaceBuilding:
            bMatches = Event.EventTag == TAG_Astrawild_Event_BuildingPlaced && Event.TargetId == Objective.TargetId;
            break;
        case EAstrawildQuestObjectiveType::UnlockTechnology:
            bMatches = Event.EventTag == TAG_Astrawild_Event_TechUnlocked && Event.TargetId == Objective.TargetId;
            break;
        case EAstrawildQuestObjectiveType::ObserveEcho:
            bMatches = Event.EventTag == TAG_Astrawild_Event_EchoObserved && Event.TargetId == Objective.TargetId;
            break;
        default:
            break;
        }

        if (bMatches)
        {
            Objective.ProgressCount = FMath::Min(Objective.RequiredCount, Objective.ProgressCount + FMath::Max(1, Event.Amount));
            bAnyProgress = true;
            OnObjectiveProgress.Broadcast(State->QuestId, i, Objective.ProgressCount, Objective.RequiredCount);
        }

        if (!Objective.IsComplete())
        {
            bAllComplete = false;
        }
    }

    if (bAnyProgress)
    {
        UE_LOG(LogAstrawild, Log, TEXT("Quest %s progress via event %s."), *State->QuestId.ToString(), *Event.EventTag.ToString());
    }

    if (bAllComplete)
    {
        CompleteQuest(State->QuestId);
    }
}

void UAstrawildQuestComponent::CompleteQuest(const FName QuestId)
{
    FAstrawildQuestSaveData* State = QuestStates.FindByPredicate(
        [&QuestId](const FAstrawildQuestSaveData& Item) { return Item.QuestId == QuestId; });
    if (!State || State->bCompleted)
    {
        return;
    }

    State->bCompleted = true;
    State->bActive = false;
    CompletedQuestIds.AddUnique(QuestId);

    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    UAstrawildQuestDefinition* Definition = Registry ? Registry->FindQuest(QuestId) : nullptr;
    if (Definition)
    {
        GrantRewards(Definition);
    }

    OnQuestStateChanged.Broadcast(QuestId, true);
    UE_LOG(LogAstrawild, Log, TEXT("Quest completed: %s."), *QuestId.ToString());

    // Chain the next quest (directive §25 quest chain).
    if (Definition && !Definition->NextQuestId.IsNone())
    {
        ActiveQuestId = NAME_None;
        StartQuest(Definition->NextQuestId);
    }
    else
    {
        ActiveQuestId = NAME_None;
    }
}

void UAstrawildQuestComponent::GrantRewards(const UAstrawildQuestDefinition* Definition)
{
    if (!Definition)
    {
        return;
    }

    AAstrawildPlayerCharacter* Player = GetPlayerCharacter();

    if (Player && Player->InventoryComponent)
    {
        for (const FAstrawildItemStack& Reward : Definition->RewardItems)
        {
            Player->InventoryComponent->AddItem(Reward.ItemId, Reward.Quantity);
        }
    }

    if (Definition->RewardResearchPoints > 0)
    {
        const UWorld* World = GetWorld();
        if (World && World->GetGameInstance())
        {
            if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
            {
                Research->AddResearchPoints(Definition->RewardResearchPoints);
            }
        }
    }

    // Story-critical tech unlock (directive §19 research tie-in).
    if (!Definition->RewardTechId.IsNone())
    {
        const UWorld* World = GetWorld();
        if (World && World->GetGameInstance())
        {
            if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
            {
                Research->TryUnlockTech(Definition->RewardTechId);
            }
        }
    }
}

void UAstrawildQuestComponent::ExportForSave(TArray<FAstrawildQuestSaveData>& OutQuests) const
{
    OutQuests = QuestStates;
}

void UAstrawildQuestComponent::ImportFromSave(const TArray<FAstrawildQuestSaveData>& InQuests)
{
    QuestStates = InQuests;
    CompletedQuestIds.Reset();
    ActiveQuestId = NAME_None;

    for (const FAstrawildQuestSaveData& State : QuestStates)
    {
        if (State.bCompleted)
        {
            CompletedQuestIds.AddUnique(State.QuestId);
        }
        else if (State.bActive)
        {
            ActiveQuestId = State.QuestId;
        }
    }

    // Saved game without any active quest and an unstarted chain: restart the intro quest.
    if (QuestStates.IsEmpty() && !StartingQuestId.IsNone())
    {
        StartQuest(StartingQuestId);
    }
}
