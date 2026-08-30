#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AstrawildNPCAIController.generated.h"

class AAstrawildNPCCharacter;
class AAstrawildEchoCharacter;

/**
 * Batch 8 — living villages: the NPC brain (Docs/ASTRAWILD_VILLAGES_SKIFF.md).
 *
 * Zero-asset C++ state machine patterned on AAstrawildEchoAIController:
 *  - All roles walk a waypoint circuit around their home village, pausing at
 *    each post ("เดินวิ่งได้" — patrol speeds differ from chase speeds).
 *  - Night schedule: villagers gather around the village campfire (21:00-06:00).
 *  - Guards (EAstrawildNPCRole::Guard) defend the village: they aggro hostile
 *    wild Echoes inside the guard radius, chase at run speed and strike with
 *    a cooldown-gated melee tap through the Echo damage pipeline.
 *  - Recently-interacted NPCs pause and face the player (conversation beat).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildNPCAIController : public AAIController
{
    GENERATED_BODY()

public:
    AAstrawildNPCAIController();

    /** Seconds between think steps. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI", meta=(ClampMin="0.1"))
    float ThinkIntervalSeconds = 0.4f;

    /** Hostile-aggro radius for guards (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI|Guard", meta=(ClampMin="500.0"))
    float GuardAggroRadius = 3500.0f;

    /** Guard melee reach (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI|Guard", meta=(ClampMin="100.0"))
    float GuardAttackRange = 220.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI|Guard", meta=(ClampMin="0.2"))
    float GuardAttackCooldownSeconds = 1.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI|Guard", meta=(ClampMin="1.0"))
    float GuardDamage = 14.0f;

    /** Patrol walk speed (cm/s) — villagers stroll. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI", meta=(ClampMin="50.0"))
    float PatrolWalkSpeed = 190.0f;

    /** Chase run speed (cm/s) — guards sprint when defending the village. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI", meta=(ClampMin="100.0"))
    float ChaseRunSpeed = 430.0f;

    /** Idle pause at each waypoint (seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI", meta=(ClampMin="1.0"))
    float WaypointPauseSeconds = 5.0f;

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

protected:
    UFUNCTION()
    void Think();

private:
    TWeakObjectPtr<AAstrawildNPCCharacter> NpcPawn;
    TWeakObjectPtr<AAstrawildEchoCharacter> GuardTarget;
    FTimerHandle ThinkTimerHandle;
    FVector SpawnAnchor = FVector::ZeroVector;
    double NextWaypointTime = 0.0;
    double LastAttackTime = -BIG_NUMBER;

    void ExecutePatrol();
    void ExecuteGuardDuty();
    AAstrawildEchoCharacter* FindNearestHostileEcho(float MaxDistance) const;
    bool IsNight() const;
    void SetNpcWalkSpeed(float NewSpeed);
};
