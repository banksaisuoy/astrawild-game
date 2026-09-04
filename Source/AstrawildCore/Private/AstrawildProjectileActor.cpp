#include "AstrawildProjectileActor.h"

#include "AstrawildCombatComponent.h"
#include "AstrawildComboSubsystem.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDamageTarget.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildVfxActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "ProceduralMeshComponent.h"
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

    // Production V2 Batch 2: element-tinted procedural core. Built server-side
    // on launch (PMC sections never replicate); remote clients keep the neutral
    // constructor sphere above until the Niagara trail lands in the art pass.
    VisualBody = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("VisualBody"));
    VisualBody->SetupAttachment(RootComponent);
    VisualBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VisualBody->SetCastShadow(false);
    VisualBody->bVisibleInRayTracing = false;
    VisualBody->SetVisibility(false); // shown when the tinted core is built

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
    if (AActor* OwnerPtr = OwnerActor.Get())
    {
        CollisionSphere->IgnoreActorWhenMoving(OwnerPtr, true);
    }
}

void AAstrawildProjectileActor::BuildElementCore()
{
    // Vertex-colored low-poly energy core: element tint with a bright leading
    // pole so the bolt visibly streaks nose-first. Same DebugMeshMaterial path
    // as the terrain tiles and Echo bodies.
    if (!VisualBody)
    {
        return;
    }

    const FLinearColor Tint = FAstrawildVfxPalette::GetElementTint(Element);
    const FColor BaseColor = Tint.ToFColor(false);
    const FColor HotColor = FLinearColor(
        FMath::Clamp(Tint.R * 1.45f + 0.08f, 0.0f, 1.0f),
        FMath::Clamp(Tint.G * 1.45f + 0.08f, 0.0f, 1.0f),
        FMath::Clamp(Tint.B * 1.45f + 0.08f, 0.0f, 1.0f), 1.0f).ToFColor(false);

    constexpr int32 Rings = 6;
    constexpr int32 Slices = 8;
    constexpr float Radius = 50.0f; // engine-sphere convention — VisualScale applies outside

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;

    for (int32 Ring = 0; Ring <= Rings; ++Ring)
    {
        const float Phi = PI * static_cast<float>(Ring) / static_cast<float>(Rings);
        for (int32 Slice = 0; Slice <= Slices; ++Slice)
        {
            const float Theta = 2.0f * PI * static_cast<float>(Slice) / static_cast<float>(Slices);
            const FVector Normal(
                FMath::Sin(Phi) * FMath::Cos(Theta),
                FMath::Sin(Phi) * FMath::Sin(Theta),
                FMath::Cos(Phi));
            Vertices.Add(Normal * Radius);
            Normals.Add(Normal);
            UVs.Add(FVector2D(static_cast<float>(Slice) / Slices, static_cast<float>(Ring) / Rings));
            // Nose (front pole) runs hot; the rest carries the element identity.
            Colors.Add(Normal.X > 0.55f ? HotColor : BaseColor);
        }
    }
    for (int32 Ring = 0; Ring < Rings; ++Ring)
    {
        for (int32 Slice = 0; Slice < Slices; ++Slice)
        {
            const int32 A = Ring * (Slices + 1) + Slice;
            const int32 B = (Ring + 1) * (Slices + 1) + Slice;
            const int32 C = (Ring + 1) * (Slices + 1) + Slice + 1;
            const int32 D = Ring * (Slices + 1) + Slice + 1;
            Triangles.Append({ A, B, C, A, C, D });
        }
    }

    VisualBody->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
    if (UMaterial* Material = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial")))
    {
        VisualBody->SetMaterial(0, Material);
    }
    VisualBody->SetVisibility(true);
    if (VisualMesh)
    {
        VisualMesh->SetVisibility(false); // tinted core takes over (server/listen view)
    }
}

void AAstrawildProjectileActor::Launch(const FVector& Direction, const float Damage, const EAstrawildElementType InElement, AActor* InOwner)
{
    DamageAmount = FMath::Max(0.0f, Damage);
    Element = InElement;
    OwnerActor = InOwner;

    BuildElementCore();

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
    VisualBody->SetWorldScale3D(FVector(VisualScale));
    BuildElementCore();

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

void AAstrawildProjectileActor::SetWeaponVfxAssets(const UAstrawildWeaponDefinition* WeaponDef)
{
    // CP-05: copy the profile's direct Niagara bindings (soft — unloaded refs
    // keep the procedural element core; loaded ones attach immediately).
    if (!WeaponDef)
    {
        return;
    }

    TrailVfxAsset = WeaponDef->ProjectileTrailVfx;
    ImpactVfxAsset = WeaponDef->ImpactVfx;

    if (UNiagaraSystem* Trail = TrailVfxAsset.LoadSynchronous())
    {
        UNiagaraFunctionLibrary::SpawnSystemAttached(Trail, CollisionSphere, NAME_None,
            FVector::ZeroVector, FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset,
            /*bAutoDestroy*/ true);
    }
}

void AAstrawildProjectileActor::SetStatusPayload(FName InStatusId, float InStatusSeconds, float InStatusSpeedMultiplier, float InStatusDamagePerSecond)
{
    // FCR-1-a (M-a9): authored ability payload — sanitized (positive seconds,
    // speed clamped to a sane band).
    StatusPayloadId = InStatusId;
    StatusPayloadSeconds = FMath::Max(0.0f, InStatusSeconds);
    StatusPayloadSpeedMultiplier = FMath::Clamp(InStatusSpeedMultiplier, 0.2f, 2.5f);
    StatusPayloadDamagePerSecond = FMath::Max(0.0f, InStatusDamagePerSecond);
}

void AAstrawildProjectileActor::OnHit(UPrimitiveComponent* /*HitComponent*/, AActor* OtherActor, UPrimitiveComponent* /*OtherComp*/, FVector /*NormalImpulse*/, const FHitResult& /*Hit*/)
{
    // CP-05: impact burst on every machine that sees the contact (visual only,
    // before the authority gate — clients get the hit callback through the
    // replicated movement as well).
    if (UNiagaraSystem* Impact = ImpactVfxAsset.LoadSynchronous())
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Impact, GetActorLocation());
    }

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
            const AAstrawildEchoCharacter* SourceEcho = Cast<AAstrawildEchoCharacter>(OwnerActor.Get());

            // FCR-1-a fix (H-a6): party-friendly fire — a bolt from any CAPTURED
            // (player-owned) echo never damages another captured echo. The T-key
            // volley fires the whole party at once; the old caster-only exclusion
            // let stray bolts shred the player's own echoes standing in the line
            // of fire. Wild (uncaptured) sources still hit captured targets — that
            // is a hostile attack, exactly as designed.
            const bool bBothCaptured = SourceEcho && SourceEcho->bCaptured && Echo->bCaptured;
            if (!bBothCaptured)
            {
            float ComboDamage = DamageAmount;
            // SCP Phase 6: party Echo ability bolts striking a player-marked
            // target resolve the Dual-Tech reaction (bonus damage + status).
            if (SourceEcho && GetWorld())
            {
                if (UAstrawildComboSubsystem* Combos = GetWorld()->GetSubsystem<UAstrawildComboSubsystem>())
                {
                    const FAstrawildComboReaction Reaction = Combos->TryResolveEchoAbilityCombo(OtherActor, SourceEcho);
                    if (Reaction.IsValid())
                    {
                        ComboDamage *= Reaction.DamageMultiplier;
                        // FCR-1-d fix (L-d15): the Dual-Tech toast reaches the
                        // PLAYER (GetLastComboName had zero consumers — reactions
                        // happened invisibly).
                        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(
                                GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr))
                        {
                            PC->Notify(FText::FromString(FString::Printf(TEXT("Dual-Tech: %s (x%.1f)"),
                                *Reaction.DisplayName, Reaction.DamageMultiplier)));
                        }
                        if (!Reaction.StatusId.IsNone())
                        {
                            // Hitstop reads as a hard brief slow; other ids map
                            // onto the standard status vocabulary.
                            FAstrawildStatusEffect ComboStatus;
                            ComboStatus.StatusId = Reaction.StatusId;
                            if (Reaction.StatusId == TEXT("Status.Hitstop"))
                            {
                                ComboStatus.RemainingSeconds = UAstrawildComboLibrary::HitstopSeconds;
                                ComboStatus.SpeedMultiplier = UAstrawildComboLibrary::HitstopSpeedMultiplier;
                                ComboStatus.DamagePerSecond = 0.0f;
                            }
                            else
                            {
                                const EAstrawildElementType ReactionElement = Element;
                                ComboStatus = UAstrawildCombatComponent::MakeElementalStatusEffect(ReactionElement, ComboDamage);
                                ComboStatus.StatusId = Reaction.StatusId;
                            }
                            Echo->AddStatusEffect(ComboStatus);
                        }
                    }
                }
            }
            Echo->ApplyElementalDamage(ComboDamage, Element);

            // FCR-1-a fix (M-a9): the AUTHORED ability status lands on hit (the
            // old offensive path discarded Data->StatusId/Seconds/Speed — blind,
            // fear, rally, chill variants never happened as authored).
            if (!StatusPayloadId.IsNone() && StatusPayloadSeconds > 0.0f)
            {
                FAstrawildStatusEffect Payload;
                Payload.StatusId = StatusPayloadId;
                Payload.RemainingSeconds = StatusPayloadSeconds;
                Payload.DamagePerSecond = StatusPayloadDamagePerSecond;
                Payload.SpeedMultiplier = StatusPayloadSpeedMultiplier;
                Echo->AddStatusEffect(Payload);
            }
            }
        }
    }
    else if (AAstrawildEchoBossCharacter* Boss = Cast<AAstrawildEchoBossCharacter>(OtherActor))
    {
        if (!Boss->IsDefeated())
        {
            // FCR-1-d fix (M-d11): Dual-Tech reactions resolve against BOSSES and
            // damage targets too — the old echo-only resolution made the entire
            // combo system dead in every boss fight (where it matters most).
            float BossDamage = DamageAmount;
            if (const AAstrawildEchoCharacter* SourceEcho = Cast<AAstrawildEchoCharacter>(OwnerActor.Get()))
            {
                if (UAstrawildComboSubsystem* Combos = GetWorld() ? GetWorld()->GetSubsystem<UAstrawildComboSubsystem>() : nullptr)
                {
                    const FAstrawildComboReaction Reaction = Combos->TryResolveEchoAbilityCombo(OtherActor, SourceEcho);
                    if (Reaction.IsValid())
                    {
                        BossDamage *= Reaction.DamageMultiplier;
                    }
                }
            }
            Boss->ApplyElementalBossDamage(BossDamage, Element);
        }
    }
    else if (AAstrawildDamageTarget* DamageTarget = Cast<AAstrawildDamageTarget>(OtherActor))
    {
        if (!DamageTarget->IsDefeated())
        {
            // FCR-1-d fix (M-d11): same combo resolution for damage targets.
            float TargetDamage = DamageAmount;
            if (const AAstrawildEchoCharacter* SourceEcho = Cast<AAstrawildEchoCharacter>(OwnerActor.Get()))
            {
                if (UAstrawildComboSubsystem* Combos = GetWorld() ? GetWorld()->GetSubsystem<UAstrawildComboSubsystem>() : nullptr)
                {
                    const FAstrawildComboReaction Reaction = Combos->TryResolveEchoAbilityCombo(OtherActor, SourceEcho);
                    if (Reaction.IsValid())
                    {
                        TargetDamage *= Reaction.DamageMultiplier;
                    }
                }
            }
            DamageTarget->ApplyDamage(TargetDamage);
        }
    }
    else if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(OtherActor))
    {
        // Final production run: HOSTILE bolts hurt players — routed through the
        // player's mitigation pipeline exactly like a boss melee hit.
        // FCR-1-a fix (H-a4): hostility is decided by the OWNER, not the class —
        // the old boss-only gate made WILD hostile casters (Gloomfang, Emberfang…)
        // fire bolts that impacted players and silently vanished. A boss owner OR
        // any uncaptured hostile-definition echo owner makes the bolt hostile.
        // Player-fired bolts can never reach this branch with the owner as victim.
        const AActor* Owner = OwnerActor.Get();
        const AAstrawildEchoCharacter* OwnerEcho = Cast<AAstrawildEchoCharacter>(Owner);
        const bool bHostileBolt = (Cast<AAstrawildEchoBossCharacter>(Owner) != nullptr)
            || (OwnerEcho && !OwnerEcho->bCaptured && OwnerEcho->EchoDefinition && OwnerEcho->EchoDefinition->bHostileToPlayers);
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
