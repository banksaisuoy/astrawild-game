#include "World/AstrawildDungeonSubsystem.h"

#include "Components/AstrawildInventoryComponent.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UAstrawildDungeonSubsystem::StartDungeon(const FName DungeonId, AActor* Starter)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
    {
        OnDungeonFailed.Broadcast(DungeonId, FText::FromString(TEXT("Dungeon start requires server authority.")));
        return false;
    }
    if (IsDungeonActive())
    {
        OnDungeonFailed.Broadcast(DungeonId, FText::FromString(TEXT("Another dungeon encounter is already active.")));
        return false;
    }

    const FAstrawildDungeonRow* Row = FindDungeonRow(DungeonId);
    if (!Row || !Starter)
    {
        OnDungeonFailed.Broadcast(DungeonId, FText::FromString(TEXT("Dungeon or starter is not configured.")));
        return false;
    }
    if (!CheckRequiredKey(*Row, Starter))
    {
        OnDungeonFailed.Broadcast(DungeonId, FText::FromString(TEXT("The required tower key is missing.")));
        return false;
    }
    if (Row->bConsumeRequiredKey && Row->RequiredKeyTag.IsValid())
    {
        UAstrawildInventoryComponent* Inventory = Starter->FindComponentByClass<UAstrawildInventoryComponent>();
        if (!Inventory || !Inventory->RemoveItem(Row->RequiredKeyTag, 1))
        {
            OnDungeonFailed.Broadcast(DungeonId, FText::FromString(TEXT("The required tower key could not be consumed.")));
            return false;
        }
    }

    ActiveDungeonId = DungeonId;
    ActiveDungeonStartTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    Participants.Reset();
    Participants.Add(Starter);
    OnDungeonStarted.Broadcast(ActiveDungeonId, Starter);
    return true;
}

bool UAstrawildDungeonSubsystem::JoinActiveDungeon(AActor* Participant)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
    {
        return false;
    }
    if (!Participant || !IsDungeonActive())
    {
        return false;
    }
    const FAstrawildDungeonRow* Row = FindDungeonRow(ActiveDungeonId);
    if (!Row || !Row->bSupportsCoop)
    {
        return false;
    }
    Participants.Add(Participant);
    return true;
}

bool UAstrawildDungeonSubsystem::RegisterBossDefeated(const FName DungeonId, AActor* DefeatedBoss)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
    {
        return false;
    }
    if (!IsDungeonActive() || DungeonId != ActiveDungeonId || !DefeatedBoss)
    {
        return false;
    }
    const FAstrawildDungeonRow* Row = FindDungeonRow(ActiveDungeonId);
    const AAstrawildEchoBase* DefeatedEcho = Cast<AAstrawildEchoBase>(DefeatedBoss);
    if (!Row || !DefeatedEcho || (Row->BossSpeciesTag.IsValid() && DefeatedEcho->InstanceData.SpeciesTag != Row->BossSpeciesTag))
    {
        return false;
    }
    if (GetRemainingTimeSeconds() <= 0.0f)
    {
        OnDungeonFailed.Broadcast(ActiveDungeonId, FText::FromString(TEXT("The dungeon time limit expired.")));
        StopDungeon();
        return false;
    }

    OnDungeonCompleted.Broadcast(ActiveDungeonId);
    StopDungeon();
    return true;
}

void UAstrawildDungeonSubsystem::StopDungeon()
{
    ActiveDungeonId = NAME_None;
    ActiveDungeonStartTimeSeconds = 0.0;
    Participants.Reset();
}

float UAstrawildDungeonSubsystem::GetRemainingTimeSeconds() const
{
    const FAstrawildDungeonRow* Row = FindDungeonRow(ActiveDungeonId);
    if (!Row || !GetWorld() || ActiveDungeonId.IsNone())
    {
        return 0.0f;
    }
    const double Elapsed = GetWorld()->GetTimeSeconds() - ActiveDungeonStartTimeSeconds;
    return FMath::Max(0.0f, Row->TimeLimitSeconds - static_cast<float>(Elapsed));
}

int32 UAstrawildDungeonSubsystem::GetParticipantCount() const
{
    int32 Count = 0;
    for (const TWeakObjectPtr<AActor>& Participant : Participants)
    {
        if (Participant.IsValid())
        {
            ++Count;
        }
    }
    return Count;
}

const FAstrawildDungeonRow* UAstrawildDungeonSubsystem::FindDungeonRow(const FName DungeonId) const
{
    if (!DungeonTable || DungeonId.IsNone())
    {
        return nullptr;
    }

    for (const TPair<FName, uint8*>& RowPair : DungeonTable->GetRowMap())
    {
        const FAstrawildDungeonRow* Row = reinterpret_cast<const FAstrawildDungeonRow*>(RowPair.Value);
        if (Row && Row->DungeonId == DungeonId)
        {
            return Row;
        }
    }
    return nullptr;
}

bool UAstrawildDungeonSubsystem::CheckRequiredKey(const FAstrawildDungeonRow& Row, AActor* Starter) const
{
    if (!Row.RequiredKeyTag.IsValid())
    {
        return true;
    }
    const UAstrawildInventoryComponent* Inventory = Starter ? Starter->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    return Inventory && Inventory->HasItem(Row.RequiredKeyTag, 1);
}
