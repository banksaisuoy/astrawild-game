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

    /** Final-audit M-7: cached definition MaxQuantity — RespawnNode restores the full node, not the per-harvest rate. */
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Resource")
    int32 CachedMaxQuantity = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", meta=(ClampMin="1"), Replicated)
    int32 RemainingQuantity = 3;

    /** LCP-2: replicated harvest state — mirrors the depleted/respawned visual on clients. */
    UFUNCTION()
    void OnRep_RemainingQuantity();

    /** LCP-2: apply the depleted/respawned visual for the current quantity (pure mirror of the server path). */
    void ApplyQuantityVisual();

    /**
     * LCP-2 (world-free testable): the ONE depleted predicate shared by the
     * server harvest path and the client OnRep mirror — depleted iff finite
     * and quantity <= 0. Pinned by ASTRAWILD.LCP2.DressingGate so the two
     * paths can never drift.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Resource")
    static bool IsNodeDepleted(bool bInfinite, int32 RemainingQuantity);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", Replicated)
    bool bInfiniteResource = false;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", meta=(ClampMin="0.0"))
    float RespawnDurationSeconds = 30.0f;

    /**
     * Production V2 (P0 fix — Master Plan §1 "ResourceItemId bootstrap weakness"):
     * deterministic node identity. When set, BeginPlay resolves EVERY stat from
     * the registered definition — item id, quantities, respawn, hidden-scanner
     * gate, tint + scale. Direct property writes keep working for level-placed
     * actors, but the bootstrapper now only sets this id.
     * LCP-2: replicated — clients resolve the identical definition locally.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", Replicated)
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
