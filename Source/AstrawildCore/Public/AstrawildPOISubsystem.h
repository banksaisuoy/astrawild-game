#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildPOISubsystem.generated.h"

class AAstrawildPOIMarkerActor;
class UAstrawildPOIDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildPOIDiscovered, FName, PoiId);

/**
 * Production V2 (Master Plan §5): data-driven points of interest — discovery
 * tracking, rewards and persistence for the world's landmarks, ruins, ancient
 * tech and signal sources.
 *
 * The bootstrapper spawns one AAstrawildPOIMarkerActor per registered
 * UAstrawildPOIDefinition; this subsystem sweeps players against every
 * definition's discovery radius (1s cadence, server-side) and:
 *   - grants loot + research on first discovery,
 *   - publishes Event.PoiDiscovered (quest hookable),
 *   - saves discovered ids in save schema v4.
 *
 * Ancient signal sources additionally require a scanner with ancient-signal
 * tracking (bRequiresSignalScanner); every discovery radius doubles while such
 * a scanner is equipped (Master Plan §10 — scanner progression value).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildPOISubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|POI")
    FAstrawildPOIDiscovered OnPOIDiscovered;

    /** Sweep cadence in seconds (server). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|POI")
    float SweepIntervalSeconds = 1.0f;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildPOISubsystem, STATGROUP_Tickables);
    }
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
    {
        return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
    }

    /** True when the POI was already discovered this save. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|POI")
    bool IsPOIDiscovered(FName PoiId) const;

    /** All discovered ids (HUD counters). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|POI")
    TArray<FName> GetDiscoveredPOIIds() const;

    /** Discovered count over the total registered (HUD "N/M explored"). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|POI")
    void GetDiscoveryProgress(int32& OutDiscovered, int32& OutTotal) const;

    /**
     * Effective discovery radius (pure — unit-tested): definition radius, doubled
     * by the ancient-signal scanner bonus when applicable.
     */
    static float ComputeDiscoveryRadius(const UAstrawildPOIDefinition* Definition, bool bHasSignalScanner);

    /** Discover a POI directly (server — resolves rewards once). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|POI")
    bool DiscoverPOI(FName PoiId);

    /** Save/load (schema v4). */
    void ExportForSave(TArray<FName>& OutDiscoveredIds) const;
    void ImportFromSave(const TArray<FName>& InDiscoveredIds);

private:
    UPROPERTY()
    TArray<FName> DiscoveredPOIIds;

    float SweepAccumulator = 0.0f;

    /** Cached markers by POI id (spawned by the bootstrapper). */
    TMap<FName, TWeakObjectPtr<AAstrawildPOIMarkerActor>> Markers;

    void RunSweep();
    void ResolveDiscoveryRewards(const UAstrawildPOIDefinition* Definition);
    class UAstrawildItemRegistrySubsystem* GetRegistry() const;
};
