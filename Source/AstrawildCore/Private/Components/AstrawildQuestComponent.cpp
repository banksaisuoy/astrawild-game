#include "Components/AstrawildQuestComponent.h"

#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

UAstrawildQuestComponent::UAstrawildQuestComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

FName UAstrawildQuestComponent::MakeObjectiveKey(const FName QuestId, const FName ObjectiveId)
{
    return FName(*FString::Printf(TEXT("%s.%s"), *QuestId.ToString(), *ObjectiveId.ToString()));
}

const FAstrawildQuestRow* UAstrawildQuestComponent::FindQuest(const FName QuestId) const
{
    if (!QuestTable || QuestId.IsNone())
    {
        return nullptr;
    }

    if (const FAstrawildQuestRow* DirectRow = QuestTable->FindRow<FAstrawildQuestRow>(QuestId, TEXT("QuestLookup")))
    {
        return DirectRow;
    }

    for (const TPair<FName, uint8*>& Pair : QuestTable->GetRowMap())
    {
        const FAstrawildQuestRow* Row = reinterpret_cast<const FAstrawildQuestRow*>(Pair.Value);
        if (Row && Row->QuestId == QuestId)
        {
            return Row;
        }
    }
    return nullptr;
}

const FAstrawildQuestObjectiveRow* UAstrawildQuestComponent::FindObjective(const FName QuestId, const FName ObjectiveId) const
{
    if (ObjectiveTable)
    {
        for (const TPair<FName, uint8*>& Pair : ObjectiveTable->GetRowMap())
        {
            const FAstrawildQuestObjectiveRow* Row = reinterpret_cast<const FAstrawildQuestObjectiveRow*>(Pair.Value);
            if (Row && Row->QuestId == QuestId && Row->ObjectiveId == ObjectiveId)
            {
                return Row;
            }
        }
    }
    return nullptr;
}

bool UAstrawildQuestComponent::StartQuest(const FName QuestId)
{
    if (GetOwner() && !GetOwner()->HasAuthority())
    {
        return false;
    }

    const FAstrawildQuestRow* Quest = FindQuest(QuestId);
    if (!Quest || IsQuestActive(QuestId) || IsQuestCompleted(QuestId))
    {
        return false;
    }

    if (Quest->PrerequisiteQuestTag.IsValid())
    {
        const FName PrerequisiteId = FName(*Quest->PrerequisiteQuestTag.ToString());
        if (!IsQuestCompleted(PrerequisiteId))
        {
            return false;
        }
    }

    ActiveQuestIds.AddUnique(QuestId);
    OnQuestStarted.Broadcast(QuestId);
    return true;
}

bool UAstrawildQuestComponent::AddObjectiveProgress(const FName QuestId, const FName ObjectiveId, const int32 Amount)
{
    if (GetOwner() && !GetOwner()->HasAuthority())
    {
        return false;
    }
    if (Amount <= 0 || !IsQuestActive(QuestId) || IsQuestCompleted(QuestId))
    {
        return false;
    }

    TArray<FAstrawildQuestObjective> Definitions;
    GetObjectiveDefinitions(QuestId, Definitions);
    bool bObjectiveDefined = false;
    for (const FAstrawildQuestObjective& Definition : Definitions)
    {
        if (Definition.ObjectiveId == ObjectiveId)
        {
            bObjectiveDefined = true;
            break;
        }
    }
    if (!bObjectiveDefined)
    {
        return false;
    }

    const FName Key = MakeObjectiveKey(QuestId, ObjectiveId);
    ObjectiveProgress.FindOrAdd(Key) = FMath::Max(0, ObjectiveProgress.FindRef(Key) + Amount);
    OnQuestUpdated.Broadcast(QuestId);

    if (IsQuestComplete(QuestId))
    {
        CompleteQuest(QuestId);
    }
    return true;
}

int32 UAstrawildQuestComponent::AddProgressForTarget(const EAstrawildQuestObjectiveType ObjectiveType, const FGameplayTag& TargetTag, const int32 Amount)
{
    if ((GetOwner() && !GetOwner()->HasAuthority()) || Amount <= 0)
    {
        return 0;
    }

    TArray<TPair<FName, FName>> Matches;
    for (const FName QuestId : ActiveQuestIds)
    {
        TArray<FAstrawildQuestObjective> Objectives;
        GetObjectiveDefinitions(QuestId, Objectives);
        for (const FAstrawildQuestObjective& Objective : Objectives)
        {
            const bool bWildcardTarget = Objective.TargetTag.IsValid() && Objective.TargetTag.ToString().EndsWith(TEXT(".Any"));
            const bool bTargetMatches = bWildcardTarget || (!Objective.TargetTag.IsValid() || !TargetTag.IsValid()
                ? !Objective.TargetTag.IsValid() && !TargetTag.IsValid()
                : Objective.TargetTag == TargetTag || Objective.TargetTag.MatchesTag(TargetTag) || TargetTag.MatchesTag(Objective.TargetTag));
            if (Objective.Type == ObjectiveType && bTargetMatches)
            {
                Matches.Emplace(QuestId, Objective.ObjectiveId);
            }
        }
    }

    int32 ProgressedCount = 0;
    for (const TPair<FName, FName>& Match : Matches)
    {
        if (AddObjectiveProgress(Match.Key, Match.Value, Amount))
        {
            ++ProgressedCount;
        }
    }
    return ProgressedCount;
}

