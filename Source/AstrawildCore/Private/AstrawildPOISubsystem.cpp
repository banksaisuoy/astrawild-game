#include "AstrawildPOISubsystem.h"

#include "AstrawildDataAssets.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPOIMarkerActor.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildResearchSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

void UAstrawildPOISubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    const UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    SweepAccumulator += DeltaTime;
    if (SweepAccumulator < SweepIntervalSeconds)
    {
        return;
    }
    SweepAccumulator = 0.0f;
    RunSweep();
}

UAstrawildItemRegistrySubsystem* UAstrawildPOISubsystem::GetRegistry() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
}

bool UAstrawildPOISubsystem::IsPOIDiscovered(const FName PoiId) const
{
    return DiscoveredPOIIds.Contains(PoiId);
}

TArray<FName> UAstrawildPOISubsystem::GetDiscoveredPOIIds() const
{
    return DiscoveredPOIIds;
}

void UAstrawildPOISubsystem::GetDiscoveryProgress(int32& OutDiscovered, int32& OutTotal) const
{
    const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    OutDiscovered = DiscoveredPOIIds.Num();
    OutTotal = Registry ? Registry->GetAllPOIs().Num() : 0;
}

float UAstrawildPOISubsystem::ComputeDiscoveryRadius(const UAstrawildPOIDefinition* Definition, const bool bHasSignalScanner)
{
    if (!IsValid(Definition))
    {
        return 0.0f;
    }
    // Ancient-signal scanners DOUBLE every discovery radius (they are the
    // exploration tier reward — Master Plan §10).
    const float Base = FMath::Max(100.0f, Definition->DiscoveryRadius);
    return bHasSignalScanner ? Base * 2.0f : Base;
}

void UAstrawildPOISubsystem::RunSweep()
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!World || !Registry)
    {
        return;
    }

    // Refresh the marker cache once (bootstrapper spawns them at world start).
    if (Markers.IsEmpty())
    {
        for (TActorIterator<AAstrawildPOIMarkerActor> It(World); It; ++It)
        {
            AAstrawildPOIMarkerActor* Marker = *It;
            if (Marker && !Marker->PoiId.IsNone())
            {
                Markers.Add(Marker->PoiId, Marker);
            }
        }
        if (Markers.IsEmpty())
        {
            return; // No POI markers placed yet.
        }
    }

    for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
    {
        AAstrawildPlayerCharacter* Player = *It;
        if (!Player || !Player->InventoryComponent)
        {
            continue;
        }

        const bool bHasSignalScanner = Player->InventoryComponent->HasAncientSignalTracking();

        for (const TPair<FName, TWeakObjectPtr<AAstrawildPOIMarkerActor>>& Pair : Markers)
        {
            if (DiscoveredPOIIds.Contains(Pair.Key))
            {
                continue;
            }
            const AAstrawildPOIMarkerActor* Marker = Pair.Value.Get();
            const UAstrawildPOIDefinition* Def = Registry->FindPOI(Pair.Key);
            if (!Marker || !Def)
            {
                continue;
            }

            // Ancient signal sources only reveal themselves to trackers.
            if (Def->bRequiresSignalScanner && !bHasSignalScanner)
            {
                continue;
            }

            const float Radius = ComputeDiscoveryRadius(Def, bHasSignalScanner);
            if (FVector::DistSquared2D(Player->GetActorLocation(), Marker->GetActorLocation()) <= FMath::Square(Radius))
            {
                DiscoverPOI(Pair.Key);
            }
        }
    }
}

bool UAstrawildPOISubsystem::DiscoverPOI(const FName PoiId)
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    UAstrawildPOIDefinition* Definition = Registry ? Registry->FindPOI(PoiId) : nullptr;
    if (!World || !Definition || DiscoveredPOIIds.Contains(PoiId))
    {
        return false;
    }

    DiscoveredPOIIds.Add(PoiId);
    ResolveDiscoveryRewards(Definition);

    if (TWeakObjectPtr<AAstrawildPOIMarkerActor>* MarkerPtr = Markers.Find(PoiId))
    {
        if (AAstrawildPOIMarkerActor* Marker = MarkerPtr->Get())
        {
            Marker->MarkDiscovered();
        }
    }

    if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
    {
        EventBus->PublishEvent(TAG_Astrawild_Event_PoiDiscovered, nullptr, PoiId, 1,
            Markers.Contains(PoiId) && Markers[PoiId].IsValid() ? Markers[PoiId]->GetActorLocation() : FVector::ZeroVector);
    }
    OnPOIDiscovered.Broadcast(PoiId);

    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (AAstrawildPlayerController* AstrawildPC = Cast<AAstrawildPlayerController>(PC))
        {
            AstrawildPC->Notify(FText::FromString(FString::Printf(TEXT("Discovered: %s — %s"),
                *Definition->DisplayName.ToString(), *Definition->LoreLine.ToString())));
        }
    }
    UE_LOG(LogAstrawild, Log, TEXT("POI discovered: %s"), *PoiId.ToString());
    return true;
}

void UAstrawildPOISubsystem::ResolveDiscoveryRewards(const UAstrawildPOIDefinition* Definition)
{
    // Loot table + research — resolved once on discovery (data-driven).
    UWorld* World = GetWorld();
    if (!World || !IsValid(Definition))
    {
        return;
    }

    if (Definition->ResearchReward > 0 && World->GetGameInstance())
    {
        if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->AddResearchPoints(Definition->ResearchReward);
        }
    }

    if (!Definition->RewardLootTableId.IsNone())
    {
        const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
        const UAstrawildLootTableDefinition* Loot = Registry ? Registry->FindLootTable(Definition->RewardLootTableId) : nullptr;
        if (Loot)
        {
            for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
            {
                AAstrawildPlayerCharacter* Player = *It;
                if (Player && Player->InventoryComponent)
                {
                    for (const FAstrawildItemStack& Drop : Loot->GuaranteedDrops)
                    {
                        Player->InventoryComponent->AddItemSilent(Drop.ItemId, Drop.Quantity);
                    }
                }
                break; // Single-player-first: the explorer who walked in.
            }
        }
    }
}

void UAstrawildPOISubsystem::ExportForSave(TArray<FName>& OutDiscoveredIds) const
{
    OutDiscoveredIds = DiscoveredPOIIds;
}

void UAstrawildPOISubsystem::ImportFromSave(const TArray<FName>& InDiscoveredIds)
{
    DiscoveredPOIIds = InDiscoveredIds;
    // Markers re-dim on load (markers themselves respawn from definitions).
}
