#include "AstrawildEchoRosterSubsystem.h"

#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
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

// --- Content Pack CP-02: evolution / progression ---

bool UAstrawildEchoRosterSubsystem::CanEvolveInstance(const FAstrawildEchoInstanceV2& Instance,
    const UAstrawildEchoDefinition* Definition, const UAstrawildEchoDefinition* TargetDefinition)
{
    // Both definitions must resolve and the chain link must match — a data bug
    // (dangling EvolveToDefinitionId) fails closed instead of evolving into garbage.
    if (!Definition || !TargetDefinition || Definition->EvolveToDefinitionId.IsNone())
    {
        return false;
    }
    if (Definition->EvolveToDefinitionId != TargetDefinition->DefinitionId)
    {
        return false;
    }
    if (TargetDefinition->DefinitionId == Definition->DefinitionId)
    {
        return false; // No self-cycles — a chain must terminate.
    }

    // Dual gate: combat level AND bond. Evolution is a relationship milestone.
    return Instance.Level >= Definition->EvolveRequiredLevel
        && Instance.Bond >= Definition->EvolveRequiredBond;
}

bool UAstrawildEchoRosterSubsystem::CanEvolve(const FGuid& InstanceId) const
{
    const FAstrawildEchoInstanceV2* Entry = Roster.FindByPredicate(
        [&InstanceId](const FAstrawildEchoInstanceV2& Item) { return Item.InstanceId == InstanceId; });
    if (!Entry)
    {
        return false;
    }

    const UAstrawildItemRegistrySubsystem* Registry = GetGameInstance()
        ? GetGameInstance()->GetWorld() ? GetGameInstance()->GetWorld()->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr
        : nullptr;
    if (!Registry)
    {
        return false;
    }

    const UAstrawildEchoDefinition* Definition = Registry->FindEcho(Entry->DefinitionId);
    const UAstrawildEchoDefinition* Target = Registry->FindEcho(Definition ? Definition->EvolveToDefinitionId : NAME_None);
    return CanEvolveInstance(*Entry, Definition, Target);
}

bool UAstrawildEchoRosterSubsystem::EvolveInstance(const FGuid& InstanceId)
{
    FAstrawildEchoInstanceV2* Entry = Roster.FindByPredicate(
        [&InstanceId](const FAstrawildEchoInstanceV2& Item) { return Item.InstanceId == InstanceId; });
    if (!Entry)
    {
        return false;
    }

    UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!Registry)
    {
        return false;
    }

    UAstrawildEchoDefinition* Definition = Registry->FindEcho(Entry->DefinitionId);
    UAstrawildEchoDefinition* Target = Registry->FindEcho(Definition ? Definition->EvolveToDefinitionId : NAME_None);
    if (!CanEvolveInstance(*Entry, Definition, Target))
    {
        UE_LOG(LogAstrawildAI, Verbose, TEXT("Evolution rejected: gates not met for instance %s."), *InstanceId.ToString());
        return false;
    }

    // The transformation: species swaps, identity (level/bond/trust/personality) survives.
    Entry->DefinitionId = Target->DefinitionId;
    OnRosterChanged.Broadcast(Roster.Num());
    UE_LOG(LogAstrawildAI, Log, TEXT("Echo evolved: instance %s is now species %s (level %d, bond %.1f)."),
        *InstanceId.ToString(), *Target->DefinitionId.ToString(), Entry->Level, Entry->Bond);

    // Live party actor rebuilds from the new definition (stats + silhouette).
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : SpawnedParty)
    {
        if (AAstrawildEchoCharacter* Echo = Weak.Get())
        {
            if (Echo->InstanceId == InstanceId)
            {
                Echo->InitializeFromDefinition(Target, InstanceId);
                break;
            }
        }
    }
    return true;
}
