#include "AstrawildCombatComponent.h"

#include "AstrawildAttributeComponent.h"
#include "AstrawildComboSubsystem.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDamageTarget.h"
#include "AstrawildDurabilityComponent.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildProjectileActor.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildVfxActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Net/UnrealNetwork.h"

namespace
{
    // Content Pack CP-03/CP-05/CP-06: Niagara-first dispatch — a bound asset
    // plays when loaded (no synchronous loads, no hitch risk), otherwise the
    // procedural Batch-2 fallback keeps the zero-asset visual language. Fire
    // audio is independent of the muzzle asset so sounds bind first if desired.
    void SpawnWeaponMuzzleFlash(UWorld* World, const UAstrawildWeaponDefinition* WeaponDef,
        const FVector& Location, const FVector& Direction, const FLinearColor& Tint)
    {
        if (World && WeaponDef)
        {
            UNiagaraSystem* MuzzleVfx = WeaponDef->MuzzleFlashVfx.LoadSynchronous();
            if (MuzzleVfx)
            {
                UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, MuzzleVfx, Location, Direction.Rotation());
            }
            else
            {
                AAstrawildBeamVfxActor::SpawnMuzzleFlash(World, Location, Direction, Tint);
            }
            if (USoundBase* FireSfx = WeaponDef->FireSound.LoadSynchronous())
            {
                UGameplayStatics::PlaySoundAtLocation(World, FireSfx, Location);
            }
            return;
        }
        AAstrawildBeamVfxActor::SpawnMuzzleFlash(World, Location, Direction, Tint);
    }

    /** CP-05 impact burst + CP-06 impact audio at a contact point (no-op without bindings). */
    void SpawnWeaponImpact(UWorld* World, const UAstrawildWeaponDefinition* WeaponDef, const FVector& Location)
    {
        if (!World || !WeaponDef)
        {
            return;
        }
        if (UNiagaraSystem* ImpactVfx = WeaponDef->ImpactVfx.LoadSynchronous())
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ImpactVfx, Location);
        }
        if (USoundBase* ImpactSfx = WeaponDef->ImpactSound.LoadSynchronous())
        {
            UGameplayStatics::PlaySoundAtLocation(World, ImpactSfx, Location);
        }
    }
}

namespace
{
    /** Shot tint: elemental payload color, else the weapon family identity. */
    FLinearColor ResolveWeaponVfxTint(const UAstrawildWeaponDefinition* WeaponDef)
    {
        if (WeaponDef)
        {
            if (WeaponDef->Element != EAstrawildElementType::None)
            {
                return FAstrawildVfxPalette::GetElementTint(WeaponDef->Element);
            }
            return FAstrawildVfxPalette::GetWeaponFamilyTint(WeaponDef->Family);
        }
        return FAstrawildVfxPalette::GetElementTint(EAstrawildElementType::None);
    }
}

UAstrawildCombatComponent::UAstrawildCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UAstrawildCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAstrawildCombatComponent, bIsBlocking);
    DOREPLIFETIME(UAstrawildCombatComponent, bReplicatedDodgeTimer);
}

void UAstrawildCombatComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UAstrawildCombatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (DodgeInvulnerabilityRemaining > 0.0f)
    {
        DodgeInvulnerabilityRemaining = FMath::Max(0.0f, DodgeInvulnerabilityRemaining - DeltaTime);
        if (DodgeInvulnerabilityRemaining <= 0.0f)
        {
            OnDodgeStateChanged.Broadcast(false, 0.0f);
        }
    }

    // Batch 3 — Item B: player stagger countdown. The owning character listens to
    // OnStaggerStateChanged and zeroes/restores movement speed accordingly.
    if (StaggerRemainingSeconds > 0.0f)
    {
        StaggerRemainingSeconds = FMath::Max(0.0f, StaggerRemainingSeconds - DeltaTime);
        if (StaggerRemainingSeconds <= 0.0f)
        {
            OnStaggerStateChanged.Broadcast(false, 0.0f);
        }
    }
}

UAstrawildSurvivalComponent* UAstrawildCombatComponent::GetSurvival() const
{
    AActor* Owner = GetOwner();
    return Owner ? Owner->FindComponentByClass<UAstrawildSurvivalComponent>() : nullptr;
}

bool UAstrawildCombatComponent::CanAttack(const bool bHeavy) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    const UAstrawildSurvivalComponent* Survival = GetSurvival();
    if (Survival && Survival->IsDead())
    {
        return false;
    }

    const double Now = World->GetTimeSeconds();
    const double LastTime = bHeavy ? LastHeavyAttackTime : LastLightAttackTime;
    const double Cooldown = bHeavy ? HeavyAttackCooldown : LightAttackCooldown;
    return (Now - LastTime) >= Cooldown;
}

