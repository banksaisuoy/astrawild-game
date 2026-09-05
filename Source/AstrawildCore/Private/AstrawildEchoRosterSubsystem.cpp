#include "AstrawildEchoRosterSubsystem.h"

#include "AstrawildPlayerController.h" // LCP-4: owner key

#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"

bool UAstrawildEchoRosterSubsystem::AddToRoster(AAstrawildEchoCharacter* Echo, const FName PlayerKey)
{
    if (!IsValid(Echo) || !Echo->bCaptured || !Echo->InstanceId.IsValid())
    {
        return false;
    }

    if (IsInRoster(Echo->InstanceId))
    {
        return true;
    }

    // Final-audit L-5: bounded roster — the array is exported wholesale into the
    // save; a capture-happy session (or a crafted import) must not grow it forever.
    static constexpr int32 MaxRosterSize = 100;
    if (Roster.Num() >= MaxRosterSize)
    {
        UE_LOG(LogAstrawildAI, Warning, TEXT("AddToRoster: roster at cap (%d) — capture refused."), MaxRosterSize);
        return false;
    }

    FAstrawildEchoInstanceV2 NewEntry = Echo->ToSaveDataV2();
    // LCP-4: stamp the STABLE owner key (the live actor's OwnerPlayerId stays
    // the pawn-name convention the H-1 consumers compare against).
    NewEntry.OwnerPlayerKey = PlayerKey;
    Roster.Add(NewEntry);

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

int32 UAstrawildEchoRosterSubsystem::GetBaseGarrisonCount() const
{
    // SCP Phase 9: the garrison is every spawned, healthy party echo that is
    // currently assigned to a base work site (the Base Terminal cap pool).
    int32 Count = 0;
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : SpawnedParty)
    {
        const AAstrawildEchoCharacter* Echo = Weak.Get();
        if (Echo && IsValid(Echo) && !Echo->IsDefeated() && Echo->AssignedWorkSite.IsValid())
        {
            ++Count;
        }
    }
    return Count;
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
    // FR-4 (Final Run redo): save-import sanitize. A crafted/corrupt save could
    // import duplicated InstanceIds verbatim (doubling party entries) or entries
    // with a broken guid / missing species that would fail every later lookup.
    // First-seen-wins on guid; invalid entries are dropped; every repair is logged.
    Roster.Reset();
    for (const FAstrawildEchoInstanceV2& Entry : InRoster)
    {
        if (!Entry.InstanceId.IsValid() || Entry.DefinitionId.IsNone())
        {
            UE_LOG(LogAstrawildAI, Warning, TEXT("ImportFromSave: dropped invalid roster entry (guid valid: %s, species %s)."),
                Entry.InstanceId.IsValid() ? TEXT("yes") : TEXT("no"), *Entry.DefinitionId.ToString());
            continue;
        }
        if (Roster.ContainsByPredicate(
            [&Entry](const FAstrawildEchoInstanceV2& Item) { return Item.InstanceId == Entry.InstanceId; }))
        {
            UE_LOG(LogAstrawildAI, Warning, TEXT("ImportFromSave: duplicate instance %s — first entry wins."),
                *Entry.InstanceId.ToString());
            continue;
        }
        Roster.Add(Entry);
    }
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

    // LCP-4: each player's party spawns from THEIR roster slice. The legacy
    // NAME_None key (single-player / pre-LCP saves) keeps the whole-legacy-pool
    // behavior — host/standalone flows are byte-identical.
    const AAstrawildPlayerController* AstrawildOwner = Cast<AAstrawildPlayerController>(Owner);
    const FName OwnerKey = AstrawildOwner ? AstrawildOwner->GetPlayerKey() : NAME_None;

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
        if (!IsRosterRowOwnedBy(Entry, OwnerKey))
        {
            continue; // another player's Echo — never spawns in this party ring
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
            // Final-audit H-1: OwnerPlayerId must match the PAWN name — every
            // consumer (party passives EchoCharacter.cpp, command cycling
            // PlayerCharacter.cpp, work assignment WorkSiteActor.cpp, combat
            // owner-exclusion EchoAIController.cpp) compares against the pawn set
            // at capture. The controller name used to be written here instead,
            // silently killing party passives/commands after every save/load and
            // letting Attack-commanded echoes re-target their own player.
            Echo->OwnerPlayerId = (Owner && Owner->GetPawn()) ? Owner->GetPawn()->GetFName() : NAME_None;
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


bool UAstrawildEchoRosterSubsystem::IsRosterRowOwnedBy(const FAstrawildEchoInstanceV2& Row, const FName PlayerKey)
{
    // Exact key match, or the legacy pair (undefined row + undefined query) —
    // single-player saves load with NAME_None rows and the host's query key is
    // never None in co-op, so legacy rows only ever match the legacy query.
    return Row.OwnerPlayerKey == PlayerKey;
}

TArray<FAstrawildEchoInstanceV2> UAstrawildEchoRosterSubsystem::GetRosterForPlayer(const FName PlayerKey) const
{
    TArray<FAstrawildEchoInstanceV2> Out;
    for (const FAstrawildEchoInstanceV2& Row : Roster)
    {
        if (IsRosterRowOwnedBy(Row, PlayerKey))
        {
            Out.Add(Row);
        }
    }
    return Out;
}
