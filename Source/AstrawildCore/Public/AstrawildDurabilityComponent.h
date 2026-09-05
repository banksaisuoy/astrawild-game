#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildDurabilityComponent.generated.h"

class UAstrawildItemRegistrySubsystem;

/**
 * SCP Phase 12.1 — tool/weapon/armor wear + repair (directive [3] Phase 12).
 *
 * Tracks durability per item id across the player's equipment:
 *  - weapons lose 1 point per landed hit (melee or ranged),
 *  - tools lose 1 point per harvest action,
 *  - armor loses 1 point per damage taken.
 *
 * Broken items (durability 0) keep functioning at reduced effectiveness
 * (weapon damage x0.4, no harvest bonus) — never destroyed, per the repair-
 * bench loop. Repairs restore full durability and cost either a Field Repair
 * Kit item (anywhere) or 40% of the item's craft inputs (bench only).
 *
 * Save additive: TMap<FName, float> EquipmentDurability on the save game
 * (absent in legacy saves = pristine equipment, identical to fresh states).
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildDurabilityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildDurabilityComponent();

    /** Weapon damage penalty while broken. */
    static constexpr float BrokenWeaponDamageMultiplier = 0.4f;

    /** Fraction of an item's craft inputs a bench repair consumes. */
    static constexpr float BenchRepairCostFraction = 0.4f;

    // --- Wear entry points (server or local — cheap TMap writes) ---

    /** Landed a melee/ranged hit with the equipped weapon: -1 durability. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Durability")
    void ApplyWeaponWear();

    /** Completed a harvest swing with the equipped tool: -1 durability. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Durability")
    void ApplyToolWear();

    /** Took a hit with armor equipped: -1 durability on every armor piece. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Durability")
    void ApplyArmorWear();

    // --- Queries ---

    /** True when the item tracks durability AND its pool hit zero. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Durability")
    bool IsItemBroken(FName ItemId) const;

    /** Current durability (0 when untracked — untracked items never break). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Durability")
    float GetDurability(FName ItemId) const;

    /** Fraction 0..1 (1 when the item ignores durability). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Durability")
    float GetDurabilityFraction(FName ItemId) const;

    /**
     * Weapon damage multiplier for the equipped weapon: 1.0 healthy,
     * BrokenWeaponDamageMultiplier when broken, 1.0 when untracked.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Durability")
    float GetEquippedWeaponDamageMultiplier() const;

    /**
     * Harvest yield multiplier for the resource the player is harvesting:
     * tool HarvestMultiplier when the tool's category matches the resource's
     * HarvestCategory (and the tool is not broken), otherwise 1.0.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Durability")
    float GetHarvestYieldMultiplier(FName ResourceItemId) const;

    // --- Repair ---

    /**
     * Repair one item to full.
     *  - bAtRepairBench = true: consumes 40% (ceil) of every craft input of the
     *    item's recipe when the ingredients are known; otherwise falls back to
     *    the repair kit.
     *  - Always able to consume one Item_FieldRepairKit as the field path.
     * Returns the path taken (None when nothing was repaired).
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Durability")
    FName RepairItem(FName ItemId, bool bAtRepairBench);

    // --- Save integration (additive v5, no schema bump) ---

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Durability")
    TMap<FName, float> ExportForSave() const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Durability")
    void ImportFromSave(const TMap<FName, float>& InDurability);

    /** Broke-something notification hook (HUD warning). */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildItemBroke, FName, ItemId, float, MaxDurability);

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Durability")
    FAstrawildItemBroke OnItemBroke;

private:
    /** Durability pools keyed by item id (server-authoritative). */
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Durability")
    TMap<FName, float> DurabilityPools;

    /** Equipped weapon id cache (refreshed on wear/repair queries). */
    FName CachedEquippedWeaponId = NAME_None;

    class UAstrawildInventoryComponent* GetOwnerInventory() const;
    UAstrawildItemRegistrySubsystem* GetRegistry() const;

    /** Applies wear to one item id; returns the pool after the write. */
    float WearItem(FName ItemId, float Amount);

    /** Collects the durability-tracked equipment ids the owner wears (repair-bench loop). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Durability")
    TArray<FName> GetTrackedEquippedItemIds() const;

    /** Resolve the recipe whose output includes the item (bench repair costs). */
    const class UAstrawildRecipeDefinition* FindRecipeForOutput(FName ItemId) const;
};