void UAstrawildCombatComponent::RequestLightAttack()
{
    // Final production run (PHASE 12): a ranged weapon (Pulse Lance) reroutes the
    // light-attack input to the projectile path — same button, weapon decides mode.
    const AActor* Owner = GetOwner();
    if (const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr)
    {
        if (Inventory->IsRangedWeaponEquipped())
        {
            RequestRangedAttack();
            return;
        }
    }

    if (CanAttack(false))
    {
        ServerLightAttack();
    }
}

void UAstrawildCombatComponent::RequestRangedAttack()
{
    if (CanAttack(false))
    {
        ServerRangedAttack();
    }
}

void UAstrawildCombatComponent::RequestHeavyAttack()
{
    if (CanAttack(true))
    {
        ServerHeavyAttack();
    }
}

void UAstrawildCombatComponent::RequestDodge(const FVector& Direction)
{
    const UWorld* World = GetWorld();
    const UAstrawildSurvivalComponent* Survival = GetSurvival();
    if (!World || (Survival && Survival->IsDead()))
    {
        return;
    }

    if (World->GetTimeSeconds() - LastDodgeTime < DodgeCooldown)
    {
        return;
    }

    ServerDodge(Direction.IsNearlyZero() ? FVector::ForwardVector : Direction.GetSafeNormal());
}

void UAstrawildCombatComponent::RequestSetBlocking(const bool bBlocking)
{
    if (bIsBlocking != bBlocking)
    {
        ServerSetBlocking(bBlocking);
    }
}

void UAstrawildCombatComponent::ServerLightAttack_Implementation()
{
    ExecuteAttack(false);
}

void UAstrawildCombatComponent::ServerRangedAttack_Implementation()
{
    ExecuteRangedAttack();
}

void UAstrawildCombatComponent::ServerHeavyAttack_Implementation()
{
    ExecuteAttack(true);
}

void UAstrawildCombatComponent::ServerDodge_Implementation(const FVector_NetQuantizeNormal Direction)
{
    UWorld* World = GetWorld();
    UAstrawildSurvivalComponent* Survival = GetSurvival();
    if (!World || !Survival || Survival->IsDead())
    {
        return;
    }

    if (World->GetTimeSeconds() - LastDodgeTime < DodgeCooldown)
    {
        return;
    }

    if (!Survival->TryConsumeStamina(DodgeStaminaCost))
    {
        return;
    }

    LastDodgeTime = World->GetTimeSeconds();
    DodgeInvulnerabilityRemaining = DodgeInvulnerabilitySeconds;
    bReplicatedDodgeTimer = DodgeInvulnerabilitySeconds;
    OnDodgeStateChanged.Broadcast(true, DodgeInvulnerabilitySeconds);
    ApplyDodgeImpulse(FVector(Direction.X, Direction.Y, 0.0f).GetSafeNormal());
}

void UAstrawildCombatComponent::ServerSetBlocking_Implementation(const bool bBlocking)
{
    UAstrawildSurvivalComponent* Survival = GetSurvival();
    if (Survival && Survival->IsDead())
    {
        bIsBlocking = false;
        return;
    }

    if (bIsBlocking != bBlocking)
    {
        bIsBlocking = bBlocking;
        OnBlockingChanged.Broadcast(bIsBlocking);
    }
}

void UAstrawildCombatComponent::ApplyDodgeImpulse(const FVector& Direction)
{
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!OwnerCharacter)
    {
        return;
    }

    if (UCharacterMovementComponent* Movement = OwnerCharacter->GetCharacterMovement())
    {
        // Ground dodge impulse in input direction (or forward).
        const FVector Launch = Direction.IsNearlyZero()
            ? OwnerCharacter->GetActorForwardVector() * DodgeImpulseStrength
            : Direction * DodgeImpulseStrength;
        OwnerCharacter->LaunchCharacter(Launch, false, false);
    }
}

