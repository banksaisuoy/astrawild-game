#include "AstrawildWorldEventSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildGameState.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildResourceNode.h"
#include "AstrawildTerrainTileActor.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildWeatherSubsystem.h"
#include "AstrawildZoneSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void UAstrawildWorldEventSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Server-only scheduling: client worlds never roll or resolve events.
    const UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    const int32 AbsoluteMinute = GetAbsoluteMinute();
    if (AbsoluteMinute < 0)
    {
        return; // Time subsystem not ready yet.
    }

    if (!bScheduleInitialized)
    {
        InitializeSchedule();
        return;
    }

    EndExpiredEvents(AbsoluteMinute);

    if (AbsoluteMinute >= NextRollAbsoluteMinute)
    {
        RunRoll();
    }
}

void UAstrawildWorldEventSubsystem::InitializeSchedule()
{
    // First event window: mid-morning of day 1 (in-world minute 540 = 09:00).
    // Seeded jitter of ±60 minutes keeps different worlds on different clocks
    // while remaining fully deterministic per seed.
    const AAstrawildGameState* GameState = GetGameState();
    const int32 Seed = GameState ? GameState->WorldSeed : 1337;
    FRandomStream Stream(Seed ^ 0x57E17);
    NextRollAbsoluteMinute = 540 + Stream.RandRange(-60, 60);
    bScheduleInitialized = true;
    UE_LOG(LogAstrawild, Log, TEXT("World-event scheduler armed — first roll at absolute minute %d."), NextRollAbsoluteMinute);
}

int32 UAstrawildWorldEventSubsystem::GetAbsoluteMinute() const
{
    UAstrawildTimeSubsystem* Time = GetTime();
    if (!Time)
    {
        return -1;
    }
    // Day 1 minute 0 = absolute 0; 1440 minutes per in-world day.
    return (Time->GetCurrentDay() - 1) * 1440 + Time->GetCurrentMinute();
}

UAstrawildTimeSubsystem* UAstrawildWorldEventSubsystem::GetTime() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildTimeSubsystem>() : nullptr;
}

AAstrawildGameState* UAstrawildWorldEventSubsystem::GetGameState() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetGameState<AAstrawildGameState>() : nullptr;
}

TArray<FName> UAstrawildWorldEventSubsystem::GetActiveEventIds() const
{
    TArray<FName> Out;
    for (const FAstrawildWorldEventSaveData& Runtime : ActiveEvents)
    {
        Out.Add(Runtime.EventId);
    }
    return Out;
}

FText UAstrawildWorldEventSubsystem::GetActiveEventSummaryText() const
{
    if (ActiveEvents.IsEmpty())
    {
        return FText::FromString(TEXT(""));
    }
    const UWorld* World = GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    TArray<FString> Names;
    for (const FAstrawildWorldEventSaveData& Runtime : ActiveEvents)
    {
        const UAstrawildWorldEventDefinition* Def = Registry ? Registry->FindWorldEvent(Runtime.EventId) : nullptr;
        Names.Add(Def ? Def->DisplayName.ToString() : Runtime.EventId.ToString());
    }
    return FText::FromString(FString::Join(Names, TEXT(" | ")));
}

int32 UAstrawildWorldEventSubsystem::GetCooldownRemainingMinutes(const FName EventId) const
{
    const int32 CurrentMinute = GetAbsoluteMinute();
    const int32* End = CooldownEndMinutes.Find(EventId);
    return (End && CurrentMinute >= 0) ? FMath::Max(0, *End - CurrentMinute) : 0;
}

bool UAstrawildWorldEventSubsystem::IsEventEligible(const UAstrawildWorldEventDefinition* Definition,
    const int32 CurrentAbsoluteMinute, const int32 CurrentDay, const int32 CurrentHour,
    const int32 ActiveEventCount, const TMap<FName, int32>& CooldownEndMinutes, const int32 MaxConcurrent)
{
    if (!IsValid(Definition) || Definition->EventId.IsNone() || Definition->RarityWeight <= 0.0f)
    {
        return false;
    }
    if (ActiveEventCount >= MaxConcurrent)
    {
        return false;
    }
    if (CurrentDay < FMath::Max(1, Definition->MinDay))
    {
        return false;
    }
    if (Definition->bRequiresNight && (CurrentHour >= 6 && CurrentHour < 21))
    {
        return false;
    }
    if (const int32* CooldownEnd = CooldownEndMinutes.Find(Definition->EventId))
    {
        if (CurrentAbsoluteMinute < *CooldownEnd)
        {
            return false;
        }
    }
    return true;
}

