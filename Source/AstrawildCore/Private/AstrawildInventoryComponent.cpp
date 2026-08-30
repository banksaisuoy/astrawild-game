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
    DOREPLIFETIME(UAstrawildInventoryComponent, EquippedShieldItemId);
    DOREPLIFETIME(UAstrawildInventoryComponent, EquippedArmorItemId);
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

bool UAstrawildInventoryComponent::AddItemSilent(const FName ItemId, const int32 Quantity)
{
    if (!IsValidQuantityRequest(ItemId, Quantity))
    {
        return false;
    }

    if (GetOwnerRole() == ROLE_Authority && !CanAddItem(ItemId, Quantity))
    {
        UE_LOG(LogAstrawildEconomy, Verbose, TEXT("AddItemSilent rejected (over weight): %s x%d"), *ItemId.ToString(), Quantity);
        return false;
    }

    int32& Count = Items.FindOrAdd(ItemId);
    Count += Quantity;
    OnInventoryChanged.Broadcast(ItemId, Count);
    BroadcastWeight();
    // Intentionally NOT publishing TAG_Astrawild_Event_ItemCollected — refunds
    // should not advance CollectItem quest objectives (Batch 2 — Item B).
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
            // Wave 3 routing: attack items are weapons, mitigation items are shields.
            if (ItemDef->AttackPower > 0.0f)
            {
                EquippedItemId = ItemId;
            }
            else if (ItemDef->BlockMitigation > 0.0f)
            {
                EquippedShieldItemId = ItemId;
            }
            // Batch 3 — Item C: torso armor routes to its own slot (must come before
            // the statless legacy fallback below).
            else if (ItemDef->ArmorRating > 0.0f)
            {
                const FName Previous = EquippedArmorItemId;
                EquippedArmorItemId = ItemId;
                if (Previous != ItemId)
                {
                    OnArmorChanged.Broadcast(EquippedArmorItemId);
                }
            }
            else
            {
                // Statless equipment keeps the legacy weapon slot behaviour.
                EquippedItemId = ItemId;
            }
            OnEquipmentChanged.Broadcast(EquippedItemId, EquippedShieldItemId);
            return true;
        }
    }
    return false;
}

void UAstrawildInventoryComponent::Unequip()
{
    EquippedItemId = NAME_None;
    EquippedShieldItemId = NAME_None;
    // Batch 3 — Item C: clear the armor slot too.
    if (!EquippedArmorItemId.IsNone())
    {
        EquippedArmorItemId = NAME_None;
        OnArmorChanged.Broadcast(NAME_None);
    }
    OnEquipmentChanged.Broadcast(EquippedItemId, EquippedShieldItemId);
}

float UAstrawildInventoryComponent::GetEquippedWeaponAttackPower() const
{
    if (EquippedItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedItemId))
        {
            return ItemDef->AttackPower;
        }
    }
    return 0.0f;
}

float UAstrawildInventoryComponent::GetEquippedShieldMitigation() const
{
    if (EquippedShieldItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedShieldItemId))
        {
            return ItemDef->BlockMitigation;
        }
    }
    return 0.0f;
}

float UAstrawildInventoryComponent::GetEquippedArmorRating() const
{
    // Batch 3 — Item C: armor rating for the combat component's damage-reduction formula.
    if (EquippedArmorItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedArmorItemId))
        {
            return ItemDef->ArmorRating;
        }
    }
    return 0.0f;
}

EAstrawildElementType UAstrawildInventoryComponent::GetEquippedWeaponElement() const
{
    // Batch 3 — Item A: the equipped weapon's element overrides the combat tunable.
    if (EquippedItemId.IsNone())
    {
        return EAstrawildElementType::None;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedItemId))
        {
            return ItemDef->Element;
        }
    }
    return EAstrawildElementType::None;
}

void UAstrawildInventoryComponent::BroadcastWeight()
{
    OnWeightChanged.Broadcast(GetCurrentWeight());
}
