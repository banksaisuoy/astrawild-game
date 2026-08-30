#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildBossHazardActor.generated.h"

class UStaticMeshComponent;

/**
 * Final production run (PHASE 14 — arena hazards): a lingering elemental hazard
 * zone the boss scatters around the arena in phase 2+. Damages players standing
 * inside on a tick cadence (server-authoritative, routed through the player's
 * combat component so armor/dodge rules apply), then dissipates.
 *
 * REPLACE_BEFORE_RELEASE: swap the placeholder sphere for a Niagara field.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildBossHazardActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildBossHazardActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    /** Hazard radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="50.0"))
    float HazardRadius = 220.0f;

    /** Total lifetime before dissipating (s). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="1.0"))
    float LifetimeSeconds = 12.0f;

    /** Damage per second applied to players inside (pre-mitigation). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="0.0"))
    float DamagePerSecond = 6.0f;

    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

private:
    float Elapsed = 0.0f;
    float DamageAccumulator = 0.0f;

    void ApplyHazardDamage(float DeltaSeconds);
};
