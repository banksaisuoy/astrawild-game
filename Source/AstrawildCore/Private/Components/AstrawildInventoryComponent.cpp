// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AstrawildInventoryComponent.h"
#include "AstrawildLogChannels.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UAstrawildInventoryComponent::UAstrawildInventoryComponent()
	: MaxSlots(30)
	, MaxWeightCapacity(250.0f)
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UAstrawildInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UAstrawildInventoryComponent, Slots);
}

void UAstrawildInventoryComponent::ServerAddItem_Implementation(const FGameplayTag ItemTag, const int32 Quantity, const float Durability)
{
	AddItem(ItemTag, Quantity, Durability);
}

void UAstrawildInventoryComponent::ServerRemoveItem_Implementation(const FGameplayTag ItemTag, const int32 Quantity)
{
	RemoveItem(ItemTag, Quantity);
}

void UAstrawildInventoryComponent::ServerMoveOrSwapSlot_Implementation(const int32 FromIndex, const int32 ToIndex)
{
	MoveOrSwapSlot(FromIndex, ToIndex);
}

void UAstrawildInventoryComponent::ServerSplitSlot_Implementation(const int32 FromIndex, const int32 ToIndex, const int32 Amount)
{
	SplitSlot(FromIndex, ToIndex, Amount);
}

void UAstrawildInventoryComponent::ServerClearInventory_Implementation()
{
	ClearInventory();
}

void UAstrawildInventoryComponent::OnRepSlots()
{
	OnInventoryUpdated.Broadcast();
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
	if (const AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		if (OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
		{
			ServerAddItem(ItemTag, Quantity, Durability);
		}
		return false;
	}
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

bool UAstrawildInventoryComponent::CanAddItem(const FGameplayTag& ItemTag, const int32 Quantity) const
{
	if (!ItemTag.IsValid() || Quantity <= 0)
	{
		return false;
	}

	int32 RemainingToAdd = Quantity;
	constexpr int32 MaxStack = 99;
	for (const FAstrawildItemSlot& Slot : Slots)
	{
		if (Slot.ItemTag == ItemTag && Slot.Quantity < MaxStack)
		{
			RemainingToAdd -= FMath::Min(RemainingToAdd, MaxStack - Slot.Quantity);
			if (RemainingToAdd <= 0)
			{
				return true;
			}
		}
	}

	for (const FAstrawildItemSlot& Slot : Slots)
	{
		if (!Slot.IsValid())
		{
			RemainingToAdd -= FMath::Min(RemainingToAdd, MaxStack);
			if (RemainingToAdd <= 0)
			{
				return true;
			}
		}
	}
	return false;
}

bool UAstrawildInventoryComponent::RemoveItem(const FGameplayTag& ItemTag, int32 Quantity)
{
	if (const AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		if (OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
		{
			ServerRemoveItem(ItemTag, Quantity);
		}
		return false;
	}
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

bool UAstrawildInventoryComponent::MoveOrSwapSlot(int32 FromIndex, int32 ToIndex)
{
	if (const AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		if (OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
		{
			ServerMoveOrSwapSlot(FromIndex, ToIndex);
		}
		return false;
	}
	if (!Slots.IsValidIndex(FromIndex) || !Slots.IsValidIndex(ToIndex) || FromIndex == ToIndex)
	{
		return false;
	}

	// If both slots contain the same item, try to merge stacks
	if (Slots[FromIndex].IsValid() && Slots[ToIndex].IsValid() && Slots[FromIndex].ItemTag == Slots[ToIndex].ItemTag)
	{
		const int32 MaxStack = 99;
		const int32 AvailableSpace = MaxStack - Slots[ToIndex].Quantity;
		if (AvailableSpace > 0)
		{
			const int32 TransferAmount = FMath::Min(Slots[FromIndex].Quantity, AvailableSpace);
			Slots[ToIndex].Quantity += TransferAmount;
			Slots[FromIndex].Quantity -= TransferAmount;

			if (Slots[FromIndex].Quantity <= 0)
			{
				Slots[FromIndex].Clear();
			}

			OnInventoryUpdated.Broadcast();
			return true;
		}
	}

	// Otherwise swap slots directly
	Slots.Swap(FromIndex, ToIndex);
	OnInventoryUpdated.Broadcast();
	return true;
}

bool UAstrawildInventoryComponent::SplitSlot(int32 FromIndex, int32 ToIndex, int32 Amount)
{
	if (const AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		if (OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
		{
			ServerSplitSlot(FromIndex, ToIndex, Amount);
		}
		return false;
	}
	if (!Slots.IsValidIndex(FromIndex) || !Slots.IsValidIndex(ToIndex) || FromIndex == ToIndex || Amount <= 0)
	{
		return false;
	}

	if (!Slots[FromIndex].IsValid() || Slots[FromIndex].Quantity <= Amount)
	{
		return false;
	}

	if (Slots[ToIndex].IsValid())
	{
		return false; // Target slot must be empty to split into
	}

	Slots[ToIndex] = FAstrawildItemSlot(Slots[FromIndex].ItemTag, Amount, Slots[FromIndex].Durability);
	Slots[FromIndex].Quantity -= Amount;

	OnInventoryUpdated.Broadcast();
	return true;
}

void UAstrawildInventoryComponent::ClearInventory()
{
	if (const AActor* OwnerActor = GetOwner(); OwnerActor && !OwnerActor->HasAuthority())
	{
		if (OwnerActor->GetLocalRole() == ROLE_AutonomousProxy)
		{
			ServerClearInventory();
		}
		return;
	}
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