bool UAstrawildCombatComponent::ExecuteAttack(const bool bHeavy)
{
    UWorld* World = GetWorld();
    UAstrawildSurvivalComponent* Survival = GetSurvival();
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (!World || !Survival || !OwnerCharacter || Survival->IsDead() || !CanAttack(bHeavy))
    {
        return false;
    }

    if (bHeavy && !Survival->TryConsumeStamina(HeavyAttackStaminaCost))
    {
        return false;
    }

    if (bHeavy)
    {
        LastHeavyAttackTime = World->GetTimeSeconds();
    }
    else
    {
        LastLightAttackTime = World->GetTimeSeconds();
    }

    // Sweep in front of the character — melee arc via multi-sphere trace.
    const FVector Start = OwnerCharacter->GetActorLocation();
    const FVector End = Start + OwnerCharacter->GetActorForwardVector() * AttackRange;
    TArray<FHitResult> HitResults;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ASTRAWILDCombatAttack), false, OwnerCharacter);

    const float SweepRadius = 90.0f;
    World->SweepMultiByChannel(HitResults, Start, End, FQuat::Identity, ECC_Pawn,
        FCollisionShape::MakeSphere(SweepRadius), QueryParams);

    float TotalDamageDealt = 0.0f;

    // GDP-3: Power Strike — the queued skill multiplies THIS swing's damage and
    // is consumed by it (window set by the smart-cast, spent here, never both).
    AAstrawildPlayerCharacter* PlayerForSkills = Cast<AAstrawildPlayerCharacter>(GetOwner());
    const bool bEmpowered = PlayerForSkills && PlayerForSkills->IsNextMeleeEmpowered();

    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!IsValid(HitActor) || HitActor == OwnerCharacter)
        {
            continue;
        }

        float BaseDamage = GetOutgoingAttackDamage(bHeavy);
        if (bEmpowered)
        {
            BaseDamage *= 2.2f;
        }
        // Batch 3 — Item A: resolve the element per hit (weapon override → tunable fallback)
        // so elemental statuses apply from whichever weapon is equipped.
        const EAstrawildElementType ResolvedElement = GetResolvedAttackElement();
        if (AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(HitActor))
        {
            if (Echo->IsDefeated())
            {
                continue;
            }
            const float Actual = Echo->ApplyElementalDamage(BaseDamage, ResolvedElement);
            TotalDamageDealt += Actual;
        }
        else if (AAstrawildEchoBossCharacter* Boss = Cast<AAstrawildEchoBossCharacter>(HitActor))
        {
            // Boss encounters have their own phase pipeline (directive §24).
            // Batch 6: attacks now resolve their element against the boss's
            // weakness/own element and can inflict status effects — previously
            // bosses skipped the entire elemental layer.
            if (Boss->IsDefeated())
            {
                continue;
            }
            TotalDamageDealt += Boss->ApplyElementalBossDamage(BaseDamage, ResolvedElement);
        }
        else if (AAstrawildDamageTarget* DamageTarget = Cast<AAstrawildDamageTarget>(HitActor))
        {
            if (DamageTarget->IsDefeated())
            {
                continue;
            }
            DamageTarget->ApplyDamage(BaseDamage);
            TotalDamageDealt += BaseDamage;
        }

        // SCP Phase 6: every landed player melee hit drops a combo mark on
        // the victim — party Echo abilities striking the same target inside
        // 3s resolve a Dual-Tech reaction.
        if (UWorld* ComboWorld = GetWorld())
        {
            if (UAstrawildComboSubsystem* Combos = ComboWorld->GetSubsystem<UAstrawildComboSubsystem>())
            {
                Combos->NotifyPlayerMeleeHit(HitActor, bEmpowered);
            }
        }
    }

    // GDP-3: Power Strike spent on this swing + Might XP for connecting hits.
    // FCR-1-b fix (L-b7): the empowered swing is consumed only when the swing
    // CONNECTED — a whiffed swing no longer burns the buff.
    if (bEmpowered && PlayerForSkills && TotalDamageDealt > 0.0f)
    {
        PlayerForSkills->ConsumeEmpoweredMelee();
    }
    if (PlayerForSkills && PlayerForSkills->AttributeComponent && TotalDamageDealt > 0.0f)
    {
        PlayerForSkills->AttributeComponent->AddAttributeXP(EAstrawildAttributeType::Might, bHeavy ? 4.0f : 2.0f);
    }

    // SCP Phase 12: weapon wear only on CONNECTED hits (whiffed swings cost
    // stamina, not durability) — the melee tool path shares the same pool.
    if (PlayerForSkills && PlayerForSkills->DurabilityComponent && TotalDamageDealt > 0.0f)
    {
        PlayerForSkills->DurabilityComponent->ApplyWeaponWear();
    }

    OnAttackExecuted.Broadcast(bHeavy, TotalDamageDealt);
    return true;
}

