#include "AstrawildSpoilageSubsystem.h"

#include "AstrawildBuildingActor.h"
#include "AstrawildDataAssets.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"

UAstrawildSpoilageSubsystem::UAstrawildSpoilageSubsystem()
{
    // Tick is enabled lazily by IsTickable (game world + at least one player).
}

TStatId UAstrawildSpoilageSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildSpoilageSubsystem, STATGROUP_Tickables);
}

bool UAstrawildSpoilageSubsystem::IsTickable() const
{
    // Only meaningful with a live game world (players may join later, so keep
    // the tick armed — the body exits cheaply when there is nothing to age).
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

void UAstrawildSpoilageSubsystem::Tick(float DeltaTime)
{
    if (!IsTickable())
    {
        return;
    }

    TickAccumulator += DeltaTime;
    if (TickAccumulator < TickCadenceSeconds)
    {
        return;
    }

    // Consume whole cadences only (deterministic steps regardless of frame rate).
    const float Step = TickAccumulator;
    TickAccumulator = 0.0f;

    AdvanceSpoilage(Step);
}

UAstrawildItemRegistrySubsystem* UAstrawildSpoilageSubsystem::GetRegistry() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    return World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
}

bool UAstrawildSpoilageSubsystem::QueryPreservedNearPlayer() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    static constexpr float PreservationRadius = 900.0f;
    static const FName IceBoxId = TEXT("Building_IceBox");

    for (TActorIterator<AAstrawildBuildingActor> It(World); It; ++It)
    {
        const AAstrawildBuildingActor* Building = *It;
        if (!IsValid(Building) || Building->DefinitionId != IceBoxId)
        {
            continue;
        }

        for (TActorIterator<AAstrawildPlayerCharacter> PlayerIt(World); PlayerIt; ++PlayerIt)
        {
            const AAstrawildPlayerCharacter* Player = *PlayerIt;
            if (!IsValid(Player))
            {
                continue;
            }
            if (FVector::DistSquared(Building->GetActorLocation(), Player->GetActorLocation()) <=
                PreservationRadius * PreservationRadius)
            {
                return true;
            }
        }
    }

    return false;
}

bool UAstrawildSpoilageSubsystem::IsPlayerPreserved() const
{
    return bPlayerPreserved;
}

void UAstrawildSpoilageSubsystem::AdvanceSpoilage(float DeltaSeconds)
{
    const UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!World || !Registry)
    {
        return;
    }

    bPlayerPreserved = QueryPreservedNearPlayer();

    for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
    {
        AAstrawildPlayerCharacter* Player = *It;
        if (!IsValid(Player) || !Player->InventoryComponent)
        {
            continue;
        }

        UAstrawildInventoryComponent* Inventory = Player->InventoryComponent;
        const TArray<FAstrawildItemStack> Stacks = Inventory->GetItemStacks();

        for (const FAstrawildItemStack& Stack : Stacks)
        {
            const UAstrawildItemDefinition* Item = Registry->FindItem(Stack.ItemId);
            if (!Item || Item->PerishableSeconds <= 0.0f)
            {
                continue;
            }

            const float Remaining = ComputeSpoilStep(GetFreshness(Stack.ItemId),
                Item->PerishableSeconds, DeltaSeconds, bPlayerPreserved);

            if (Remaining > 0.0f)
            {
                FoodFreshness.Add(Stack.ItemId, Remaining);
                continue;
            }

            // Deadline hit: the whole stack converts to spoiled organics.
            const int32 SpoiledAmount = ComputeSpoiledConversion(Stack.Quantity);
            Inventory->RemoveItem(Stack.ItemId, Stack.Quantity);
            // Silent add: spoilage is not "freshly gathered" progress.
            Inventory->AddItemSilent(TEXT("Item_SpoiledOrganics"), SpoiledAmount);
            FoodFreshness.Remove(Stack.ItemId);

            UE_LOG(LogAstrawild, Log, TEXT("Spoilage: %s x%d spoiled into %d organics (player %s)"),
                *Stack.ItemId.ToString(), Stack.Quantity, SpoiledAmount, *Player->GetName());
        }
    }
}

float UAstrawildSpoilageSubsystem::ComputeSpoilStep(float Freshness, float PerishableSeconds,
    float DeltaSeconds, bool bPreserved)
{
    // Unknown freshness initializes to the full shelf life.
    const float Current = Freshness > 0.0f ? Freshness : PerishableSeconds;

    const float EffectiveDelta = bPreserved ? DeltaSeconds * IceBoxSlowdownFactor : DeltaSeconds;
    const float Remaining = Current - EffectiveDelta;

    // Negative = overdue; callers treat <= 0 as the conversion deadline.
    return FMath::Max(Remaining, -1.0f);
}

int32 UAstrawildSpoilageSubsystem::ComputeSpoiledConversion(int32 StackQuantity)
{
    // Half the stack (floor 1) becomes composting organics — food never
    // silently vanishes, it becomes a farm-loop input.
    return FMath::Max(1, StackQuantity / 2);
}

float UAstrawildSpoilageSubsystem::GetFreshness(FName ItemId) const
{
    const float* Found = FoodFreshness.Find(ItemId);
    return Found ? *Found : 0.0f;
}

TMap<FName, float> UAstrawildSpoilageSubsystem::ExportForSave() const
{
    TMap<FName, float> Out;
    for (const TPair<FName, float>& Pair : FoodFreshness)
    {
        if (Pair.Value > 0.0f)
        {
            Out.Add(Pair.Key, Pair.Value);
        }
    }
    return Out;
}

void UAstrawildSpoilageSubsystem::ImportFromSave(const TMap<FName, float>& InFreshness)
{
    FoodFreshness.Reset();

    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry)
    {
        return;
    }

    for (const TPair<FName, float>& Pair : InFreshness)
    {
        const UAstrawildItemDefinition* Item = Registry->FindItem(Pair.Key);
        if (!Item || Item->PerishableSeconds <= 0.0f)
        {
            // Sanitize: non-perishable ids from edited saves are dropped.
            continue;
        }
        FoodFreshness.Add(Pair.Key, FMath::Clamp(Pair.Value, 0.0f, Item->PerishableSeconds));
    }
}
