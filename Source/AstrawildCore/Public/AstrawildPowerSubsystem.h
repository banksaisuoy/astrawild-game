#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "AstrawildPowerSubsystem.generated.h"

class AAstrawildBuildingActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildPowerGridChanged, float, TotalGeneration, float, TotalDraw);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildPowerStateChanged, bool, bGridPowered);

/**
 * Base power grid (directive §17): generators produce, consumers draw, batteries buffer.
 * Buildings auto-connect into one grid by proximity (ConnectivityRadius). The subsystem
 * re-solves the network every ResolveInterval seconds on the server — never per frame.
 * Brownout rule: when demand exceeds generation+storage, lowest-priority consumers
 * (Decoration first) lose power first.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildPowerSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildPowerSubsystem();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Power")
    FAstrawildPowerGridChanged OnGridChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Power")
    FAstrawildPowerStateChanged OnPowerStateChanged;

    /** Buildings within this distance connect to the same grid (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Power", meta=(ClampMin="100.0"))
    float ConnectivityRadius = 1200.0f;

    /** Seconds between grid re-solves. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Power", meta=(ClampMin="0.5"))
    float ResolveIntervalSeconds = 2.0f;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    void RegisterBuilding(AAstrawildBuildingActor* Building);
    void UnregisterBuilding(AAstrawildBuildingActor* Building);

    /** Grid totals for UI. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Power")
    float GetTotalGeneration() const { return TotalGeneration; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Power")
    float GetTotalDraw() const { return TotalDraw; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Power")
    float GetStoredEnergy() const { return StoredEnergy; }

    /** Is the consumer building at this location currently powered? */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Power")
    bool IsLocationPowered(const FVector& Location) const;

    bool IsBuildingPowered(const AAstrawildBuildingActor* Building) const;

    /**
     * Batch 2 — Item C: force an immediate grid re-solve outside the natural tick
     * cadence. Used by SaveSubsystem::LoadWorld so the first frame the player sees
     * after loading is already correctly powered (no 2s lamp-flicker window).
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Power")
    void ResolveGridNow();

    /**
     * Final production run (save v3): restore the buffered battery charge after a
     * load — the grid previously reset to zero every reload.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Power")
    void SetStoredEnergy(float InStoredEnergy);

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    TArray<TWeakObjectPtr<AAstrawildBuildingActor>> Buildings;
    TMap<FObjectKey, bool> BuildingPowerState;

    float TotalGeneration = 0.0f;
    float TotalDraw = 0.0f;
    float TotalBatteryCapacity = 0.0f;
    float StoredEnergy = 0.0f;
    float ResolveAccumulator = 0.0f;
    bool bGridPowered = false;

    void ResolveGrid();
};