bool UAstrawildCombatComponent::ExecuteRangedAttack()
{
    // Production V2 (Master Plan §8): the weapon DEFINITION drives the firing
    // archetype — projectile / homing missile / piercing beam / chaining arc —
    // while ammo gating + server authority stay exactly as the Pulse Lance path.
    UWorld* World = GetWorld();
    UAstrawildSurvivalComponent* Survival = GetSurvival();
    ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
    UAstrawildInventoryComponent* Inventory = OwnerCharacter ? OwnerCharacter->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    if (!World || !Survival || !OwnerCharacter || !Inventory || Survival->IsDead())
    {
        return false;
    }

    if (!Inventory->IsRangedWeaponEquipped())
    {
        return false;
    }

    const UAstrawildWeaponDefinition* WeaponDef = GetEquippedWeaponDefinition();
    const float FireInterval = GetRangedFireInterval();
    if (World->GetTimeSeconds() - LastRangedAttackTime < FireInterval)
    {
        return false;
    }

    // Ammo gate: the weapon profile is authoritative; the legacy item field is
    // the fallback (weapons without either are free).
    FName AmmoId = WeaponDef ? WeaponDef->AmmoItemId : Inventory->GetEquippedAmmoItemId();
    if (AmmoId.IsNone())
    {
        AmmoId = Inventory->GetEquippedAmmoItemId();
    }
    if (!AmmoId.IsNone())
    {
        FAstrawildItemStack AmmoCost;
        AmmoCost.ItemId = AmmoId;
        AmmoCost.Quantity = 1;
        if (!Inventory->ConsumeItems(TArray<FAstrawildItemStack>{AmmoCost}))
        {
            return false;
        }
    }

    LastRangedAttackTime = World->GetTimeSeconds();

    // FCR-1-c fix (M-c7): every ranged shot WEARS the weapon (the durability
    // spec says "melee or ranged"; only melee ever called ApplyWeaponWear).
    // Applied once here — the beam/arc/projective branches all pass through.
    if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
    {
        if (Player->DurabilityComponent && GetEquippedWeaponDefinition())
        {
            Player->DurabilityComponent->ApplyWeaponWear();
        }
    }

    // Muzzle: slightly in front of the player's eyes, aimed along the view.
    // Final-audit F-01: the pawn uses orient-to-movement (bUseControllerRotationYaw
    // = false), so the ACTOR forward is the last MOVEMENT direction, not the aim.
    // Ranged fire, beam traces and lock-on must derive from the CONTROL rotation —
    // the same axis the HUD crosshair sits on — or shots diverge up to 180°.
    const FVector AimDirection = OwnerCharacter->GetControlRotation().Vector();
    const FVector AimOrigin = OwnerCharacter->GetActorLocation() + AimDirection * ProjectileSpawnOffset + FVector(0.0f, 0.0f, 30.0f);

    // --- Fire-mode branches (data-driven archetypes) ---

    if (WeaponDef && WeaponDef->FireMode == EAstrawildWeaponFireMode::Beam)
    {
        return ExecuteBeamAttack(WeaponDef, OwnerCharacter);
    }
    if (WeaponDef && WeaponDef->FireMode == EAstrawildWeaponFireMode::ArcChain)
    {
        return ExecuteArcAttack(WeaponDef, OwnerCharacter);
    }

    // Projectile + Homing families share the spawned bolt; homing acquires a lock.
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.Instigator = OwnerCharacter;
    AAstrawildProjectileActor* Bolt = World->SpawnActor<AAstrawildProjectileActor>(
        AAstrawildProjectileActor::StaticClass(), AimOrigin, AimDirection.Rotation(), Params);
    if (!Bolt)
    {
        return false;
    }

    // Production V2 Batch 2 / CP-03: every projectile shot reads with a muzzle
    // flash — Niagara-first when the profile binds one, procedural octahedron
    // otherwise (element/family tinted).
    SpawnWeaponMuzzleFlash(World, WeaponDef, AimOrigin, AimDirection, ResolveWeaponVfxTint(WeaponDef));

    if (WeaponDef)
    {
        AActor* HomingTarget = nullptr;
        if (WeaponDef->FireMode == EAstrawildWeaponFireMode::HomingProjectile)
        {
            HomingTarget = AcquireLockOnTarget(WeaponDef, OwnerCharacter, AimOrigin, AimDirection);
        }
        Bolt->TrailVfxId = WeaponDef->TrailVfxId; // VFX contract — first runtime consumer
        Bolt->SetWeaponVfxAssets(WeaponDef); // CP-05: direct Niagara trail/impact binding.
        Bolt->LaunchFromWeapon(AimDirection, GetRangedDamage(), GetResolvedAttackElement(), OwnerCharacter,
            WeaponDef->ProjectileSpeed, WeaponDef->ProjectileVisualScale, WeaponDef->ProjectileLifetimeSeconds,
            HomingTarget, HomingTarget ? WeaponDef->HomingAcceleration : 0.0f);
    }
    else
    {
        Bolt->Launch(AimDirection, GetOutgoingAttackDamage(false), GetResolvedAttackElement(), OwnerCharacter);
    }

    OnAttackExecuted.Broadcast(false, 0.0f);
    return true;
}

