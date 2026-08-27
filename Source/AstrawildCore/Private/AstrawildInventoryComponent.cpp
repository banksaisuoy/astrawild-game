#include "AstrawildInventoryComponent.h"

#include "AstrawildCore.h"

UAstrawildInventoryComponent::UAstrawildInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

bool UAstrawildInventoryComponent::IsValidQuantityRequest(const FName ItemId, const int32 Quantity) const
{
    return !ItemId.IsNone() && Quantity > 0;
}

bool UAstrawildInventoryComponent::AddItem(const FName ItemId, const int32 Quantity)
{
    if (!IsValidQuantityRequest(ItemId, Quantity))
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Inventory AddItem rejected: invalid request for %s x%d"), *ItemId.ToString(), Quantity);
        return false;
    }

    int32& CurrentQuantity = Items.FindOrAdd(ItemId);
    CurrentQuantity = FMath::Max(0, CurrentQuantity + Quantity);
    OnInventoryChanged.Broadcast(ItemId, CurrentQuantity);
    return true;
}

bool UAstrawildInventoryComponent::RemoveItem(const FName ItemId, const int32 Quantity)
{
    if (!IsValidQuantityRequest(ItemId, Quantity) || !HasItem(ItemId, Quantity))
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("Inventory RemoveItem rejected: %s x%d is unavailable"), *ItemId.ToString(), Quantity);
        return false;
    }

    int32& CurrentQuantity = Items.FindChecked(ItemId);
    CurrentQuantity -= Quantity;
    const int32 NewQuantity = CurrentQuantity;
    if (CurrentQuantity <= 0)
    {
        Items.Remove(ItemId);
    }

    OnInventoryChanged.Broadcast(ItemId, NewQuantity);
    return true;
}

int32 UAstrawildInventoryComponent::GetQuantity(const FName ItemId) const
{
    if (const int32* Quantity = Items.Find(ItemId))
    {
        return *Quantity;
    }
    return 0;
}

bool UAstrawildInventoryComponent::HasItem(const FName ItemId, const int32 Quantity) const
{
    return !ItemId.IsNone() && Quantity > 0 && GetQuantity(ItemId) >= Quantity;
}

bool UAstrawildInventoryComponent::ConsumeItems(const TArray<FAstrawildItemStack>& RequiredItems)
{
    for (const FAstrawildItemStack& Required : RequiredItems)
    {
        if (!Required.IsValid() || !HasItem(Required.ItemId, Required.Quantity))
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
    TArray<FAstrawildItemStack> Result;
    Result.Reserve(Items.Num());
    for (const TPair<FName, int32>& Pair : Items)
    {
        if (Pair.Key.IsNone() || Pair.Value <= 0)
        {
            continue;
        }

        FAstrawildItemStack Stack;
        Stack.ItemId = Pair.Key;
        Stack.Quantity = Pair.Value;
        Result.Add(Stack);
    }
    return Result;
}

void UAstrawildInventoryComponent::SetItemStacks(const TArray<FAstrawildItemStack>& InStacks)
{
    Items.Reset();
    for (const FAstrawildItemStack& Stack : InStacks)
    {
        if (Stack.IsValid())
        {
            Items.FindOrAdd(Stack.ItemId) += Stack.Quantity;
        }
    }

    for (const TPair<FName, int32>& Pair : Items)
    {
        OnInventoryChanged.Broadcast(Pair.Key, Pair.Value);
    }
}

void UAstrawildInventoryComponent::ClearInventory()
{
    const TArray<FAstrawildItemStack> ExistingStacks = GetItemStacks();
    Items.Reset();
    for (const FAstrawildItemStack& Stack : ExistingStacks)
    {
        OnInventoryChanged.Broadcast(Stack.ItemId, 0);
    }
}
