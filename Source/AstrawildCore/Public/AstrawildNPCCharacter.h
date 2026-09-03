#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildInteractable.h"
#include "AstrawildTypes.h"
#include "AstrawildNPCCharacter.generated.h"

class UAstrawildNPCDefinition;
class UAstrawildNPCScheduleComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class AAstrawildVillageActor;
class UNavigationInvokerComponent;

/**
 * NPC base (directive §26): interaction offers quests from its NPC definition.
 * Schedule/dialogue/faction architecture-ready; conversation data assets arrive
 * with the content pass (see Docs/ASTRAWILD_QUEST_SYSTEM.md).
 *
 * Batch 4 — M-11: NPCs with a ShopLootTableId + CurrencyItemId on their definition
 * are vendors. Interacting with one lists its wares + prices in a HUD toast; the
 * transaction itself runs through TryPurchase/TrySell (server-authoritative).
 *
 * Batch 8 — living villages: NPCs walk their village waypoint circuit, guards
 * fight hostile Echoes, everyone gathers at the campfire at night
 * (AAstrawildNPCAIController + AAstrawildVillageActor — see
 * Docs/ASTRAWILD_VILLAGES_SKIFF.md). Role drives a procedural look: body tint,
 * head + role hat and a small role-colored lantern light.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildNPCCharacter : public ACharacter, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildNPCCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    /** Batch 8 — head + role hat (procedural villager silhouette). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC|Appearance")
    TObjectPtr<UStaticMeshComponent> HeadMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC|Appearance")
    TObjectPtr<UStaticMeshComponent> HatMesh;

    /** Batch 8 — role-colored lantern so roles read at a glance (zero-asset look). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC|Appearance")
    TObjectPtr<UPointLightComponent> RoleLight;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    TObjectPtr<UAstrawildNPCDefinition> NpcDefinition;

    /** SCP Phase 7: daily schedule (work/home/shelter/sleep anchors + service gating). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC|Schedule")
    TObjectPtr<UAstrawildNPCScheduleComponent> ScheduleComponent;

    /** Home village — waypoint provider for the patrol AI (set by the bootstrapper). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    TObjectPtr<AAstrawildVillageActor> HomeVillage;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|NPC")
    void SetHomeVillage(AAstrawildVillageActor* Village) { HomeVillage = Village; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC")
    AAstrawildVillageActor* GetHomeVillage() const { return HomeVillage; }

    /** True when the NPC definition marks this NPC a guard (village defender). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC")
    bool IsGuard() const;

    /** Patrol waypoint cursor (wraps via the village waypoint count). */
    int32 AdvancePatrolIndex();

    /** Last interaction bookkeeping (conversation pause + facing). */
    double GetLastInteractedTime() const { return LastInteractedTime; }
    AActor* GetLastInteractedActor() const { return LastInteractedActor.Get(); }

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

    /**
     * Batch 4 — M-11: sell value of an item at a vendor — half its VendorPrice,
     * floored at 1 for anything tradeable. Static pure so the economy rule is
     * unit-testable without a world.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Vendor")
    static int32 ComputeVendorSellValue(int32 VendorPrice);

    /**
     * Batch 4 — M-11: server-authoritative purchase. Wares = the items in the
     * NPC's ShopLootTableId loot table; prices = each item's VendorPrice in the
     * NPC's CurrencyItemId. Validates range, ware membership, funds and weight
     * before transferring anything (no partial transactions).
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|NPC|Vendor")
    EAstrawildVendorResult TryPurchase(AActor* Purchaser, FName ItemId, int32 Quantity);

    /**
     * Batch 4 — M-11: server-authoritative sale. Only items with VendorPrice > 0
     * are sellable (prevents minting currency from junk); pays
     * ComputeVendorSellValue per unit in the NPC's currency.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|NPC|Vendor")
    EAstrawildVendorResult TrySell(AActor* Seller, FName ItemId, int32 Quantity);

    /** Batch 8: applies definition appearance (tint lantern + role silhouette). Public — the bootstrapper calls it after assigning NpcDefinition (BeginPlay runs before the assignment). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|NPC")
    void RefreshAppearanceFromDefinition();

    // ------------------------------------------------------------------
    // GDP-4 — NPC affinity (relationship growth with the player).
    // ------------------------------------------------------------------

    /** Current affinity 0..100 (grows by talking/trading; saved per NPC id). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC|Affinity")
    float Affinity = 0.0f;

    /** Relationship tier 0..3 (Stranger / Acquaintance / Friend / Confidant). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Affinity")
    int32 GetAffinityTier() const;

    /** Tier title for prompts/UI. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Affinity")
    FText GetAffinityTierTitle() const;

    /** Vendor discount fraction by tier (0 / 5 / 10 / 15%). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Affinity")
    float GetVendorDiscountFraction() const;

    /** Stable id for save persistence (definition NpcId, NAME_None without one). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Affinity")
    FName GetStableNPCId() const;

    /** Server-side affinity grant with the once-per-in-world-day gate. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|NPC|Affinity")
    void AddAffinity(float Amount);

protected:
    virtual void BeginPlay() override;

private:
    int32 PatrolIndex = 0;
    double LastInteractedTime = -BIG_NUMBER;
    TWeakObjectPtr<AActor> LastInteractedActor;

    /** GDP-4: in-world day of the last affinity gain (once-per-day gate). */
    UPROPERTY()
    int32 LastAffinityGainDay = -1;

    /** Current in-world day (absolute minutes / 1440). */
    int32 GetCurrentWorldDay() const;
};