float UAstrawildCombatComponent::ResolveRangedHit(AActor* Target, const float BaseDamage, const EAstrawildElementType Element) const
{
    // One damage vocabulary for every ranged archetype (projectile/beam/arc) —
    // mirrors the melee sweep exactly: Echo → elemental, boss → elemental boss
    // pipeline, damage target → flat. Returns damage actually dealt (0 when the
    // target is not a valid combat participant).
    if (!IsValid(Target))
    {
        return 0.0f;
    }
    if (const AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(Target))
    {
        if (!Echo->IsDefeated())
        {
            return const_cast<AAstrawildEchoCharacter*>(Echo)->ApplyElementalDamage(BaseDamage, Element);
        }
        return 0.0f;
    }
    if (const AAstrawildEchoBossCharacter* Boss = Cast<AAstrawildEchoBossCharacter>(Target))
    {
        if (!Boss->IsDefeated())
        {
            return const_cast<AAstrawildEchoBossCharacter*>(Boss)->ApplyElementalBossDamage(BaseDamage, Element);
        }
        return 0.0f;
    }
    if (const AAstrawildDamageTarget* DamageTarget = Cast<AAstrawildDamageTarget>(Target))
    {
        if (!DamageTarget->IsDefeated())
        {
            const_cast<AAstrawildDamageTarget*>(DamageTarget)->ApplyDamage(BaseDamage);
            return BaseDamage;
        }
    }
    return 0.0f;
}

bool UAstrawildCombatComponent::ExecuteBeamAttack(const UAstrawildWeaponDefinition* WeaponDef, ACharacter* OwnerCharacter)
{
    // Laser/Rail family: instant hitscan down the view axis. The beam pierces
    // through up to PierceCount targets (VFX contract: TrailVfxId beam draw —
    // Antigravity binds the Niagara beam; the damage resolves server-side now).
    UWorld* World = GetWorld();
    if (!World || !WeaponDef || !OwnerCharacter)
    {
        return false;
    }

    // Final-audit F-01: view-axis firing (see ExecuteRangedAttack).
    const FVector AimDirection = OwnerCharacter->GetControlRotation().Vector();
    const FVector Start = OwnerCharacter->GetActorLocation() + AimDirection * ProjectileSpawnOffset + FVector(0.0f, 0.0f, 30.0f);
    const FVector End = Start + AimDirection * WeaponDef->BeamRange;

    TArray<FHitResult> HitResults;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ASTRAWILDBeamAttack), false, OwnerCharacter);
    World->LineTraceMultiByChannel(HitResults, Start, End, ECC_Pawn, QueryParams);

    const int32 MaxTargets = 1 + FMath::Max(0, WeaponDef->PierceCount);
    int32 DamagedTargets = 0;
    float TotalDamage = 0.0f;
    for (const FHitResult& Hit : HitResults)
    {
        if (DamagedTargets >= MaxTargets)
        {
            break;
        }
        AActor* HitActor = Hit.GetActor();
        if (!IsValid(HitActor) || HitActor == OwnerCharacter)
        {
            continue;
        }
        const float Dealt = ResolveRangedHit(HitActor, GetRangedDamage(), GetResolvedAttackElement());
        if (Dealt > 0.0f)
        {
            TotalDamage += Dealt;
            ++DamagedTargets;
        }
    }

    // Production V2 Batch 2 / CP-05: runtime beam placeholder (Niagara stays the
    // upgrade target). The beam draws to the furthest contact or full range; the
    // impact burst + audio land at the terminal point when the profile binds them.
    {
        const FVector BeamEnd = HitResults.Num() > 0 ? HitResults.Last().ImpactPoint : End;
        AAstrawildBeamVfxActor::SpawnBeam(World, Start, BeamEnd, ResolveWeaponVfxTint(WeaponDef));
        SpawnWeaponMuzzleFlash(World, WeaponDef, Start, AimDirection,
            ResolveWeaponVfxTint(WeaponDef));
        if (HitResults.Num() > 0)
        {
            SpawnWeaponImpact(World, WeaponDef, BeamEnd);
        }
    }

    OnAttackExecuted.Broadcast(false, TotalDamage);
    return true;
}

