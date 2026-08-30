#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildProjectileActor.generated.h"

class UProjectileMovementComponent;
class UStaticMeshComponent;
class USphereComponent;

/**
 * Final production run (PHASE 12 — advanced weapons): server-spawned energy
 * projectile for ranged weapons (Pulse Lance laser path). Mirrors the melee
 * damage vocabulary exactly — Echo -> ApplyElementalDamage, boss ->
 * ApplyElementalBossDamage, damage target -> ApplyDamage — so elemental
 * statuses, weaknesses and quest credit behave identically at range.
 *
 * Server-authoritative: the server spawns, simulates and resolves the hit;
 * clients see the replicated actor travel (visual-only on remotes).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildProjectileActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildProjectileActor();

    /** Collision + visual root (placeholder sphere — engine primitive, zero assets). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Projectile")
    TObjectPtr<USphereComponent> CollisionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Projectile")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Projectile")
    TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

    /** Damage payload (resolved by the combat component before spawn). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Projectile", meta=(ClampMin="0.0"))
    float DamageAmount = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Projectile")
    EAstrawildElementType Element = EAstrawildElementType::Pulse;

    /** Seconds before self-destruct (safety net so bolts never live forever). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Projectile", meta=(ClampMin="0.5"))
    float LifetimeSeconds = 5.0f;

    virtual void BeginPlay() override;

    /** Server: initialize the payload and launch direction. */
    void Launch(const FVector& Direction, float Damage, EAstrawildElementType InElement, AActor* InOwner);

protected:
    /** Component-hit callback: resolve damage against the hit actor, then die. */
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
    /** Weak owner reference — never dereference after death, only for ignore checks. */
    TWeakObjectPtr<AActor> OwnerActor;

    float ElapsedSeconds = 0.0f;

    virtual void Tick(float DeltaTime) override;
};
