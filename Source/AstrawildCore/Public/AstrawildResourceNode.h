#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildInteractable.h"
#include "AstrawildResourceNode.generated.h"

class UStaticMeshComponent;
class UAstrawildResourceNodeDefinition;

UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildResourceNode : public AActor, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildResourceNode();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Resource")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource")
    FName ResourceItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", meta=(ClampMin="1"))
    int32 ResourceQuantityPerHarvest = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", meta=(ClampMin="1"))
    int32 RemainingQuantity = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource")
    bool bInfiniteResource = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", meta=(ClampMin="0.0"))
    float RespawnDurationSeconds = 30.0f;

    /**
     * Production V2 (P0 fix — Master Plan §1 "ResourceItemId bootstrap weakness"):
     * deterministic node identity. When set, BeginPlay resolves EVERY stat from
     * the registered definition — item id, quantities, respawn, hidden-scanner
     * gate, tint + scale. Direct property writes keep working for level-placed
     * actors, but the bootstrapper now only sets this id.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource")
    FName NodeDefinitionId = NAME_None;

    /** Resolved definition (null when the id is unset or unknown). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Resource")
    UAstrawildResourceNodeDefinition* GetNodeDefinition() const;

    /** True while the node needs a scanner with hidden-resource detection to harvest. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Resource")
    bool RequiresScannerDetection() const;

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

protected:
    virtual void BeginPlay() override;

private:
    FTimerHandle RespawnTimerHandle;
    void RespawnNode();

    /** Definition-driven identity resolution (P0: never depends on fallback loot). */
    void ApplyNodeDefinition();

    /** Shape-by-rarity differentiation (placeholder visuals — zero assets). */
    void ApplyRarityShape(const UAstrawildResourceNodeDefinition* Def);

    /** Best-effort placeholder tint (no-op when the material has no Color param). */
    void ApplyVisualTint(const FLinearColor& Tint);

    UPROPERTY() TObjectPtr<UStaticMesh> CommonShapeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> UncommonShapeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> RareShapeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> EpicShapeMesh;
};
