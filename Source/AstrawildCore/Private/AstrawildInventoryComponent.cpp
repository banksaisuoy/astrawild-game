#include "AstrawildInventoryComponent.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UAstrawildInventoryComponent::UAstrawildInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UAstrawildInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAstrawildInventoryComponent, Items);
    DOREPLIFETIME(UAstrawildInventoryComponent, EquippedItemId);
}

void UAstrawildInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
}

UAstrawildItemRegistrySubsystem* UAstrawildInventoryComponent::GetRegistry() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
}

bool UAstrawildInventoryComponent::IsValidQuantityRequest(const FName ItemId, const int32 Quantity) const
{
    return !ItemId.IsNone() && Quantity > 0;
}

float UAstrawildInventoryComponent::GetCurrentWeight() const
{
    float Weight = 0.0f;
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        for (const TPair<FName, int32>& Pair : Items)
        {
            if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(Pair.Key))
            {
                Weight += ItemDef->Weight * Pair.Value;
            }
        }
    }
    return Weight;
}

float UAstrawildInventoryComponent::GetWeightFraction() const
{
    if (MaxWeight <= 0.0f)
    {
        return 0.0f;
    }
    return FMath::Clamp(GetCurrentWeight() / MaxWeight, 0.0f, 1.0f);
}

bool UAstrawildInventoryComponent::CanAddItem(const FName ItemId, const int32 Quantity) const
{
    if (!IsValidQuantityRequest(ItemId, Quantity))
    {
        return false;
    }

    if (MaxWeight <= 0.0f)
    {
        return true;
    }

    float AddedWeight = static_cast<float>(Quantity);
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(ItemId))
        {
            AddedWeight = ItemDef->Weight * Quantity;
        }
    }
    return GetCurrentWeight() + AddedWeight <= MaxWeight + KINDA_SMALL_NUMBER;
}

bool UAstrawildInventoryComponent::AddItem(const FName ItemId, const int32 Quantity)
{
    if (!IsValidQuantityRequest(ItemId, Quantity))
    {
        return false;
    }

    // Weight gate (server authoritative; clients keep a loose copy for UI).
    if (GetOwnerRole() == ROLE_Authority && !CanAddItem(ItemId, Quantity))
    {
        UE_LOG(LogAstrawildEconomy, Verbose, TEXT("AddItem rejected (over weight): %s x%d"), *ItemId.ToString(), Quantity);
        return false;
    }

    int32& Count = Items.FindOrAdd(ItemId);
    Count += Quantity;
    OnInventoryChanged.Broadcast(ItemId, Count);
    BroadcastWeight();

    // Publish collection event for quests/journal (server only, directive §25).
    if (GetOwnerRole() == ROLE_Authority)
    {
        if (UWorld* World = GetWorld())
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_ItemCollected, GetOwner(), ItemId, Quantity, GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
            }
        }
    }
    return true;
}

bool UAstrawildInventoryComponent::RemoveItem(const FName ItemId, const int32 Quantity)
{
    if (!HasItem(ItemId, Quantity))
    {
        return false;
    }

    int32& Count = Items.FindChecked(ItemId);
    Count -= Quantity;
    if (Count <= 0)
    {
        Items.Remove(ItemId);
    }
    OnInventoryChanged.Broadcast(ItemId, Count);
    BroadcastWeight();
    return true;
}

int32 UAstrawildInventoryComponent::GetQuantity(const FName ItemId) const
{
    const int32* Count = Items.Find(ItemId);
    return Count ? *Count : 0;
}

bool UAstrawildInventoryComponent::HasItem(const FName ItemId, const int32 Quantity) const
{
    return GetQuantity(ItemId) >= Quantity;
}

bool UAstrawildInventoryComponent::ConsumeItems(const TArray<FAstrawildItemStack>& RequiredItems)
{
    for (const FAstrawildItemStack& Required : RequiredItems)
    {
        if (!HasItem(Required.ItemId, Required.Quantity))
        {
            return false;
        }
    }

    for (const FAstrawildItemStack& Required : RequiredItems)
    {
        RemoveItem(Required.ItemId, Required.Quantity);
    }
    return true;
}

TArray<FAstrawildItemStack> UAstrawildInventoryComponent::GetItemStacks() const
{
    TArray<FAstrawildItemStack> Stacks;
    Stacks.Reserve(Items.Num());
    for (const TPair<FName, int32>& Pair : Items)
    {
        if (Pair.Value > 0)
        {
            FAstrawildItemStack Stack;
            Stack.ItemId = Pair.Key;
            Stack.Quantity = Pair.Value;
            Stacks.Add(Stack);
        }
    }
    return Stacks;
}

void UAstrawildInventoryComponent::SetItemStacks(const TArray<FAstrawildItemStack>& InStacks)
{
    Items.Reset();
    for (const FAstrawildItemStack& Stack : InStacks)
    {
        if (Stack.IsValid())
        {
            Items.Add(Stack.ItemId, Stack.Quantity);
        }
    }
    OnInventoryChanged.Broadcast(NAME_None, 0);
    BroadcastWeight();
}

void UAstrawildInventoryComponent::ClearInventory()
{
    Items.Reset();
    OnInventoryChanged.Broadcast(NAME_None, 0);
    BroadcastWeight();
}

bool UAstrawildInventoryComponent::EquipItem(const FName ItemId)
{
    if (!HasItem(ItemId, 1))
    {
        return false;
    }

    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        const UAstrawildItemDefinition* ItemDef = Registry->FindItem(ItemId);
        if (ItemDef && ItemDef->Category == EAstrawildItemCategory::Equipment)
        {
            EquippedItemId = ItemId;
            return true;
        }
    }
    return false;
}

void UAstrawildInventoryComponent::Unequip()
{
    EquippedItemId = NAME_None;
}

void UAstrawildInventoryComponent::BroadcastWeight()
{
    OnWeightChanged.Broadcast(GetCurrentWeight());
}