bool UAstrawildCombatComponent::ExecuteArcAttack(const UAstrawildWeaponDefinition* WeaponDef, ACharacter* OwnerCharacter)
{
    // Arc family: hitscan the first target, then the bolt CHAINS to the nearest
    // additional targets (ChainCount) within ChainRadius, each hop dealing
    // ChainDamageFraction of the previous hop — the electric weapon feel.
    UWorld* World = GetWorld();
    if (!World || !WeaponDef || !OwnerCharacter)
    {
        return false;
    }

    // Final-audit F-01: view-axis firing (see ExecuteRangedAttack).
    const FVector AimDirection = OwnerCharacter->GetControlRotation().Vector();
    const FVector Start = OwnerCharacter->GetActorLocation() + AimDirection * ProjectileSpawnOffset + FVector(0.0f, 0.0f, 30.0f);
    const FVector End = Start + AimDirection * WeaponDef->BeamRange;

    FHitResult FirstHit;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ASTRAWILDArcAttack), false, OwnerCharacter);
    if (!World->LineTraceSingleByChannel(FirstHit, Start, End, ECC_Pawn, QueryParams))
    {
        // Blank shot: still a muzzle flash so the trigger reads (ammo was spent).
        SpawnWeaponMuzzleFlash(World, WeaponDef, Start, AimDirection,
            ResolveWeaponVfxTint(WeaponDef));
        OnAttackExecuted.Broadcast(false, 0.0f);
        return true; // The shot fired (blank) — cooldown/ammo already spent.
    }

    // VFX hop chain: muzzle -> first impact -> each zapped target.
    TArray<FVector> VfxHops;
    VfxHops.Add(Start);
    VfxHops.Add(FirstHit.ImpactPoint);

    float TotalDamage = 0.0f;
    AActor* Current = FirstHit.GetActor();
    if (IsValid(Current) && Current != OwnerCharacter)
    {
        TotalDamage += ResolveRangedHit(Current, GetRangedDamage(), GetResolvedAttackElement());
        if (Current)
        {
            VfxHops.Add(Current->GetActorLocation());
        }
    }

    // Chain hops: nearest valid combatant around the previous target.
    int32 HopsLeft = FMath::Max(0, WeaponDef->ChainCount);
    float HopDamage = GetRangedDamage() * FMath::Clamp(WeaponDef->ChainDamageFraction, 0.0f, 1.0f);
    TSet<AActor*> AlreadyZapped;
    AlreadyZapped.Add(Current);
    while (HopsLeft > 0 && IsValid(Current))
    {
        const FVector ChainOrigin = Current->GetActorLocation();
        TArray<FOverlapResult> Overlaps;
        FCollisionQueryParams ChainParams(SCENE_QUERY_STAT(ASTRAWILDArcChain), false, OwnerCharacter);
        World->OverlapMultiByChannel(Overlaps, ChainOrigin, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeSphere(WeaponDef->ChainRadius), ChainParams);

        AActor* NextTarget = nullptr;
        float BestDist = TNumericLimits<float>::Max();
        for (const FOverlapResult& Overlap : Overlaps)
        {
            AActor* Candidate = Overlap.GetActor();
            if (!IsValid(Candidate) || Candidate == OwnerCharacter || AlreadyZapped.Contains(Candidate))
            {
                continue;
            }
            const bool bCombatant = Cast<AAstrawildEchoCharacter>(Candidate) || Cast<AAstrawildEchoBossCharacter>(Candidate) || Cast<AAstrawildDamageTarget>(Candidate);
            if (!bCombatant)
            {
                continue;
            }
            const float Dist = FVector::Dist(ChainOrigin, Candidate->GetActorLocation());
            if (Dist < BestDist)
            {
                BestDist = Dist;
                NextTarget = Candidate;
            }
        }

        if (!NextTarget)
        {
            break;
        }
        TotalDamage += ResolveRangedHit(NextTarget, HopDamage, GetResolvedAttackElement());
        AlreadyZapped.Add(NextTarget);
        VfxHops.Add(NextTarget->GetActorLocation());
        Current = NextTarget;
        HopDamage *= FMath::Clamp(WeaponDef->ChainDamageFraction, 0.0f, 1.0f);
        --HopsLeft;
    }

    // Production V2 Batch 2 / CP-05: jagged lightning placeholder along the hop
    // chain; the impact burst lands at the first contact when the profile binds one.
    AAstrawildBeamVfxActor::SpawnArcChain(World, VfxHops, ResolveWeaponVfxTint(WeaponDef));
    SpawnWeaponMuzzleFlash(World, WeaponDef, Start, AimDirection,
        ResolveWeaponVfxTint(WeaponDef));
    SpawnWeaponImpact(World, WeaponDef, FirstHit.ImpactPoint);

    OnAttackExecuted.Broadcast(false, TotalDamage);
    return true;
}

