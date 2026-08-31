#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildProjectileActor.generated.h"

class UProjectileMovementComponent;
class UStaticMeshComponent;
class USphereComponent;
class UProceduralMeshComponent;
class UNiagaraSystem;
class UAstrawildWeaponDefinition;

/**
 * Final production run (PHASE 12 — advanced weapons): server-spawned energy
 * projectile for ranged weapons (Pulse Lance laser path). Mirrors the melee
 * damage vocabulary exactly — Echo -> ApplyElementalDamage, boss ->
 * ApplyElementalBossDamage, damage target -> ApplyDamage — so elemental
 * statuses, weaknesses and quest credit behave identically at range.
 *
 * Production V2 (Master Plan §8): the weapon definition now drives the
 * flight profile — speed, scale, lifetime and optional homing (missile
 * lock-on family) resolve from UAstrawildWeaponDefinition data.
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

    /** Production V2 Batch 2: element-tinted procedural core (server/listen view). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Projectile")
    TObjectPtr<UProceduralMeshComponent> VisualBody;

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

    /** Visual scale of the placeholder bolt (weapon families differ visibly). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Projectile", meta=(ClampMin="0.05"))
    float VisualScale = 0.35f;

    /** VFX contract id (Antigravity binds NS_AW_Weap_<TrailVfxId> to the trail). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Projectile")
    FName TrailVfxId = NAME_None;

    // --- Content Pack CP-05 (additive): direct Niagara bindings ---

    /** Trail system bound from the weapon profile (plays attached; unset = procedural core only). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Projectile|VFX")
    TSoftObjectPtr<UNiagaraSystem> TrailVfxAsset;

    /** Impact burst spawned at the contact point (unset = no impact FX). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Projectile|VFX")
    TSoftObjectPtr<UNiagaraSystem> ImpactVfxAsset;

    virtual void BeginPlay() override;

    /**
     * CP-05: copies the weapon profile's direct Niagara bindings and attaches the
     * trail when loaded (no sync load — unloaded refs stay procedural). Call before
     * LaunchFromWeapon so the trail follows from the first frame of flight.
     */
    void SetWeaponVfxAssets(const UAstrawildWeaponDefinition* WeaponDef);

    /** Production V2 Batch 2: builds the element-tinted energy core (VisualBody). */
    void BuildElementCore();

    /** Server: initialize the payload and launch direction. */
    void Launch(const FVector& Direction, float Damage, EAstrawildElementType InElement, AActor* InOwner);

    /**
     * Production V2: weapon-definition launch — speed/scale/lifetime from data,
     * optional homing target (missile lock-on). Falls back to Launch defaults.
     */
    void LaunchFromWeapon(const FVector& Direction, float Damage, EAstrawildElementType InElement,
        AActor* InOwner, float Speed, float InVisualScale, float InLifetimeSeconds,
        AActor* HomingTarget, float HomingAcceleration);

protected:
    /** Component-hit callback: resolve damage against the hit actor, then die. */
    UFUNCTION()
    void OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

private:
    /** Weak owner reference — never dereference after death, only for ignore checks. */
    TWeakObjectPtr<AActor> OwnerActor;

    /** Homing target (missiles) — steering handled by ProjectileMovement homing fields. */
    TWeakObjectPtr<AActor> HomingTargetActor;

    float ElapsedSeconds = 0.0f;

    virtual void Tick(float DeltaTime) override;
};
