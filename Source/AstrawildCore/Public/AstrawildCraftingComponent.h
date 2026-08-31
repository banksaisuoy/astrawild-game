#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildCraftingComponent.generated.h"

class UAstrawildRecipeDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildCraftCompleted, FName, RecipeId, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildCraftProgress, FName, RecipeId, float, ProgressFraction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildCraftStarted, FName, RecipeId, float, DurationSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildCraftCancelled, FName, RecipeId, bool, bRefunded);

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

    /** Fired when a timed craft begins (UMG crafting screen hook). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Crafting")
    FAstrawildCraftStarted OnCraftStarted;

    /** Fired when the active craft is cancelled (UMG crafting screen hook). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Crafting")
    FAstrawildCraftCancelled OnCraftCancelled;

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

    /** Client -> server craft request (UMG screens call this from any net role). */
    UFUNCTION(Server, Reliable)
    void ServerRequestCraft(FName RecipeId);

    /** Client -> server cancel request (server refunds ingredients). */
    UFUNCTION(Server, Reliable)
    void ServerRequestCancelCraft();

    /** Cancel the active timed craft and refund ingredients (server only). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Crafting")
    bool CancelActiveCraft();

    /** Progress of the active craft (0..1; 0 when idle) — for UMG progress bars. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting")
    float GetCraftingProgress() const;

    /** Seconds left on the active craft (0 when idle). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting")
    float GetCraftTimeRemaining() const { return FMath::Max(0.0f, CraftTimeRemaining); }

    /** All recipes visible to this player (tech gate only, ignores ingredients/station) — UMG listing source. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting")
    TArray<UAstrawildRecipeDefinition*> GetTechUnlockedRecipes() const;

    /** Station ids currently in use range (UMG "craftable here" indicator). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting")
    TArray<FName> GetNearbyStationIds() const;

protected:
    virtual void BeginPlay() override;

private:
    FName ActiveRecipeId = NAME_None;
    float CraftTimeRemaining = 0.0f;
    float CraftTimeTotal = 0.0f;
    TArray<FAstrawildItemStack> PendingOutputs;

    /**
     * H-11 guard (Production V2): true while completed outputs are held because
     * the pack is full — retries every second until space frees, blocks cancel
     * (a cancel here would refund ingredients AND keep granted outputs).
     */
    bool bOutputsPendingHandoff = false;
    TWeakObjectPtr<AActor> PendingOutputTarget;

    class UAstrawildInventoryComponent* GetInventory() const;
    class UAstrawildItemRegistrySubsystem* GetRegistry() const;
    class UAstrawildResearchSubsystem* GetResearch() const;
    bool IsNearStation(FName StationId) const;
    void CompleteActiveCraft();
};