bool UAstrawildQuestComponent::CompleteQuest(const FName QuestId)
{
    if ((GetOwner() && !GetOwner()->HasAuthority()) || !IsQuestActive(QuestId) || !IsQuestComplete(QuestId))
    {
        return false;
    }

    ActiveQuestIds.Remove(QuestId);
    CompletedQuestIds.AddUnique(QuestId);
    OnQuestCompleted.Broadcast(QuestId);

    if (bAutoStartDependentQuests && QuestTable)
    {
        TArray<FName> DependentQuestIds;
        for (const TPair<FName, uint8*>& Pair : QuestTable->GetRowMap())
        {
            const FAstrawildQuestRow* Candidate = reinterpret_cast<const FAstrawildQuestRow*>(Pair.Value);
            if (Candidate && Candidate->PrerequisiteQuestTag.IsValid() && Candidate->PrerequisiteQuestTag.ToString() == QuestId.ToString())
            {
                DependentQuestIds.AddUnique(Candidate->QuestId);
            }
        }
        for (const FName DependentQuestId : DependentQuestIds)
        {
            StartQuest(DependentQuestId);
        }
    }
    return true;
}

bool UAstrawildQuestComponent::IsQuestActive(const FName QuestId) const
{
    return ActiveQuestIds.Contains(QuestId);
}

bool UAstrawildQuestComponent::IsQuestCompleted(const FName QuestId) const
{
    return CompletedQuestIds.Contains(QuestId);
}

bool UAstrawildQuestComponent::IsQuestComplete(const FName QuestId) const
{
    TArray<FAstrawildQuestObjective> Objectives;
    GetObjectiveDefinitions(QuestId, Objectives);
    if (Objectives.Num() == 0)
    {
        return false;
    }

    for (const FAstrawildQuestObjective& Objective : Objectives)
    {
        if (GetObjectiveProgress(QuestId, Objective.ObjectiveId) < Objective.RequiredQuantity)
        {
            return false;
        }
    }
    return true;
}

int32 UAstrawildQuestComponent::GetObjectiveProgress(const FName QuestId, const FName ObjectiveId) const
{
    return ObjectiveProgress.FindRef(MakeObjectiveKey(QuestId, ObjectiveId));
}

void UAstrawildQuestComponent::GetObjectiveDefinitions(const FName QuestId, TArray<FAstrawildQuestObjective>& OutObjectives) const
{
    OutObjectives.Reset();
    const FAstrawildQuestRow* Quest = FindQuest(QuestId);
    if (!Quest)
    {
        return;
    }

    OutObjectives = Quest->Objectives;
    if (OutObjectives.Num() > 0 || !ObjectiveTable)
    {
        return;
    }

    for (const TPair<FName, uint8*>& Pair : ObjectiveTable->GetRowMap())
    {
        const FAstrawildQuestObjectiveRow* Row = reinterpret_cast<const FAstrawildQuestObjectiveRow*>(Pair.Value);
        if (!Row || Row->QuestId != QuestId)
        {
            continue;
        }
        FAstrawildQuestObjective& Objective = OutObjectives.AddDefaulted_GetRef();
        Objective.ObjectiveId = Row->ObjectiveId;
        Objective.Type = Row->Type;
        Objective.TargetTag = Row->TargetTag;
        Objective.RequiredQuantity = FMath::Max(1, Row->RequiredQuantity);
        Objective.Description = Row->Description;
    }
}

bool UAstrawildQuestComponent::GetQuestData(const FName QuestId, FAstrawildQuestRow& OutQuest) const
{
    const FAstrawildQuestRow* Quest = FindQuest(QuestId);
    if (!Quest)
    {
        return false;
    }
    OutQuest = *Quest;
    return true;
}

void UAstrawildQuestComponent::ExportToProfile(FAstrawildPlayerProfile& OutProfile) const
{
    OutProfile.ActiveQuestIds = ActiveQuestIds;
    OutProfile.CompletedQuestIds = CompletedQuestIds;
    OutProfile.ObjectiveProgress = ObjectiveProgress;
}

void UAstrawildQuestComponent::ImportFromProfile(const FAstrawildPlayerProfile& InProfile)
{
    ActiveQuestIds = InProfile.ActiveQuestIds;
    CompletedQuestIds = InProfile.CompletedQuestIds;
    ObjectiveProgress = InProfile.ObjectiveProgress;
}