FName UAstrawildWorldEventSubsystem::PickWeightedEvent(const TArray<UAstrawildWorldEventDefinition*>& Pool,
    const TMap<FName, int32>& CooldownEndMinutes, const int32 CurrentAbsoluteMinute, const int32 CurrentDay,
    const int32 CurrentHour, const int32 ActiveEventCount, const int32 MaxConcurrent, FRandomStream& Stream)
{
    // Deterministic weighted pick over the eligible pool (unit-tested shape).
    TArray<const UAstrawildWorldEventDefinition*> Eligible;
    float TotalWeight = 0.0f;
    for (const UAstrawildWorldEventDefinition* Def : Pool)
    {
        if (IsEventEligible(Def, CurrentAbsoluteMinute, CurrentDay, CurrentHour, ActiveEventCount, CooldownEndMinutes, MaxConcurrent))
        {
            Eligible.Add(Def);
            TotalWeight += FMath::Max(0.0f, Def->RarityWeight);
        }
    }
    if (Eligible.IsEmpty() || TotalWeight <= 0.0f)
    {
        return NAME_None;
    }
    float Roll = Stream.FRandRange(0.0f, TotalWeight);
    for (const UAstrawildWorldEventDefinition* Def : Eligible)
    {
        Roll -= FMath::Max(0.0f, Def->RarityWeight);
        if (Roll <= 0.0f)
        {
            return Def->EventId;
        }
    }
    return Eligible.Last()->EventId;
}

void UAstrawildWorldEventSubsystem::RunRoll()
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    UAstrawildTimeSubsystem* Time = GetTime();
    if (!World || !Registry || !Time)
    {
        return;
    }

    const AAstrawildGameState* GameState = GetGameState();
    const int32 Seed = GameState ? GameState->WorldSeed : 1337;
    // Roll index keeps the stream advancing without persisting stream state:
    // roll number N is derived from (seed, NextRollAbsoluteMinute).
    FRandomStream Stream(Seed ^ static_cast<uint32>(NextRollAbsoluteMinute));
    const int32 AbsoluteMinute = GetAbsoluteMinute();

    const TArray<UAstrawildWorldEventDefinition*> Pool = Registry->GetAllWorldEvents();
    const FName Picked = PickWeightedEvent(Pool, CooldownEndMinutes, AbsoluteMinute,
        Time->GetCurrentDay(), Time->GetCurrentMinute() / 60,
        ActiveEvents.Num(), MaxConcurrentEvents, Stream);

    // Schedule the next roll regardless of the pick (the world keeps breathing).
    NextRollAbsoluteMinute = AbsoluteMinute + RollIntervalMinutes + Stream.RandRange(-30, 30);

    if (Picked.IsNone())
    {
        return; // Nothing eligible — quiet world for now.
    }

    if (const UAstrawildWorldEventDefinition* Definition = Registry->FindWorldEvent(Picked))
    {
        StartEvent(Definition);
    }
}