AActor* UAstrawildCombatComponent::AcquireLockOnTarget(const UAstrawildWeaponDefinition* WeaponDef, ACharacter* OwnerCharacter, const FVector& AimOrigin, const FVector& AimDirection) const
{
    // Missile lock-on: pick the valid combatant with the smallest angle to the
    // view axis inside the weapon's acquisition cone + range. No target = dumb-fire.
    // Iterates only the three combatant classes (never the whole actor list —
    // a missile shot must stay cheap with 200+ creatures in the world).
    UWorld* World = GetWorld();
    if (!World || !WeaponDef || !OwnerCharacter)
    {
        return nullptr;
    }

    const float ConeCos = FMath::Cos(FMath::Clamp(WeaponDef->LockOnConeHalfAngle, 5.0f, 45.0f) * PI / 180.0f);
    const float RangeSq = FMath::Square(FMath::Max(1000.0f, WeaponDef->LockOnRange));

    AActor* Best = nullptr;
    float BestAngleCos = ConeCos;

    const auto ConsiderActor = [&](AActor* Candidate)
    {
        if (!IsValid(Candidate) || Candidate == OwnerCharacter)
        {
            return;
        }
        const FVector ToTarget = Candidate->GetActorLocation() - AimOrigin;
        if (ToTarget.SizeSquared() > RangeSq)
        {
            return;
        }
        const float AngleCos = FVector::DotProduct(AimDirection, ToTarget.GetSafeNormal());
        if (AngleCos >= BestAngleCos)
        {
            BestAngleCos = AngleCos;
            Best = Candidate;
        }
    };

    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        ConsiderActor(*It);
    }
    for (TActorIterator<AAstrawildEchoBossCharacter> It(World); It; ++It)
    {
        ConsiderActor(*It);
    }
    for (TActorIterator<AAstrawildDamageTarget> It(World); It; ++It)
    {
        ConsiderActor(*It);
    }
    return Best;
}

UAstrawildWeaponDefinition* UAstrawildCombatComponent::GetEquippedWeaponDefinition() const
{
    const AActor* Owner = GetOwner();
    if (const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr)
    {
        return Inventory->GetEquippedWeaponDefinition();
    }
    return nullptr;
}

float UAstrawildCombatComponent::GetRangedFireInterval() const
{
    if (const UAstrawildWeaponDefinition* WeaponDef = GetEquippedWeaponDefinition())
    {
        return FMath::Max(0.05f, WeaponDef->FireIntervalSeconds);
    }
    return RangedAttackCooldown;
}

float UAstrawildCombatComponent::GetRangedDamage() const
{
    // Weapon profile damage + the equipped item's flat attack bonus (the item
    // still carries the progression stat; the definition carries behaviour).
    float Damage;
    if (const UAstrawildWeaponDefinition* WeaponDef = GetEquippedWeaponDefinition())
    {
        Damage = FMath::Max(1.0f, WeaponDef->DamagePerHit) + GetEquippedWeaponAttackPower();
        // FCR-1-c fix (M-c7): a BROKEN ranged weapon hits at x0.4 exactly like
        // melee (the durability spec says "melee or ranged"; the ranged pipeline
        // previously ignored the multiplier entirely).
        if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
        {
            if (Player->DurabilityComponent)
            {
                Damage *= Player->DurabilityComponent->GetEquippedWeaponDamageMultiplier();
            }
        }
    }
    else
    {
        Damage = GetOutgoingAttackDamage(false);
    }

    // GDP-3: Instinct-earned Overcharge adds +30% ranged damage while its window
    // is open (the smart-cast sets it; the window decays on the player's Tick).
    if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
    {
        if (Player->GetRangedBuffRemaining() > 0.0f)
        {
            Damage *= 1.3f;
        }
    }
    return Damage;
}

float UAstrawildCombatComponent::GetMitigatedIncomingDamage(const float RawDamage) const
{
    if (IsDodging())
    {
        return 0.0f; // Invulnerability frames (directive §9).
    }
    float Damage = RawDamage;
    if (bIsBlocking)
    {
        Damage *= (1.0f - GetEffectiveBlockMitigation());
    }
    // Batch 3 — Item C: torso armor reduces ALL incoming damage multiplicatively
    // AFTER dodge/block resolution (dodge still fully avoids, block still reduces first).
    Damage *= (1.0f - GetEquippedArmorFraction());
    return Damage;
}

float UAstrawildCombatComponent::GetEffectiveBlockMitigation() const
{
    // Wave 3: an equipped shield replaces the unarmed baseline (never stacks below it).
    const AActor* Owner = GetOwner();
    if (const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr)
    {
        const float ShieldMitigation = Inventory->GetEquippedShieldMitigation();
        if (ShieldMitigation > 0.0f)
        {
            return FMath::Clamp(ShieldMitigation, 0.0f, 0.8f);
        }
    }
    return UnarmedBlockMitigation;
}

float UAstrawildCombatComponent::GetEquippedWeaponAttackPower() const
{
    const AActor* Owner = GetOwner();
    if (const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr)
    {
        return Inventory->GetEquippedWeaponAttackPower();
    }
    return 0.0f;
}

