#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildInventoryChanged, FName, ItemId, int32, NewQuantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildWeightChanged, float, CurrentWeight);

/**
 * Data-driven inventory (directive §14): stacks + weight + capacity + one equipment slot.
 * Server-authoritative; UI is a presentation layer only.
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildInventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildInventoryComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Inventory")
    FAstrawildInventoryChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Inventory")
    FAstrawildWeightChanged OnWeightChanged;

    /** Maximum carry weight (kg). 0 = unlimited (debug). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Inventory", meta=(ClampMin="0.0"))
    float MaxWeight = 120.0f;

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

    /** Current total weight (resolves through the item registry). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory")
    float GetCurrentWeight() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory")
    float GetWeightFraction() const;

    /** Would adding this quantity exceed weight limits? */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory")
    bool CanAddItem(FName ItemId, int32 Quantity) const;

    // --- Equipment (single weapon slot v1) ---
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory|Equipment")
    bool EquipItem(FName ItemId);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory|Equipment")
    void Unequip();

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Inventory|Equipment")
    FName EquippedItemId = NAME_None;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Inventory", Replicated)
    TMap<FName, int32> Items;

    virtual void BeginPlay() override;

private:
    bool IsValidQuantityRequest(FName ItemId, int32 Quantity) const;
    void BroadcastWeight();
    class UAstrawildItemRegistrySubsystem* GetRegistry() const;
};
