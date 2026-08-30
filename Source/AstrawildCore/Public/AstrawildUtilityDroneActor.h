#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildUtilityDroneActor.generated.h"

class AAstrawildPlayerCharacter;
class UStaticMeshComponent;

/**
 * Final production run (PHASE 12 — robotics): the Utility Drone companion.
 * Deployed by consuming a drone item (Item_UtilityDrone). Server-side behavior:
 *   - hovers beside its owner (bobbing placeholder body, zero assets),
 *   - periodically feeds journal observation progress for creatures in range
 *     (scanning without holding the scanner key — passive field research),
 *   - periodically harvests the nearest resource node within range — the loot
 *     flows through the standard node interaction so quests/weight/events all
 *     behave exactly like hand-harvesting.
 *
 * One drone per player (the deploy key recalls an active drone instead of
 * spending another item). Save/load: FAstrawildDroneSaveData.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildUtilityDroneActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildUtilityDroneActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Drone")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    // --- Tunables ---

    /** Hover distance maintained from the owner (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Drone", meta=(ClampMin="50.0"))
    float FollowDistance = 260.0f;

    /** Hover height above the owner's capsule center (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Drone", meta=(ClampMin="0.0"))
    float HoverHeight = 140.0f;

    /** Movement interpolation speed (higher = snappier follow). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Drone", meta=(ClampMin="0.1"))
    float FollowInterpSpeed = 3.0f;

    /** Bobbing amplitude (cm) — cosmetic hover wobble. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Drone", meta=(ClampMin="0.0"))
    float BobAmplitude = 12.0f;

    /** Seconds between auto-scan pulses. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Drone|Scan", meta=(ClampMin="0.5"))
    float ScanIntervalSeconds = 2.0f;

    /** Radius the scanner pulse covers (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Drone|Scan", meta=(ClampMin="100.0"))
    float ScanRadius = 900.0f;

    /** Observation progress added per pulse per creature in range. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Drone|Scan", meta=(ClampMin="0.0"))
    float ScanProgressPerPulse = 4.0f;

    /** Seconds between auto-harvest attempts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Drone|Harvest", meta=(ClampMin="1.0"))
    float HarvestIntervalSeconds = 6.0f;

    /** Radius the harvester arm covers (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Drone|Harvest", meta=(ClampMin="100.0"))
    float HarvestRadius = 700.0f;

    virtual void Tick(float DeltaTime) override;

    /** Server: bind the drone to its owner (called right after deploy). */
    void InitializeForOwner(AAstrawildPlayerCharacter* Owner);

    /** Owner player id (save/load re-linking). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Drone")
    FName GetOwnerPlayerId() const { return OwnerPlayerId; }

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Drone")
    void SetOwnerPlayerId(FName InOwnerPlayerId) { OwnerPlayerId = InOwnerPlayerId; }

protected:
    virtual void BeginPlay() override;

private:
    /** Weak owner — dead/absent owner grounds the drone (no-op tick). */
    TWeakObjectPtr<AAstrawildPlayerCharacter> OwnerPlayer;

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Drone")
    FName OwnerPlayerId = NAME_None;

    float ScanAccumulator = 0.0f;
    float HarvestAccumulator = 0.0f;
    float BobPhase = 0.0f;

    void RunScanPulse();
    void RunHarvestPulse();
    AAstrawildPlayerCharacter* GetOwnerPlayer() const;
};
