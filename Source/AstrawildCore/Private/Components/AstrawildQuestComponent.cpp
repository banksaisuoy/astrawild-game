#include "Components/AstrawildQuestComponent.h"

#include "Engine/DataTable.h"

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
    return QuestTable ? QuestTable->FindRow<FAstrawildQuestRow>(QuestId, TEXT("QuestLookup")) : nullptr;
}

const FAstrawildQuestObjectiveRow* UAstrawildQuestComponent::FindObjective(const FName QuestId, const FName ObjectiveId) const
{
    if (!ObjectiveTable)
    {
        return nullptr;
    }

    for (const TPair<FName, uint8*>& Pair : ObjectiveTable->GetRowMap())
    {
        const FAstrawildQuestObjectiveRow* Row = reinterpret_cast<const FAstrawildQuestObjectiveRow*>(Pair.Value);
        if (Row && Row->QuestId == QuestId && Row->ObjectiveId == ObjectiveId)
        {
            return Row;
        }
    }
    return nullptr;
}

bool UAstrawildQuestComponent::StartQuest(const FName QuestId)
{
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
    if (Amount <= 0 || !IsQuestActive(QuestId) || IsQuestCompleted(QuestId) || !FindObjective(QuestId, ObjectiveId))
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

bool UAstrawildQuestComponent::CompleteQuest(const FName QuestId)
{
    if (!IsQuestActive(QuestId) || !IsQuestComplete(QuestId))
    {
        return false;
    }

    ActiveQuestIds.Remove(QuestId);
    CompletedQuestIds.AddUnique(QuestId);
    OnQuestCompleted.Broadcast(QuestId);
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
    const FAstrawildQuestRow* Quest = FindQuest(QuestId);
    if (!Quest || Quest->Objectives.Num() == 0)
    {
        return false;
    }

    for (const FAstrawildQuestObjective& Objective : Quest->Objectives)
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
