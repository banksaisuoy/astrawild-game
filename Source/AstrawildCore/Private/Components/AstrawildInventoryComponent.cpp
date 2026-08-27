// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AstrawildInventoryComponent.h"
#include "AstrawildLogChannels.h"

UAstrawildInventoryComponent::UAstrawildInventoryComponent()
	: MaxSlots(30)
	, MaxWeightCapacity(250.0f)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAstrawildInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	if (Slots.Num() < MaxSlots)
	{
		Slots.SetNum(MaxSlots);
	}
}

bool UAstrawildInventoryComponent::AddItem(const FGameplayTag& ItemTag, int32 Quantity, float Durability)
{
	if (!ItemTag.IsValid() || Quantity <= 0)
	{
		return false;
	}

	int32 RemainingToAdd = Quantity;
	const int32 MaxStack = 99; // Default max stack

	// 1. Try to stack into existing non-full slots
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].ItemTag == ItemTag && Slots[i].Quantity < MaxStack)
		{
			const int32 AvailableSpace = MaxStack - Slots[i].Quantity;
			const int32 AddAmount = FMath::Min(RemainingToAdd, AvailableSpace);

			Slots[i].Quantity += AddAmount;
			RemainingToAdd -= AddAmount;

			if (RemainingToAdd <= 0)
			{
				break;
			}
		}
	}

	// 2. Put remaining into first available empty slot
	if (RemainingToAdd > 0)
	{
		for (int32 i = 0; i < Slots.Num(); ++i)
		{
			if (!Slots[i].IsValid())
			{
				const int32 AddAmount = FMath::Min(RemainingToAdd, MaxStack);
				Slots[i] = FAstrawildItemSlot(ItemTag, AddAmount, Durability);
				RemainingToAdd -= AddAmount;

				if (RemainingToAdd <= 0)
				{
					break;
				}
			}
		}
	}

	const int32 SuccessfullyAdded = Quantity - RemainingToAdd;
	if (SuccessfullyAdded > 0)
	{
		UE_LOG(LogAstrawildInventory, Log, TEXT("Added %d of [%s] to Inventory."), SuccessfullyAdded, *ItemTag.ToString());
		OnItemAdded.Broadcast(ItemTag, SuccessfullyAdded);
		OnInventoryUpdated.Broadcast();
	}

	return RemainingToAdd == 0;
}

bool UAstrawildInventoryComponent::RemoveItem(const FGameplayTag& ItemTag, int32 Quantity)
{
	if (!ItemTag.IsValid() || Quantity <= 0 || !HasItem(ItemTag, Quantity))
	{
		return false;
	}

	int32 RemainingToRemove = Quantity;

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].ItemTag == ItemTag)
		{
			if (Slots[i].Quantity <= RemainingToRemove)
			{
				RemainingToRemove -= Slots[i].Quantity;
				Slots[i].Clear();
			}
			else
			{
				Slots[i].Quantity -= RemainingToRemove;
				RemainingToRemove = 0;
			}

			if (RemainingToRemove <= 0)
			{
				break;
			}
		}
	}

	UE_LOG(LogAstrawildInventory, Log, TEXT("Removed %d of [%s] from Inventory."), Quantity, *ItemTag.ToString());
	OnItemRemoved.Broadcast(ItemTag, Quantity);
	OnInventoryUpdated.Broadcast();
	return true;
}

bool UAstrawildInventoryComponent::HasItem(const FGameplayTag& ItemTag, int32 RequiredQuantity) const
{
	return GetItemCount(ItemTag) >= RequiredQuantity;
}

int32 UAstrawildInventoryComponent::GetItemCount(const FGameplayTag& ItemTag) const
{
	if (!ItemTag.IsValid())
	{
		return 0;
	}

	int32 Total = 0;
	for (const FAstrawildItemSlot& Slot : Slots)
	{
		if (Slot.ItemTag == ItemTag)
		{
			Total += Slot.Quantity;
		}
	}
	return Total;
}

int32 UAstrawildInventoryComponent::GetEmptySlotCount() const
{
	int32 Count = 0;
	for (const FAstrawildItemSlot& Slot : Slots)
	{
		if (!Slot.IsValid())
		{
			Count++;
		}
	}
	return Count;
}

void UAstrawildInventoryComponent::ClearInventory()
{
	for (FAstrawildItemSlot& Slot : Slots)
	{
		Slot.Clear();
	}
	OnInventoryUpdated.Broadcast();
}

void UAstrawildInventoryComponent::LoadInventorySlots(const TArray<FAstrawildItemSlot>& InSlots)
{
	Slots = InSlots;
	if (Slots.Num() < MaxSlots)
	{
		Slots.SetNum(MaxSlots);
	}
	OnInventoryUpdated.Broadcast();
}