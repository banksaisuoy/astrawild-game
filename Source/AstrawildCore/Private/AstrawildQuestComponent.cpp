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
    // Final production run: SurviveTime objectives need a per-second accrual tick
    // (early-outs instantly when no such objective is active — see TickComponent).
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.0f;
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

void UAstrawildQuestComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Server-side only; 1s cadence set in the constructor — negligible cost, and the
    // early-return below keeps idle quests at zero work.
    if (GetOwnerRole() != ROLE_Authority || ActiveQuestId.IsNone())
    {
        return;
    }

    TickSurviveTimeObjectives(DeltaTime);
}

void UAstrawildQuestComponent::TickSurviveTimeObjectives(const float DeltaTime)
{
    // Final production run: SurviveTime objectives accrue REAL seconds while the
    // owning player is alive (dying pauses the clock — survival means surviving).
    AAstrawildPlayerCharacter* Player = GetPlayerCharacter();
    if (!Player || !Player->IsAlive())
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

        if (Objective.Type == EAstrawildQuestObjectiveType::SurviveTime)
        {
            // RequiredCount is the number of SECONDS to survive.
            Objective.ProgressCount = FMath::Min(Objective.RequiredCount, Objective.ProgressCount + FMath::Max(1, FMath::RoundToInt(DeltaTime)));
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
        UE_LOG(LogAstrawild, Verbose, TEXT("Quest %s survive-time ticking."), *State->QuestId.ToString());
    }

    if (bAllComplete)
    {
        CompleteQuest(State->QuestId);
    }
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
        case EAstrawildQuestObjectiveType::VisitZone:
            // Final production run: consumes Event.ZoneEntered (published by the zone
            // subsystem since Batch 7 with the zone name as TargetId).
            bMatches = Event.EventTag == TAG_Astrawild_Event_ZoneEntered && Event.TargetId == Objective.TargetId;
            break;
        case EAstrawildQuestObjectiveType::SurviveTime:
            // Time-based — accrued by TickSurviveTimeObjectives, not by events.
            break;
        case EAstrawildQuestObjectiveType::DiscoverPOI:
            // Production V2: consumes Event.PoiDiscovered (published by the POI
            // subsystem on first discovery, POI id as TargetId).
            bMatches = Event.EventTag == TAG_Astrawild_Event_PoiDiscovered && Event.TargetId == Objective.TargetId;
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
    // FR-3 (Final Run redo): re-entrancy guard. The completion path broadcasts
    // delegates and chains the next quest synchronously; a future handler that
    // completes a quest from inside OnQuestStateChanged would re-enter here and
    // double-grant rewards before the state flag below settles.
    if (bBusyCompletingQuest)
    {
        return;
    }

    FAstrawildQuestSaveData* State = QuestStates.FindByPredicate(
        [&QuestId](const FAstrawildQuestSaveData& Item) { return Item.QuestId == QuestId; });
    if (!State || State->bCompleted)
    {
        return;
    }

    TGuardValue<bool> BusyGuard(bBusyCompletingQuest, true);

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
            // FR-3 (Final Run redo): negative/zero rewards are data bugs — the
            // inventory layer now rejects them, but the quest layer logs the bad
            // definition by name so the data author can fix it instead of the
            // reward silently never arriving.
            if (!Reward.IsValid())
            {
                UE_LOG(LogAstrawild, Warning, TEXT("GrantRewards: skipped invalid reward %s x%d on quest %s."),
                    *Reward.ItemId.ToString(), Reward.Quantity, *Definition->QuestId.ToString());
                continue;
            }
            // AddItemSilent (Q-9): a reward firing ItemCollected would let a CollectItem
            // objective complete itself by paying its own reward — silent grants only.
            Player->InventoryComponent->AddItemSilent(Reward.ItemId, Reward.Quantity);
        }
    }

    // FR-3: negative research rewards are the same class of data bug — skip loudly.
    if (Definition->RewardResearchPoints < 0)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("GrantRewards: negative RP reward %d on quest %s — skipped."),
            Definition->RewardResearchPoints, *Definition->QuestId.ToString());
    }
    else if (Definition->RewardResearchPoints > 0)
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
    // FR-3 (Final Run redo): save-import sanitize. Duplicated quest ids used to
    // stack (bloating the save and the HUD list); more than one ACTIVE quest broke
    // the single-active invariant the objective matcher relies on. First-seen-wins
    // for ids, first-active-wins for the active slot; every repair is logged.
    QuestStates.Reset();
    CompletedQuestIds.Reset();
    ActiveQuestId = NAME_None;

    for (const FAstrawildQuestSaveData& State : InQuests)
    {
        if (State.QuestId.IsNone())
        {
            UE_LOG(LogAstrawild, Warning, TEXT("ImportFromSave: dropped quest state with no id."));
            continue;
        }
        if (QuestStates.ContainsByPredicate(
            [&State](const FAstrawildQuestSaveData& Item) { return Item.QuestId == State.QuestId; }))
        {
            UE_LOG(LogAstrawild, Warning, TEXT("ImportFromSave: duplicate quest %s — first entry wins."), *State.QuestId.ToString());
            continue;
        }

        QuestStates.Add(State);
        if (State.bCompleted)
        {
            CompletedQuestIds.AddUnique(State.QuestId);
        }
        else if (State.bActive)
        {
            if (ActiveQuestId.IsNone())
            {
                ActiveQuestId = State.QuestId;
            }
            else
            {
                // Demote the extra active quest — exactly one may be active at a time.
                QuestStates.Last().bActive = false;
                UE_LOG(LogAstrawild, Warning, TEXT("ImportFromSave: quest %s active past the first active quest — demoted."), *State.QuestId.ToString());
            }
        }
    }

    // Saved game without any quest state and an unstarted chain: restart the intro quest.
    if (QuestStates.IsEmpty() && !StartingQuestId.IsNone())
    {
        StartQuest(StartingQuestId);
    }
}
