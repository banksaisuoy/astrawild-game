#include "AstrawildEchoRosterSubsystem.h"

#include "AstrawildEchoCharacter.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

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

int32 UAstrawildEchoRosterSubsystem::SpawnPartyActors(APlayerController* Owner)
{
    // Audit H-2: load-path party respawn — previously LoadWorld only destroyed the live
    // party and re-imported roster DATA, so captured Echoes vanished from the world on load.
    UWorld* World = Owner ? Owner->GetWorld() : nullptr;
    if (!World || World->GetNetMode() == NM_Client)
    {
        return 0;
    }

    UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
    if (!Registry)
    {
        return 0;
    }

    const FVector Origin = Owner->GetPawn() ? Owner->GetPawn()->GetActorLocation() : FVector::ZeroVector;
    int32 Spawned = 0;
    SpawnedParty.Reset();

    for (const FAstrawildEchoInstanceV2& Entry : Roster)
    {
        if (static_cast<int32>(SpawnedParty.Num()) >= MaxPartySize)
        {
            break;
        }
        if (!Entry.bInParty || !Entry.InstanceId.IsValid() || Entry.DefinitionId.IsNone())
        {
            continue;
        }

        UAstrawildEchoDefinition* Definition = Registry->FindEcho(Entry.DefinitionId);
        if (!Definition)
        {
            UE_LOG(LogAstrawildAI, Warning, TEXT("Party respawn: species %s missing from registry."), *Entry.DefinitionId.ToString());
            continue;
        }

        // Ring placement around the player.
        const float Angle = (PI * 2.0f * Spawned) / FMath::Max(1, MaxPartySize);
        const FVector Location = Origin + FVector(FMath::Cos(Angle) * 250.0f, FMath::Sin(Angle) * 250.0f, 150.0f);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(
            AAstrawildEchoCharacter::StaticClass(), Location, FRotator::ZeroRotator, Params);
        if (Echo && Echo->InitializeFromDefinition(Definition, Entry.InstanceId) && Echo->FromSaveDataV2(Entry))
        {
            Echo->OwnerPlayerId = Owner->GetFName();
            Echo->IssueCommand(EAstrawildEchoCommand::Follow);
            SpawnedParty.Add(Echo);
            ++Spawned;
        }
        else if (Echo)
        {
            Echo->Destroy();
        }
    }

    if (Spawned > 0)
    {
        UE_LOG(LogAstrawildAI, Log, TEXT("Party respawned from roster: %d Echoes."), Spawned);
    }
    return Spawned;
}
