#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildInventoryChanged, FName, ItemId, int32, NewQuantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildWeightChanged, float, CurrentWeight);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildEquipmentChanged, FName, WeaponItemId, FName, ShieldItemId);
// Batch 3 — Item C: armor slot has its own additive delegate so the existing
// two-param OnEquipmentChanged signature stays stable for Blueprint consumers.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildArmorChanged, FName, ArmorItemId);
// Final production run (PHASE 12): generic slot-change event covering the three
// new advanced slots (helmet/exosuit/scanner) without touching legacy signatures.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildSlotChanged, EAstrawildEquipmentSlot, Slot, FName, ItemId);

/**
 * Data-driven inventory (directive §14): stacks + weight + capacity + equipment
 * slots (wave 3: weapon + shield, auto-routed by item stat).
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

    /**
     * Batch 2 — Item B: add items WITHOUT publishing TAG_Astrawild_Event_ItemCollected.
     * Used for refund flows (dismantle buildings, failed-craft returns) where the items
     * are not "freshly gathered" and should not advance CollectItem quest objectives.
     * Same weight gate + delegate broadcast as AddItem — only the event-bus publish is suppressed.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory")
    bool AddItemSilent(FName ItemId, int32 Quantity);

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

    // --- Equipment (wave 3: weapon + shield slots, auto-routed by item stat) ---
    /**
     * Equips an Equipment-category item. Routing (wave 3): items with
     * AttackPower > 0 occupy the weapon slot, items with BlockMitigation > 0
     * occupy the shield slot. Returns false when the item is not equipment.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory|Equipment")
    bool EquipItem(FName ItemId);

    /** Unequips every slot (weapon + shield). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Inventory|Equipment")
    void Unequip();

    // Audit C-3 (final run): both slots are DOREPLIFETIME-registered — the missing
    // Replicated specifier rejected the replication layout (equipment would never
    // reach clients).
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Inventory|Equipment", Replicated)
    FName EquippedItemId = NAME_None;

    /** Wave 3 shield slot — feeding block mitigation on the combat component. */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Inventory|Equipment", Replicated)
    FName EquippedShieldItemId = NAME_None;

    /**
     * Batch 3 — Item C: torso armor slot. Feeds the diminishing-returns damage
     * reduction on the combat component (armor rating, not a block multiplier).
     */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Inventory|Equipment", Replicated)
    FName EquippedArmorItemId = NAME_None;

    // --- Final production run (PHASE 12): advanced equipment slots ---

    /** Helmet: extra armor rating + thermal insulation. */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Inventory|Equipment", Replicated)
    FName EquippedHelmetItemId = NAME_None;

    /** Exosuit: insulation + stamina regen + carry weight + move speed bonuses. */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Inventory|Equipment", Replicated)
    FName EquippedExosuitItemId = NAME_None;

    /** Scanner: enables the active hold-to-scan input (journal acceleration). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Inventory|Equipment", Replicated)
    FName EquippedScannerItemId = NAME_None;

    /** Fired whenever one of the advanced slots changes (helmet/exosuit/scanner). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Inventory|Equipment")
    FAstrawildSlotChanged OnSlotChanged;

    /** Fired whenever the armor slot changes (Batch 3 — additive delegate). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Inventory|Equipment")
    FAstrawildArmorChanged OnArmorChanged;

    /** Fired whenever either legacy equipment slot changes (UI hook). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Inventory|Equipment")
    FAstrawildEquipmentChanged OnEquipmentChanged;

    /** Attack power of the equipped weapon (0 when unarmed) — resolves via registry. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedWeaponAttackPower() const;

    /** Block mitigation of the equipped shield (0 when none) — resolves via registry. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedShieldMitigation() const;

    /** Armor rating of the equipped torso armor (0 when none) — resolves via registry (Batch 3 — Item C). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedArmorRating() const;

    /** Element carried by the equipped weapon (None when unarmed/no element) — Batch 3 — Item A. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    EAstrawildElementType GetEquippedWeaponElement() const;

    // --- Final production run (PHASE 12): advanced-equipment queries ---

    /** Combined armor rating of torso + helmet (feeds the combat mitigation formula). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetTotalArmorRating() const;

    /** Armor rating of the equipped helmet alone (0 when none). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedHelmetArmorRating() const;

    /** Insulation degrees from helmet + exosuit (widens the comfortable temperature band). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedInsulationRating() const;

    /** Extra stamina regen per second from the exosuit (0 when none). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedStaminaRegenBonus() const;

    /** Extra carry weight (kg) from the exosuit (0 when none). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedCarryWeightBonus() const;

    /** Fractional move-speed bonus from the exosuit (0 when none). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedMoveSpeedBonus() const;

    /** True when the equipped weapon fires projectiles (laser/energy path). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    bool IsRangedWeaponEquipped() const;

    /** Ammo item id required per shot by the equipped ranged weapon (NAME_None = free). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    FName GetEquippedAmmoItemId() const;

    // --- Production V2 (additive): weapon profiles, split insulation, scanner tiers ---

    /** Behaviour profile of the equipped weapon (null when the item has no WeaponDefinitionId). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    class UAstrawildWeaponDefinition* GetEquippedWeaponDefinition() const;

    /** Cold-side insulation from helmet + exosuit + torso (legacy InsulationRating counts on both sides). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedColdInsulationRating() const;

    /** Heat-side insulation from helmet + exosuit + torso (legacy InsulationRating counts on both sides). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedHeatInsulationRating() const;

    /** Observation range multiplier of the equipped scanner (1 = no scanner/stock). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEquippedScannerRangeMultiplier() const;

    /** True when the equipped scanner reveals hidden resource nodes. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    bool HasHiddenResourceDetection() const;

    /** True when the equipped scanner tracks ancient signals (POI + event hooks). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    bool HasAncientSignalTracking() const;

    /** Rarity of the equipped weapon (display hook). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    EAstrawildRarity GetEquippedWeaponRarity() const;

    /** Effective carry limit including the exosuit bonus (kg). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Inventory|Equipment")
    float GetEffectiveMaxWeight() const;

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
