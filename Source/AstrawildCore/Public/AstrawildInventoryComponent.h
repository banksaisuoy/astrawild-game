#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildInventoryChanged, FName, ItemId, int32, NewQuantity);

UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildInventoryComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Inventory")
    FAstrawildInventoryChanged OnInventoryChanged;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory")
    bool AddItem(FName ItemId, int32 Quantity);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory")
    bool RemoveItem(FName ItemId, int32 Quantity);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory")
    int32 GetQuantity(FName ItemId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory")
    bool HasItem(FName ItemId, int32 Quantity = 1) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory")
    bool ConsumeItems(const TArray<FAstrawildItemStack>& RequiredItems);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory")
    TArray<FAstrawildItemStack> GetItemStacks() const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory")
    void SetItemStacks(const TArray<FAstrawildItemStack>& InStacks);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory")
    void ClearInventory();

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Inventory")
    TMap<FName, int32> Items;

private:
    bool IsValidQuantityRequest(FName ItemId, int32 Quantity) const;
};
