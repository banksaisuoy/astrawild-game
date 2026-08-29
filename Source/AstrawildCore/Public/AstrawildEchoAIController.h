#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoAIController.generated.h"

class AAstrawildEchoCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/**
 * Echo AI (directive §6): AI Perception (sight) + a C++ state machine that requires
 * zero Behavior Tree assets to function — BT/StateTree can later drive the same states
 * through the documented blackboard key contract below.
 *
 * Behavior is modulated by: personality (directive §5), needs, activity pattern vs
 * world time (§13), weather (§12) and player commands (§10).
 *
 * Blackboard key contract (for future BT assets):
 *   "TargetActor"  - AActor* combat/follow target
 *   "HomeLocation" - FVector spawn anchor
 *   "AIState"      - Enum as uint8 (EAstrawildEchoAIState)
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildEchoAIController : public AAIController
{
    GENERATED_BODY()

public:
    AAstrawildEchoAIController();

    /** Seconds between AI think steps for Tier0. Higher LOD tiers lengthen this. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI", meta=(ClampMin="0.05"))
    float ThinkIntervalSeconds = 0.25f;

    /** Health fraction below which wild Echoes flee (scaled by personality). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI", meta=(ClampMin="0.05", ClampMax="0.9"))
    float BaseFleeHealthFraction = 0.30f;

    /** Attack reach for melee Echoes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI|Combat", meta=(ClampMin="50.0"))
    float AttackRange = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI|Combat", meta=(ClampMin="0.2"))
    float AttackCooldownSeconds = 1.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI|Combat", meta=(ClampMin="1.0"))
    float AttackDamageMultiplier = 1.0f;

    /** Follow distance for captured party Echoes (directive §10). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI|Commands", meta=(ClampMin="100.0"))
    float FollowDistance = 280.0f;

    /** Distance the creature wanders from home while exploring. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|AI", meta=(ClampMin="200.0"))
    float ExploreRadius = 1500.0f;

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|AI")
    EAstrawildEchoAIState GetAIState() const;

protected:
    UFUNCTION()
    void Think();

    void HandlePerception(AActor* Actor, struct FAIStimulus Stimulus);

private:
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    FVector HomeLocation = FVector::ZeroVector;
    TWeakObjectPtr<AAstrawildEchoCharacter> EchoPawn;
    TWeakObjectPtr<AActor> TargetActor;
    FTimerHandle ThinkTimerHandle;
    double LastAttackTime = -BIG_NUMBER;
    double NextWanderTime = 0.0;
    bool bPerceivedThreat = false;

    void TransitionTo(EAstrawildEchoAIState NewState);
    EAstrawildEchoAIState DecideState() const;
    void ExecuteState(float DeltaThinkSeconds);

    // State executors.
    void ExecuteExplore();
    void ExecuteFlee();
    void ExecuteCombat(float DeltaThinkSeconds);
    void ExecuteFollow();
    void ExecuteProtect();
    void ExecuteWork();
    void ExecuteSleep();
    void ExecuteSearchFood();
    void ExecuteSocialize();

    bool TryAttackTarget(AActor* Target, float DeltaThinkSeconds);
    AActor* FindNearestPlayer(float MaxDistance) const;
    AAstrawildEchoCharacter* GetEcho() const;
    class UAstrawildEcosystemSubsystem* GetEcosystem() const;
};