float UAstrawildCombatComponent::GetOutgoingAttackDamage(const bool bHeavy) const
{
    // Wave 3: the equipped weapon adds flat attack power to both attack tiers.
    float Base = bHeavy ? HeavyAttackDamage : LightAttackDamage;
    Base += GetEquippedWeaponAttackPower();

    // GDP-3: Might attribute scales melee output (1 + 4% per level above 1).
    if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
    {
        if (const UAstrawildAttributeComponent* Attributes = Player->AttributeComponent)
        {
            Base *= Attributes->GetMeleeDamageMultiplier();
        }
        // SCP Phase 12: broken weapons hit at 40% — the repair-bench loop has
        // a real mechanical consequence.
        if (const UAstrawildDurabilityComponent* Durability = Player->DurabilityComponent)
        {
            Base *= Durability->GetEquippedWeaponDamageMultiplier();
        }
    }
    return Base;
}

// --- Batch 3 — Item A: element-driven status effects ---

FAstrawildStatusEffect UAstrawildCombatComponent::MakeElementalStatusEffect(const EAstrawildElementType Element, const float SourceDamage)
{
    // One shared element→status vocabulary (directive §9 elemental interactions):
    //   Ember → Burn (4s DoT, DPS scales mildly with the hit that applied it)
    //   Frost → Chill (3s, 50% speed)
    //   Flora → Poison (6s DoT, flat DPS)
    //   Pulse → Shock (0.8s hard slow — a soft stagger without the full state change)
    //   None/Light/Ash → no status (invalid StatusId → callers skip)
    FAstrawildStatusEffect Effect;
    switch (Element)
    {
    case EAstrawildElementType::Ember:
        Effect.StatusId = TEXT("Status.Burning");
        Effect.RemainingSeconds = 4.0f;
        Effect.DamagePerSecond = 2.0f + FMath::Max(0.0f, SourceDamage) * 0.05f;
        Effect.SpeedMultiplier = 1.0f;
        break;
    case EAstrawildElementType::Frost:
        Effect.StatusId = TEXT("Status.Chilled");
        Effect.RemainingSeconds = 3.0f;
        Effect.DamagePerSecond = 0.0f;
        Effect.SpeedMultiplier = 0.5f;
        break;
    case EAstrawildElementType::Flora:
        Effect.StatusId = TEXT("Status.Poisoned");
        Effect.RemainingSeconds = 6.0f;
        Effect.DamagePerSecond = 2.0f;
        Effect.SpeedMultiplier = 1.0f;
        break;
    case EAstrawildElementType::Pulse:
        Effect.StatusId = TEXT("Status.Shocked");
        Effect.RemainingSeconds = 0.8f;
        Effect.DamagePerSecond = 0.0f;
        Effect.SpeedMultiplier = 0.3f;
        break;
    default:
        Effect.StatusId = NAME_None;
        Effect.RemainingSeconds = 0.0f;
        break;
    }
    return Effect;
}

EAstrawildElementType UAstrawildCombatComponent::GetResolvedAttackElement() const
{
    // Item A: weapon Element (when set) overrides the AttackElement tunable —
    // closes the "weapon element override" MEDIUM gap from the audit.
    const AActor* Owner = GetOwner();
    if (const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr)
    {
        if (Inventory->GetEquippedWeaponElement() != EAstrawildElementType::None)
        {
            return Inventory->GetEquippedWeaponElement();
        }
    }
    return AttackElement;
}

// --- Batch 3 — Item C: armor ---

float UAstrawildCombatComponent::ComputeArmorFraction(const float ArmorRating, const float K, const float MaxFraction)
{
    // Pure diminishing-returns formula (testable — see AutomationTests):
    // Rating 0 → 0%; K → 50%; asymptotically approaches the clamp ceiling.
    if (ArmorRating <= 0.0f || K <= 0.0f)
    {
        return 0.0f;
    }
    return FMath::Clamp(ArmorRating / (ArmorRating + K), 0.0f, FMath::Max(0.0f, MaxFraction));
}

float UAstrawildCombatComponent::GetEquippedArmorFraction() const
{
    const AActor* Owner = GetOwner();
    if (const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr)
    {
        // Final production run: torso + helmet ratings sum before the single
        // diminishing-returns formula (PHASE 12 armor progression).
        return ComputeArmorFraction(Inventory->GetTotalArmorRating(), ArmorConstantK, ArmorMaxFraction);
    }
    return 0.0f;
}

// --- Batch 3 — Item B: player stagger ---

void UAstrawildCombatComponent::ApplyStagger(const float Seconds)
{
    if (GetOwnerRole() != ROLE_Authority || Seconds <= 0.0f)
    {
        return;
    }

    // Clamp to a sane ceiling so stacked sources can never perma-lock the player.
    const float Clamped = FMath::Min(Seconds, 2.0f);
    const bool bWasStaggering = StaggerRemainingSeconds > 0.0f;
    StaggerRemainingSeconds = FMath::Max(StaggerRemainingSeconds, Clamped);
    if (!bWasStaggering)
    {
        OnStaggerStateChanged.Broadcast(true, StaggerRemainingSeconds);
    }
}
