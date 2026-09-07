#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildCropComponent.generated.h"

class AAstrawildBuildingActor;
class UAstrawildInventoryComponent;

/** Crop lifecycle (directive Phase 8.1 — 7 states including Empty). */
UENUM(BlueprintType)
enum class EAstrawildCropState : uint8
{
    Empty UMETA(DisplayName="Empty"),
    Planted UMETA(DisplayName="Planted"),
    Sprout UMETA(DisplayName="Sprout"),
    Young UMETA(DisplayName="Young"),
    Mature UMETA(DisplayName="Mature"),
    Harvested UMETA(DisplayName="Harvested"),
    Withered UMETA(DisplayName="Withered")
};

/**
 * SCP Phase 8 — farm plot crops (directive [3] Phase 8.1).
 *
 * Attached to Farm Plot buildings. The 7-state lifecycle advances with water
 * (rain auto-waters; dry plots grow at half rate), fertilizer (Item_Compost
 * doubles the rate) and the season multiplier (TimeSubsystem). Mature plots
 * harvest on interact; unharvested mature crops wither after 5 minutes.
 *
 * Save additive: crop fields on FAstrawildBuildingSaveData.
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCropComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildCropComponent();

    /** Seconds from planted to mature under ideal conditions. */
    static constexpr float BaseGrowthSeconds = 300.0f;

    /** Wither window: mature crops spoil after this long unharvested. */
    static constexpr float WitherSeconds = 300.0f;

    // --- Static contracts (automation-tested) ---

    /** One growth step (0..1 progress) — water halves nothing, fertilizer doubles. */
    static float ComputeGrowthStep(float DeltaSeconds, bool bWatered, bool bFertilized, float SeasonMultiplier);

    /** Season multiplier from the month index (directive: 4 seasons). */
    static float ComputeSeasonMultiplier(int32 MonthIndex);

    /** Next state on the ladder from a growth fraction. */
    static EAstrawildCropState ResolveStateFromProgress(float Progress);

    // --- Runtime state ---

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Crop")
    EAstrawildCropState CropState = EAstrawildCropState::Empty;

    /** Seed item planted (NAME_None on empty plots). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Crop")
    FName SeedItemId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Crop")
    float GrowthProgress = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Crop")
    bool bFertilized = false;

    /** Plant a seed (consumes it). Returns false when the plot is occupied. */
    bool PlantSeed(UAstrawildInventoryComponent* Inventory, FName SeedId);

    /** Harvest a mature crop into the inventory. Returns granted item id + count. */
    bool Harvest(UAstrawildInventoryComponent* Inventory, FName& OutItemId, int32& OutQuantity);

    /** Interaction prompt fragment for the owning building. */
    FText GetPlotStatusText() const;

    /** Manual watering (Dew Flask path): refreshes the 90s water window. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Crop")
    void WaterPlot() { WateredSecondsRemaining = 90.0f; }

    // --- Save integration (additive) ---

    void ExportForSave(FName& OutSeedId, uint8& OutState, float& OutProgress, bool& OutFertilized) const;
    void ImportFromSave(FName InSeedId, uint8 InState, float InProgress, bool bInFertilized);

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** Seconds remaining in the watered window (rain refreshes it). */
    float WateredSecondsRemaining = 0.0f;

    /** Mature clock for the wither window. */
    float MatureSeconds = 0.0f;

    /** FCR-1-d (L-d14): last tick delta — the watering decay scales with it. */
    float LastTickDelta = 1.0f;

    bool IsAuthority() const;
    void RefreshWatering();
    int32 GetCurrentMonth() const;
    bool IsRaining() const;
};
