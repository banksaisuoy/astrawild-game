#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildSpoilageSubsystem.generated.h"

class AAstrawildBuildingActor;
class UAstrawildInventoryComponent;
class UAstrawildItemRegistrySubsystem;

/**
 * SCP Phase 12.2 — food spoilage + preservation (directive [3] Phase 12).
 *
 * World subsystem ticking at a fixed 5-second cadence (server):
 *  - ages every perishable stack in every player inventory,
 *  - slows aging x10 while the owning player stands in an Ice Box preservation
 *    radius (stacks being "stored cold"),
 *  - on the deadline converts the stack into Item_SpoiledOrganics (half
 *    quantity, floor 1) — the composting input for the farm loop.
 *
 * Save additive: TMap<FName, float> FoodFreshness (remaining seconds per item
 * id; absent = fresh). Single-player keyed like the save inventory itself.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildSpoilageSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildSpoilageSubsystem();

    /** Ice Box preservation factor (directive: x10 slower spoilage). */
    static constexpr float IceBoxSlowdownFactor = 0.1f;

    /** Fixed tick cadence — spoilage advances in discrete steps (server). */
    static constexpr float TickCadenceSeconds = 5.0f;

    // FTickableGameObject interface.
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }
    virtual bool IsTickable() const override;

    /** Remaining freshness (seconds) for an item id (full = unknown entry). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Spoilage")
    float GetFreshness(FName ItemId) const;

    /** True while the player is inside any Ice Box preservation radius. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Spoilage")
    bool IsPlayerPreserved() const;

    // --- Save integration (additive v5, no schema bump) ---

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Spoilage")
    TMap<FName, float> ExportForSave() const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Spoilage")
    void ImportFromSave(const TMap<FName, float>& InFreshness);

    /** Compute the spoil step for one stack — static for automation tests. */
    static float ComputeSpoilStep(float Freshness, float PerishableSeconds, float DeltaSeconds, bool bPreserved);

    /** Conversion outcome for a fully-aged stack — static for automation tests. */
    static int32 ComputeSpoiledConversion(int32 StackQuantity);

private:
    /** Remaining seconds keyed by item id (entries missing = fresh). */
    TMap<FName, float> FoodFreshness;

    float TickAccumulator = 0.0f;

    bool bPlayerPreserved = false;

    void AdvanceSpoilage(float DeltaSeconds);

    UAstrawildItemRegistrySubsystem* GetRegistry() const;

    /** Finds any Ice Box building within PreservationRadius of the player. */
    bool QueryPreservedNearPlayer() const;
};