bool UAstrawildWorldEventSubsystem::StartEvent(const UAstrawildWorldEventDefinition* Definition)
{
    UWorld* World = GetWorld();
    UAstrawildTimeSubsystem* Time = GetTime();
    if (!World || !Time || !IsValid(Definition))
    {
        return false;
    }

    const int32 AbsoluteMinute = GetAbsoluteMinute();

    FAstrawildWorldEventSaveData Runtime;
    Runtime.EventId = Definition->EventId;
    Runtime.EndAbsoluteMinute = Definition->DurationGameMinutes > 0 ? AbsoluteMinute + Definition->DurationGameMinutes : 0;
    Runtime.Zone = Definition->Zone;

    // Anchored events get a deterministic location inside their zone.
    const FAstrawildZoneDescriptor* ZoneDesc = UAstrawildZoneSubsystem::FindZone(Definition->Zone);
    if (ZoneDesc)
    {
        FRandomStream Stream((World->GetGameState<AAstrawildGameState>() ? World->GetGameState<AAstrawildGameState>()->WorldSeed : 1337) ^ static_cast<uint32>(AbsoluteMinute));
        const FVector2D Center = ZoneDesc->GetCenter();
        Runtime.Location = FVector(
            Center.X + Stream.FRandRange(-ZoneDesc->GetSizeX() * 0.3f, ZoneDesc->GetSizeX() * 0.3f),
            Center.Y + Stream.FRandRange(-ZoneDesc->GetSizeY() * 0.3f, ZoneDesc->GetSizeY() * 0.3f), 0.0f);
    }
    else
    {
        Runtime.Location = FVector::ZeroVector;
    }

    ActiveEvents.Add(Runtime);
    CooldownEndMinutes.Add(Definition->EventId, AbsoluteMinute + FMath::Max(0.0f, Definition->CooldownGameHours) * 60);

    ResolveEventEffects(Definition, Runtime);

    if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
    {
        EventBus->PublishEvent(TAG_Astrawild_Event_WorldEventStarted, nullptr, Definition->EventId, 1, Runtime.Location);
    }
    OnWorldEventStateChanged.Broadcast(Definition->EventId, true);
    BroadcastToast(FText::FromString(FString::Printf(TEXT("WORLD EVENT: %s"), *Definition->DisplayName.ToString())));

    UE_LOG(LogAstrawild, Log, TEXT("World event started: %s (ends minute %d)."), *Definition->EventId.ToString(), Runtime.EndAbsoluteMinute);

    // Instant events (duration 0) end immediately after resolving.
    if (Runtime.EndAbsoluteMinute == 0)
    {
        EndEvent(Definition->EventId);
    }
    return true;
}

