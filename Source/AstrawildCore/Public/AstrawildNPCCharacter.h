#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildInteractable.h"
#include "AstrawildTypes.h"
#include "AstrawildNPCCharacter.generated.h"

class UAstrawildNPCDefinition;
class UStaticMeshComponent;

/**
 * NPC base (directive §26): interaction offers quests from its NPC definition.
 * Schedule/dialogue/faction architecture-ready; conversation data assets arrive
 * with the content pass (see Docs/ASTRAWILD_QUEST_SYSTEM.md).
 *
 * Batch 4 — M-11: NPCs with a ShopLootTableId + CurrencyItemId on their definition
 * are vendors. Interacting with one lists its wares + prices in a HUD toast; the
 * transaction itself runs through TryPurchase/TrySell (server-authoritative).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildNPCCharacter : public ACharacter, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildNPCCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    TObjectPtr<UAstrawildNPCDefinition> NpcDefinition;

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

protected:
    virtual void BeginPlay() override;
};
