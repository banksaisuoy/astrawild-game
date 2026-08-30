#include "AstrawildDungeonGeneratorActor.h"

#include "AstrawildCore.h"
#include "AstrawildDungeonGateActor.h"
#include "AstrawildGameState.h"
#include "AstrawildLog.h"
#include "AstrawildResearchSubsystem.h"
#include "Engine/World.h"

AAstrawildDungeonGeneratorActor::AAstrawildDungeonGeneratorActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    // Default creature pool — first dungeon: gloomfang pack (CODE_DEFAULT content).
    CreaturePoolIds = { TEXT("Echo_Gloomfang"), TEXT("Echo_Gloomfang"), TEXT("Echo_Stonehide") };
}

void AAstrawildDungeonGeneratorActor::BeginPlay()
{
    Super::BeginPlay();

    if (GetLocalRole() == ROLE_Authority)
    {
        Generate();
    }
}

FAstrawildDungeonRoomTemplate AAstrawildDungeonGeneratorActor::MakeTemplate(const int32 RoomIndex) const
{
    // Hand-authored chain layout (directive §23): Entry -> Combat -> Puzzle -> Elite -> Boss.
    FAstrawildDungeonRoomTemplate Template;

    if (RoomIndex == 0)
    {
        Template.RoomTypeId = TEXT("Entry");
        Template.HalfExtents = FVector(500.0f, 500.0f, 300.0f);
        Template.CreatureSpawnOffsets.Reset(); // Safe entry.
    }
    else if (RoomIndex == RoomCount - 1)
    {
        Template.RoomTypeId = TEXT("Boss");
        Template.HalfExtents = FVector(900.0f, 900.0f, 400.0f);
        Template.bIsBossRoom = true;
        Template.CreatureSpawnOffsets = { FVector(-300.0f, -300.0f, 120.0f) };
        Template.ClearLootTableId = TEXT("Loot_DungeonBoss"); // Wave 3: boss loot table.
    }
    else if (RoomIndex == RoomCount - 2)
    {
        Template.RoomTypeId = TEXT("Elite");
        Template.HalfExtents = FVector(700.0f, 700.0f, 350.0f);
        Template.CreatureSpawnOffsets = { FVector(-250.0f, 0.0f, 120.0f), FVector(250.0f, 0.0f, 120.0f) };
    }
    else if (RoomIndex == RoomCount - 3)
    {
        Template.RoomTypeId = TEXT("Puzzle");
        Template.HalfExtents = FVector(600.0f, 600.0f, 300.0f);
        Template.CreatureSpawnOffsets = { FVector(0.0f, 300.0f, 120.0f) }; // Light guard.
    }
    else
    {
        Template.RoomTypeId = TEXT("Combat");
        Template.HalfExtents = FVector(650.0f, 650.0f, 320.0f);
        Template.CreatureSpawnOffsets = { FVector(-200.0f, -200.0f, 120.0f), FVector(200.0f, 200.0f, 120.0f) };
    }

    return Template;
}