void UAstrawildWorldEventSubsystem::ResolveEventEffects(const UAstrawildWorldEventDefinition* Definition, const FAstrawildWorldEventSaveData& Runtime)
{
    // Every effect is data — no per-event subclasses (Master Plan §19).
    UWorld* World = GetWorld();
    if (!World || !IsValid(Definition))
    {
        return;
    }

    // 1) Forced weather (storm surges etc.).
    if (Definition->bForcesWeather)
    {
        if (UAstrawildWeatherSubsystem* Weather = World->GetSubsystem<UAstrawildWeatherSubsystem>())
        {
            Weather->ForceWeather(Definition->ForcedWeather);
        }
    }

    // 2) Research rewards (ancient signals pay knowledge).
    if (Definition->ResearchPointReward > 0 && World->GetGameInstance())
    {
        if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->AddResearchPoints(Definition->ResearchPointReward);
        }
    }

    // 3) Bonus resource nodes (resource surge / meteor crater).
    if (!Definition->BonusNodeIds.IsEmpty() && !Runtime.Location.IsNearlyZero())
    {
        const AAstrawildGameState* GameState = GetGameState();
        const int32 Seed = GameState ? GameState->WorldSeed : 1337;
        FRandomStream Stream(Seed ^ 0xB100 ^ static_cast<uint32>(GetAbsoluteMinute()));
        for (const FName NodeId : Definition->BonusNodeIds)
        {
            const FVector2D Offset(Stream.FRandRange(-800.0f, 800.0f), Stream.FRandRange(-800.0f, 800.0f));
            const FVector2D XY(Runtime.Location.X + Offset.X, Runtime.Location.Y + Offset.Y);
            const float GroundZ = AAstrawildTerrainTileActor::EvalWorldHeight(XY, Seed);
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
            if (AAstrawildResourceNode* Node = World->SpawnActor<AAstrawildResourceNode>(
                AAstrawildResourceNode::StaticClass(), FVector(XY.X, XY.Y, GroundZ + 60.0f), FRotator::ZeroRotator, Params))
            {
                Node->NodeDefinitionId = NodeId;
            }
        }
    }

    // 4) Species boost (migration / rare bloom) — extra wild spawns in the zone.
    if (!Definition->SpeciesBoostId.IsNone() && Definition->SpeciesBoostCount > 0)
    {
        const UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
        UAstrawildEchoDefinition* SpeciesDef = Registry ? Registry->FindEcho(Definition->SpeciesBoostId) : nullptr;
        const FAstrawildZoneDescriptor* ZoneDesc = UAstrawildZoneSubsystem::FindZone(Runtime.Zone != EAstrawildZone::None ? Runtime.Zone : Definition->Zone);
        if (SpeciesDef && ZoneDesc)
        {
            const AAstrawildGameState* GameState = GetGameState();
            const int32 Seed = GameState ? GameState->WorldSeed : 1337;
            FRandomStream Stream(Seed ^ 0xB100A0u ^ static_cast<uint32>(GetAbsoluteMinute()));
            const FVector2D Center = ZoneDesc->GetCenter();
            for (int32 i = 0; i < Definition->SpeciesBoostCount; ++i)
            {
                const FVector2D Point(
                    Center.X + Stream.FRandRange(-ZoneDesc->GetSizeX() * 0.3f, ZoneDesc->GetSizeX() * 0.3f),
                    Center.Y + Stream.FRandRange(-ZoneDesc->GetSizeY() * 0.3f, ZoneDesc->GetSizeY() * 0.3f));
                const float GroundZ = AAstrawildTerrainTileActor::EvalWorldHeight(Point, Seed);
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
                if (AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(
                    AAstrawildEchoCharacter::StaticClass(), FVector(Point.X, Point.Y, GroundZ + 150.0f), FRotator::ZeroRotator, Params))
                {
                    Echo->InitializeFromDefinition(SpeciesDef);
                }
            }
        }
    }

    // 5) Night raid — hostiles converge on the camp (Dawn Fields center).
    if (Definition->RaidHostileCount > 0)
    {
        const UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
        const FAstrawildZoneDescriptor* DawnFields = UAstrawildZoneSubsystem::FindZone(EAstrawildZone::DawnFields);
        if (Registry && DawnFields)
        {
            // Deterministic hostile pick: any hostile species homed near the camp zone.
            TArray<UAstrawildEchoDefinition*> Hostiles;
            for (UAstrawildEchoDefinition* Def : Registry->GetAllEchoDefinitions())
            {
                if (Def && Def->bHostileToPlayers && Def->HomeZone == EAstrawildZone::DawnFields)
                {
                    Hostiles.Add(Def);
                }
            }
            if (!Hostiles.IsEmpty())
            {
                const AAstrawildGameState* GameState = GetGameState();
                const int32 Seed = GameState ? GameState->WorldSeed : 1337;
                FRandomStream Stream(Seed ^ 0x64A1D ^ static_cast<uint32>(GetAbsoluteMinute()));
                const FVector2D Camp = DawnFields->GetCenter();
                for (int32 i = 0; i < Definition->RaidHostileCount; ++i)
                {
                    UAstrawildEchoDefinition* HostileDef = Hostiles[Stream.RandRange(0, Hostiles.Num() - 1)];
                    const float Angle = Stream.FRandRange(0.0f, 2.0f * PI);
                    const float Radius = Stream.FRandRange(2600.0f, 5200.0f);
                    const FVector2D Point(Camp.X + Radius * FMath::Cos(Angle), Camp.Y + Radius * FMath::Sin(Angle));
                    const float GroundZ = AAstrawildTerrainTileActor::EvalWorldHeight(Point, Seed);
                    FActorSpawnParameters Params;
                    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
                    if (AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(
                        AAstrawildEchoCharacter::StaticClass(), FVector(Point.X, Point.Y, GroundZ + 150.0f), FRotator::ZeroRotator, Params))
                    {
                        Echo->InitializeFromDefinition(HostileDef);
                    }
                }
            }
        }
    }

    // 6) Loot drop (supply drop) — granted directly to the first player with a
    // toast telling the story (a physical crate actor lands with the art pass).
    if (!Definition->RewardLootTableId.IsNone())
    {
        const UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
        const UAstrawildLootTableDefinition* Loot = Registry ? Registry->FindLootTable(Definition->RewardLootTableId) : nullptr;
        if (Loot)
        {
            const AAstrawildGameState* GameState = GetGameState();
            const int32 Seed = GameState ? GameState->WorldSeed : 1337;
            FRandomStream Stream(Seed ^ 0x900D ^ static_cast<uint32>(GetAbsoluteMinute()));
            for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
            {
                AAstrawildPlayerCharacter* Player = *It;
                if (Player && Player->InventoryComponent)
                {
                    for (const FAstrawildItemStack& Drop : Loot->GuaranteedDrops)
                    {
                        Player->InventoryComponent->AddItemSilent(Drop.ItemId, Drop.Quantity);
                    }
                    if (Stream.FRand() < Loot->BonusRollChance && !Loot->GuaranteedDrops.IsEmpty())
                    {
                        const FAstrawildItemStack& Bonus = Loot->GuaranteedDrops[Stream.RandRange(0, Loot->GuaranteedDrops.Num() - 1)];
                        Player->InventoryComponent->AddItemSilent(Bonus.ItemId, Bonus.Quantity);
                    }
                }
                break; // First player only (single-player-first design).
            }
        }
    }
}

