#include "AstrawildProjectileActor.h"

#include "AstrawildCombatComponent.h"
#include "AstrawildCore.h"
#include "AstrawildDamageTarget.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildSurvivalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildProjectileActor::AAstrawildProjectileActor()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->InitSphereRadius(12.0f);
    CollisionSphere->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    // The bolt is a trigger-style hazard: it must overlap pawns but resolve hits
    // itself — block so the physics engine reports the first contact.
    CollisionSphere->SetNotifyRigidBodyCollision(true);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    RootComponent = CollisionSphere;

    // Placeholder visual (zero-asset playability — REPLACE_BEFORE_RELEASE with a
    // Niagara beam / asset mesh once art passes exist).
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetupAttachment(RootComponent);
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(SphereMesh.Object);
    }
    VisualMesh->SetWorldScale3D(FVector(0.25f));

    ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
    ProjectileMovement->UpdatedComponent = CollisionSphere;
    ProjectileMovement->InitialSpeed = 6000.0f;
    ProjectileMovement->MaxSpeed = 6000.0f;
    ProjectileMovement->bRotationFollowsVelocity = true;
    ProjectileMovement->bShouldBounce = false;
    // Energy bolt — gravity-free straight line (laser behavior).
    ProjectileMovement->ProjectileGravityScale = 0.0f;

    // Bind the hit callback in the constructor (dynamic delegate — safe pattern).
    CollisionSphere->OnComponentHit.AddDynamic(this, &AAstrawildProjectileActor::OnHit);
}

void AAstrawildProjectileActor::BeginPlay()
{
    Super::BeginPlay();

    // Never collide with whoever fired the bolt.
    if (const AActor* OwnerPtr = OwnerActor.Get())
    {
        CollisionSphere->IgnoreActorWhenMoving(OwnerPtr, true);
    }
}

void AAstrawildProjectileActor::Launch(const FVector& Direction, const float Damage, const EAstrawildElementType InElement, AActor* InOwner)
{
    DamageAmount = FMath::Max(0.0f, Damage);
    Element = InElement;
    OwnerActor = InOwner;

    if (InOwner)
    {
        CollisionSphere->IgnoreActorWhenMoving(InOwner, true);
    }

    const FVector Normalized = Direction.GetSafeNormal();
    if (!Normalized.IsNearlyZero() && ProjectileMovement)
    {
        ProjectileMovement->Velocity = Normalized * ProjectileMovement->InitialSpeed;
    }

    UE_LOG(LogAstrawildCombat, Verbose, TEXT("Projectile launched (damage %.1f, element %d)."), DamageAmount, static_cast<int32>(Element));
}

void AAstrawildProjectileActor::LaunchFromWeapon(const FVector& Direction, const float Damage, const EAstrawildElementType InElement,
    AActor* InOwner, const float Speed, const float InVisualScale, const float InLifetimeSeconds,
    AActor* HomingTarget, const float HomingAcceleration)
{
    // Production V2 (Master Plan §8): weapon-definition flight profile. Everything
    // routes through the legacy Launch so hit resolution stays identical.
    DamageAmount = FMath::Max(0.0f, Damage);
    Element = InElement;
    OwnerActor = InOwner;
    LifetimeSeconds = FMath::Max(0.5f, InLifetimeSeconds);
    VisualScale = FMath::Max(0.05f, InVisualScale);
    VisualMesh->SetWorldScale3D(FVector(VisualScale));

    if (InOwner)
    {
        CollisionSphere->IgnoreActorWhenMoving(InOwner, true);
    }

    if (ProjectileMovement)
    {
        ProjectileMovement->InitialSpeed = FMath::Max(500.0f, Speed);
        ProjectileMovement->MaxSpeed = FMath::Max(500.0f, Speed);

        // Missile lock-on family: home onto the acquired target's root component.
        HomingTargetActor = HomingTarget;
        if (IsValid(HomingTarget) && HomingTarget->GetRootComponent())
        {
            ProjectileMovement->HomingTargetComponent = HomingTarget->GetRootComponent();
            ProjectileMovement->HomingAccelerationMagnitude = FMath::Max(0.0f, HomingAcceleration);
            ProjectileMovement->bIsHomingProjectile = HomingAcceleration > 0.0f;
        }
        else
        {
            ProjectileMovement->bIsHomingProjectile = false;
            ProjectileMovement->HomingAccelerationMagnitude = 0.0f;
        }

        const FVector Normalized = Direction.GetSafeNormal();
        if (!Normalized.IsNearlyZero())
        {
            ProjectileMovement->Velocity = Normalized * ProjectileMovement->InitialSpeed;
        }
    }

    UE_LOG(LogAstrawildCombat, Verbose, TEXT("Weapon projectile launched (damage %.1f, speed %.0f, homing %d)."),
        DamageAmount, Speed, IsValid(HomingTarget) ? 1 : 0);
}

void AAstrawildProjectileActor::OnHit(UPrimitiveComponent* /*HitComponent*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, FVector /*NormalImpulse*/, const FHitResult& /*Hit*/)
{
    // Resolve only on the server (single player shares the same path).
    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    if (!IsValid(OtherActor) || OtherActor == OwnerActor.Get())
    {
        return;
    }

    // Same damage vocabulary as the melee sweep (directive §9) — one pipeline,
    // three target kinds, identical elemental/status/quest behavior.
    if (AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(OtherActor))
    {
        if (!Echo->IsDefeated())
        {
            Echo->ApplyElementalDamage(DamageAmount, Element);
        }
    }
    else if (AAstrawildEchoBossCharacter* Boss = Cast<AAstrawildEchoBossCharacter>(OtherActor))
    {
        if (!Boss->IsDefeated())
        {
            Boss->ApplyElementalBossDamage(DamageAmount, Element);
        }
    }
    else if (AAstrawildDamageTarget* DamageTarget = Cast<AAstrawildDamageTarget>(OtherActor))
    {
        if (!DamageTarget->IsDefeated())
        {
            DamageTarget->ApplyDamage(DamageAmount);
        }
    }
    else if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(OtherActor))
    {
        // Final production run: HOSTILE bolts (boss-fired) hurt players — routed
        // through the player's mitigation pipeline exactly like a boss melee hit.
        // Player-fired bolts can never reach this branch with the owner as victim.
        const bool bHostileBolt = Cast<AAstrawildEchoBossCharacter>(OwnerActor.Get()) != nullptr;
        if (bHostileBolt && Player->IsAlive())
        {
            if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
            {
                const float Mitigated = Player->CombatComponent
                    ? Player->CombatComponent->GetMitigatedIncomingDamage(DamageAmount)
                    : DamageAmount;
                Survival->ApplyDamage(Mitigated);
            }
        }
    }

    if (UWorld* World = GetWorld())
    {
        World->DestroyActor(this);
    }
}

void AAstrawildProjectileActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Lifetime safety net (authority only — clients mirror replicated lifetime).
    if (GetLocalRole() == ROLE_Authority)
    {
        ElapsedSeconds += DeltaTime;
        if (ElapsedSeconds >= LifetimeSeconds && GetWorld())
        {
            GetWorld()->DestroyActor(this);
        }
    }
}