void AAstrawildDungeonGeneratorActor::Generate()
{
    UWorld* World = GetWorld();
    if (!World || GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    // Deterministic from the replicated world seed.
    if (const AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
    {
        RandomStream = FRandomStream(GameState->WorldSeed + 777);
    }
    else
    {
        RandomStream = FRandomStream(777);
    }

    // Clear previous generation (regeneration support).
    for (AAstrawildDungeonRoomActor* Room : Rooms)
    {
        if (IsValid(Room))
        {
            Room->Destroy();
        }
    }
    Rooms.Reset();
    RoomsCleared = 0;

    for (AAstrawildDungeonGateActor* Gate : Gates)
    {
        if (IsValid(Gate))
        {
            Gate->Destroy();
        }
    }
    Gates.Reset();

    TArray<FVector> RoomCenters;

    const int32 Count = FMath::Clamp(RoomCount, 3, 12);
    for (int32 i = 0; i < Count; ++i)
    {
        FAstrawildDungeonRoomTemplate Template = MakeTemplate(i);

        // Slight lateral offset per room so the chain feels like a real layout, not a corridor.
        const float Lateral = (i % 2 == 0 ? 1.0f : -1.0f) * RandomStream.FRandRange(0.0f, 400.0f);
        const FVector RoomCenter = GetActorLocation() + FVector(i * RoomSpacing, Lateral, 0.0f);
        RoomCenters.Add(RoomCenter);

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AAstrawildDungeonRoomActor* Room = World->SpawnActor<AAstrawildDungeonRoomActor>(
            AAstrawildDungeonRoomActor::StaticClass(), RoomCenter, FRotator::ZeroRotator, Params);
        if (!Room)
        {
            RoomCenters.Pop();
            continue;
        }

        Room->Template = Template;
        Room->RoomIndex = i;
        Room->OnRoomCleared.AddDynamic(this, &AAstrawildDungeonGeneratorActor::HandleRoomCleared);
        // Audit fix: BeginPlay ran with the default template — rebuild the shell with real extents.
        Room->RefreshRoomShell();
        Rooms.Add(Room);

        // Boss room uses the boss id; other rooms cycle the pool (entry spawns nothing).
        if (Template.bIsBossRoom)
        {
            Room->BossDefeatEventId = BossDefeatEventId; // Batch 8: per-dungeon quest target.
            Room->SpawnEncounter({ BossDefinitionId });
        }
        else if (!Template.CreatureSpawnOffsets.IsEmpty())
        {
            Room->SpawnEncounter(CreaturePoolIds);
        }

        // Audit C-4: rooms with no encounter (the entry) clear immediately — previously
        // they could never satisfy the clear check, so OnDungeonCompleted never fired.
        if (!Room->HasEncounter())
        {
            Room->MarkCleared();
        }
    }

    // Gates (Batch 6 — Item A): gate i seals the passage between room i and room i+1;
    // it opens when room i clears. Sealed by default — progression is now physical.
    for (int32 i = 0; i < Rooms.Num() - 1; ++i)
    {
        if (!Rooms[i] || !Rooms[i + 1] || RoomCenters.Num() <= i + 1)
        {
            continue;
        }

        const FVector Midpoint = (RoomCenters[i] + RoomCenters[i + 1]) * 0.5f;
        FActorSpawnParameters GateParams;
        GateParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AAstrawildDungeonGateActor* Gate = World->SpawnActor<AAstrawildDungeonGateActor>(
            AAstrawildDungeonGateActor::StaticClass(), Midpoint, FRotator::ZeroRotator, GateParams);
        if (Gate)
        {
            Gate->SealGate();
            Gates.Add(Gate);
        }
    }

    UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon generated: %d rooms, %d gates (entry->combat->puzzle->elite->boss)."),
        Rooms.Num(), Gates.Num());
    OnDungeonProgress.Broadcast(RoomsCleared, Rooms.Num());
}

void AAstrawildDungeonGeneratorActor::HandleRoomCleared(AAstrawildDungeonRoomActor* Room, const int32 ClearedRoomIndex)
{
    ++RoomsCleared;
    OnDungeonProgress.Broadcast(RoomsCleared, Rooms.Num());

    // Batch 6 — Item A: clearing room i unseals the gate into room i+1.
    if (ClearedRoomIndex >= 0 && ClearedRoomIndex < Gates.Num() && Gates[ClearedRoomIndex])
    {
        Gates[ClearedRoomIndex]->OpenGate();
    }

    if (RoomsCleared >= Rooms.Num())
    {
        UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon completed — all %d rooms cleared."), Rooms.Num());

        // Audit C-4: completion reward — research points for the shared pool so dungeon
        // runs feed the technology loop (previously the completion event had no consumer).
        UWorld* World = GetWorld();
        if (World && World->GetGameInstance())
        {
            if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
            {
                Research->AddResearchPoints(DungeonCompletionResearchPoints);

                // Batch 6: the unique Ancient-era technology — force-unlocked, bypassing
                // cost and prerequisites (roadmap V3 §21 unique boss technology reward).
                // Idempotent: already-unlocked (e.g. researched normally) is a no-op.
                if (!RewardTechnologyId.IsNone() && !Research->IsTechUnlocked(RewardTechnologyId))
                {
                    Research->ForceUnlockTech(RewardTechnologyId);
                }
            }
        }

        OnDungeonCompleted.Broadcast(this);
    }
}

FAstrawildDungeonSaveData AAstrawildDungeonGeneratorActor::ExportForSave() const
{
    FAstrawildDungeonSaveData Data;
    Data.DungeonId = DungeonId;
    Data.TotalRooms = Rooms.Num();
    for (const AAstrawildDungeonRoomActor* Room : Rooms)
    {
        if (Room && Room->bCleared)
        {
            Data.ClearedRoomIndices.Add(Room->RoomIndex);
        }
    }
    Data.RoomsCleared = Data.ClearedRoomIndices.Num();
    Data.bCompleted = Rooms.Num() > 0 && Data.RoomsCleared >= Rooms.Num();
    return Data;
}

void AAstrawildDungeonGeneratorActor::ApplySavedState(const FAstrawildDungeonSaveData& Data)
{
    if (GetLocalRole() != ROLE_Authority || Data.DungeonId != DungeonId)
    {
        return;
    }

    int32 Restored = 0;
    for (AAstrawildDungeonRoomActor* Room : Rooms)
    {
        if (Room && Data.ClearedRoomIndices.Contains(Room->RoomIndex))
        {
            const bool bWasCleared = Room->bCleared;
            Room->RestoreClearedState();
            if (!bWasCleared)
            {
                ++Restored;
            }
        }
    }

    // Generator-level counter comes from the record — restored rooms deliberately
    // skip the OnRoomCleared broadcast so completion rewards never double-fire.
    RoomsCleared = FMath::Clamp(Data.RoomsCleared, 0, Rooms.Num());

    // Reopen gates whose controlling room is cleared (matches live HandleRoomCleared).
    for (int32 i = 0; i < Gates.Num(); ++i)
    {
        if (Gates[i] && Data.ClearedRoomIndices.Contains(i))
        {
            Gates[i]->OpenGate();
        }
    }

    OnDungeonProgress.Broadcast(RoomsCleared, Rooms.Num());
    UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon %s restored from save: %d/%d rooms cleared (%d freshly restored, %d gates reopened)."),
        *DungeonId.ToString(), RoomsCleared, Rooms.Num(), Restored,
        static_cast<int32>(Data.ClearedRoomIndices.Num()));
}