void UAstrawildWorldEventSubsystem::EndExpiredEvents(const int32 CurrentAbsoluteMinute)
{
    // Iterate backwards — ending removes entries.
    for (int32 i = ActiveEvents.Num() - 1; i >= 0; --i)
    {
        if (ActiveEvents[i].EndAbsoluteMinute > 0 && CurrentAbsoluteMinute >= ActiveEvents[i].EndAbsoluteMinute)
        {
            const FName EventId = ActiveEvents[i].EventId;
            EndEvent(EventId);
        }
    }
}

void UAstrawildWorldEventSubsystem::EndEvent(const FName EventId)
{
    UWorld* World = GetWorld();
    for (int32 i = 0; i < ActiveEvents.Num(); ++i)
    {
        if (ActiveEvents[i].EventId == EventId)
        {
            ActiveEvents.RemoveAt(i);
            break;
        }
    }
    if (World)
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_WorldEventEnded, nullptr, EventId, 0, FVector::ZeroVector);
        }
    }
    OnWorldEventStateChanged.Broadcast(EventId, false);
    BroadcastToast(FText::FromString(FString::Printf(TEXT("World event over: %s"), *EventId.ToString())));
    UE_LOG(LogAstrawild, Log, TEXT("World event ended: %s."), *EventId.ToString());
}

bool UAstrawildWorldEventSubsystem::ForceStartEvent(const FName EventId)
{
    const UWorld* World = GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    UAstrawildWorldEventDefinition* Definition = Registry ? Registry->FindWorldEvent(EventId) : nullptr;
    return Definition ? StartEvent(Definition) : false;
}

void UAstrawildWorldEventSubsystem::ExportForSave(FAstrawildWorldEventScheduleSaveData& OutData) const
{
    OutData.ActiveEvents = ActiveEvents;
    OutData.NextRollAbsoluteMinute = NextRollAbsoluteMinute;

    // Prune expired cooldowns so the save stays small and load resumes cleanly.
    const int32 CurrentMinute = GetAbsoluteMinute();
    OutData.CooldownEndMinutes.Empty();
    for (const TPair<FName, int32>& Pair : CooldownEndMinutes)
    {
        if (CurrentMinute < 0 || Pair.Value > CurrentMinute)
        {
            OutData.CooldownEndMinutes.Add(Pair.Key, Pair.Value);
        }
    }
}

void UAstrawildWorldEventSubsystem::ImportFromSave(const FAstrawildWorldEventScheduleSaveData& InData)
{
    ActiveEvents = InData.ActiveEvents;
    NextRollAbsoluteMinute = InData.NextRollAbsoluteMinute;
    CooldownEndMinutes = InData.CooldownEndMinutes;
    bScheduleInitialized = NextRollAbsoluteMinute > 0;
}

void UAstrawildWorldEventSubsystem::BroadcastToast(const FText& Message) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (AAstrawildPlayerController* AstrawildPC = Cast<AAstrawildPlayerController>(PC))
        {
            AstrawildPC->Notify(Message);
        }
    }
}
