#pragma once

#include "CoreMinimal.h"
#include "AstrawildBuildingActor.h"
#include "AstrawildBaseTerminalActor.generated.h"

class AAstrawildPlayerCharacter;

/**
 * SCP Phase 9.1 — the Base Terminal ("Palbox" anchor, directive [3] Phase 9).
 *
 * A specialization of the standard building actor so it rides the EXISTING
 * placement, power, save/load and dismantle machinery:
 *  - Territory: 3500cm claim radius. Once ANY terminal exists, new buildings
 *    only place inside a terminal radius (BuildingComponent consults
 *    IsPlacementAllowed) and pieces left outside decay slowly.
 *  - Base level: 1/2/3 by building count inside the radius (>=8, >=16).
 *  - Garrison cap: 5 / 10 / 20 base-assigned Echoes by level (directive).
 *  - Fast-travel anchor: interacting while another terminal exists offers a
 *    jump to it (energy cost) — the first terminal only reports status.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildBaseTerminalActor : public AAstrawildBuildingActor
{
    GENERATED_BODY()

public:
    AAstrawildBaseTerminalActor();

    /** Territory radius (cm) — directive Phase 9.1: 3500. */
    static constexpr float TerritoryRadius = 3500.0f;

    /** Soft decay applied per minute to buildings outside every territory. */
    static constexpr float OutOfTerritoryDecayPerMinute = 1.5f;

    // --- Static queries (placement validation + roster enforcement) ---

    /** Nearest terminal to a location (null when none exists). */
    static AAstrawildBaseTerminalActor* FindNearestTerminal(const UWorld* World, const FVector& Location);

    /** True when Location sits inside ANY terminal territory. */
    static bool IsInsideTerritory(const UWorld* World, const FVector& Location);

    /**
     * Placement rule: with no terminal in the world everything is allowed
     * (early game); once one exists, new pieces must claim inside a territory.
     */
    static bool IsPlacementAllowed(const UWorld* World, const FVector& Location);

    /** Base level from the count of buildings inside the radius (>=8 -> 2, >=16 -> 3). */
    static int32 ComputeBaseLevel(int32 BuildingCountInRadius);

    /** Garrison cap per base level (directive: 5 / 10 / 20). */
    static int32 GetGarrisonCapForLevel(int32 Level);

    // --- Instance state ---

    /** Current base level (recomputed as buildings rise). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Base")
    int32 GetBaseLevel() const { return BaseLevel; }

    /** Current garrison cap for this base's level. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Base")
    int32 GetGarrisonCap() const { return GetGarrisonCapForLevel(BaseLevel); }

    /** Buildings currently inside the territory (non-terminal, non-destroyed). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Base")
    int32 CountBuildingsInRadius() const;

    virtual void Tick(float DeltaTime) override;
    virtual void Interact_Implementation(AActor* InteractingActor) override;

protected:
    virtual void BeginPlay() override;

private:
    /** Recompute base level from the live building count. */
    void RefreshBaseLevel();

    /** Slow decay of buildings outside every territory (60s cadence). */
    void ApplyTerritoryDecay();

    float DecayAccumulator = 0.0f;
    int32 BaseLevel = 1;
};
