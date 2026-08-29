#include "AstrawildEchoRosterSubsystem.h"

#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"

bool UAstrawildEchoRosterSubsystem::AddToRoster(AAstrawildEchoCharacter* Echo)
{
    if (!IsValid(Echo) || !Echo->bCaptured || !Echo->InstanceId.IsValid())
    {
        return false;
    }

    if (IsInRoster(Echo->InstanceId))
    {
        return true;
    }

    Roster.Add(Echo->ToSaveDataV2());

    // Track spawned party actor (capped).
    if (SpawnedParty.Num() < MaxPartySize)
    {
        SpawnedParty.Add(Echo);
    }

    OnRosterChanged.Broadcast(Roster.Num());
    UE_LOG(LogAstrawildAI, Log, TEXT("Echo added to roster (size %d)."), Roster.Num());
    return true;
}

bool UAstrawildEchoRosterSubsystem::RemoveFromRoster(const FGuid& InstanceId)
{
    const int32 Removed = Roster.RemoveAll(
        [&InstanceId](const FAstrawildEchoInstanceV2& Item) { return Item.InstanceId == InstanceId; });
    if (Removed > 0)
    {
        OnRosterChanged.Broadcast(Roster.Num());
        return true;
    }
    return false;
}

bool UAstrawildEchoRosterSubsystem::IsInRoster(const FGuid& InstanceId) const
{
    return Roster.ContainsByPredicate(
        [&InstanceId](const FAstrawildEchoInstanceV2& Item) { return Item.InstanceId == InstanceId; });
}

TArray<FAstrawildEchoInstanceV2> UAstrawildEchoRosterSubsystem::GetRoster() const
{
    return Roster;
}

TArray<AAstrawildEchoCharacter*> UAstrawildEchoRosterSubsystem::GetSpawnedParty() const
{
    TArray<AAstrawildEchoCharacter*> Out;
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : SpawnedParty)
    {
        if (AAstrawildEchoCharacter* Echo = Weak.Get())
        {
            Out.Add(Echo);
        }
    }
    return Out;
}

void UAstrawildEchoRosterSubsystem::ExportForSave(TArray<FAstrawildEchoInstanceV2>& OutRoster) const
{
    // Refresh stored data from live party actors so transforms/needs are current.
    OutRoster = Roster;
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : SpawnedParty)
    {
        if (AAstrawildEchoCharacter* Echo = Weak.Get())
        {
            const FAstrawildEchoInstanceV2 Live = Echo->ToSaveDataV2();
            for (FAstrawildEchoInstanceV2& Entry : OutRoster)
            {
                if (Entry.InstanceId == Live.InstanceId)
                {
                    Entry = Live;
                    break;
                }
            }
        }
    }
}

void UAstrawildEchoRosterSubsystem::ImportFromSave(const TArray<FAstrawildEchoInstanceV2>& InRoster)
{
    Roster = InRoster;
    SpawnedParty.Reset();
    OnRosterChanged.Broadcast(Roster.Num());
}
