#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTurretComponent.generated.h"

class AAstrawildEchoCharacter;

/**
 * SCP Phase 11 — automated base defense turret (directive [3] Phase 11.2).
 *
 * Attached to Defense-category buildings (Building_DefenseTurret). While the
 * building is powered, the turret tracks the nearest hostile Echo inside its
 * range and fires the standard projectile pipeline at a fixed cadence.
 * Unpowered turrets sit dark (the power grid stays meaningful for defense).
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildTurretComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildTurretComponent();

    /** Engagement range (cm). */
    static constexpr float TurretRange = 2500.0f;

    /** Shots per second while a target is tracked. */
    static constexpr float FireIntervalSeconds = 1.5f;

    /** Damage per bolt. */
    static constexpr float BoltDamage = 30.0f;

    /** Target selection — static for automation tests (nearest hostile). */
    static AAstrawildEchoCharacter* SelectTarget(const TArray<AAstrawildEchoCharacter*>& Candidates,
        const FVector& TurretLocation);

    /** Static range gate (pure math for tests). */
    static bool IsInRange(const FVector& A, const FVector& B, float Range);

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    float FireCooldownRemaining = 0.0f;

    bool IsAuthority() const;
    void FireAt(AAstrawildEchoCharacter* Target);
};
