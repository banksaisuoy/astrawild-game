// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdatedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAddedSignature, const FGameplayTag&, ItemTag, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemRemovedSignature, const FGameplayTag&, ItemTag, int32, Quantity);

UCLASS(ClassGroup = (Astrawild), meta = (BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAstrawildInventoryComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory", meta = (ClampMin = "1", ClampMax = "120"))
	int32 MaxSlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	float MaxWeightCapacity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
	TArray<FAstrawildItemSlot> Slots;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryUpdatedSignature OnInventoryUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemAddedSignature OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemRemovedSignature OnItemRemoved;

public:
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(const FGameplayTag& ItemTag, int32 Quantity, float Durability = 100.0f);

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(const FGameplayTag& ItemTag, int32 Quantity);

	UFUNCTION(BlueprintPure, Category = "Inventory")
	bool HasItem(const FGameplayTag& ItemTag, int32 RequiredQuantity = 1) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(const FGameplayTag& ItemTag) const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetEmptySlotCount() const;

	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FAstrawildItemSlot>& GetSlots() const { return Slots; }

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ClearInventory();

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void LoadInventorySlots(const TArray<FAstrawildItemSlot>& InSlots);
};