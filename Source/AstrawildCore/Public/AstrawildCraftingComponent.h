#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildCraftingComponent.generated.h"

class UAstrawildRecipeDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildCraftCompleted, FName, RecipeId, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildCraftProgress, FName, RecipeId, float, ProgressFraction);

/**
 * Timed, tech-gated crafting (directive §15). One craft at a time per component.
 * Station gating: recipes with RequiredStationId only craft near a matching station.
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCraftingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildCraftingComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Crafting")
    FAstrawildCraftCompleted OnCraftCompleted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Crafting")
    FAstrawildCraftProgress OnCraftProgress;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Full gate check: ingredients + tech + station proximity. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting")
    bool CanCraft(const UAstrawildRecipeDefinition* Recipe) const;

    /** Gate check without station proximity (for UI listing). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting")
    bool CanCraftIgnoringStation(const UAstrawildRecipeDefinition* Recipe) const;

    /** Start a timed craft (server). Instant recipes complete immediately. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Crafting")
    bool CraftRecipe(const UAstrawildRecipeDefinition* Recipe);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Crafting")
    bool CraftByRecipeId(FName RecipeId);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting")
    bool IsCrafting() const { return ActiveRecipeId != NAME_None; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting")
    FName GetActiveRecipeId() const { return ActiveRecipeId; }

protected:
    virtual void BeginPlay() override;

private:
    FName ActiveRecipeId = NAME_None;
    float CraftTimeRemaining = 0.0f;
    float CraftTimeTotal = 0.0f;
    TArray<FAstrawildItemStack> PendingOutputs;
    TWeakObjectPtr<AActor> PendingOutputTarget;

    class UAstrawildInventoryComponent* GetInventory() const;
    class UAstrawildItemRegistrySubsystem* GetRegistry() const;
    class UAstrawildResearchSubsystem* GetResearch() const;
    bool IsNearStation(FName StationId) const;
    void CompleteActiveCraft();
};
