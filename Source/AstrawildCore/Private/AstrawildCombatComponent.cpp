#include "AstrawildCombatComponent.h"

#include "AstrawildCore.h"
#include "AstrawildDamageTarget.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEchoBossCharacter.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildLog.h"
#include "AstrawildProjectileActor.h"
#include "AstrawildSurvivalComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

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
    for (const FHitResult& Hit : HitResults)
    {
        AActor* HitActor = Hit.GetActor();
        if (!IsValid(HitActor) || HitActor == OwnerCharacter)
        {
            continue;
        }

        const float BaseDamage = GetOutgoingAttackDamage(bHeavy);
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
    }

    OnAttackExecuted.Broadcast(bHeavy, TotalDamageDealt);
    return true;
}

bool UAstrawildCombatComponent::ExecuteRangedAttack()
{
    // Final production run (PHASE 12): energy-weapon path — validates the ranged
    // weapon + ammo, consumes a cell, then spawns the server-authoritative bolt.
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

    if (World->GetTimeSeconds() - LastRangedAttackTime < RangedAttackCooldown)
    {
        return false;
    }

    // Ammo gate: weapons without AmmoItemId are free; the Pulse Lance burns cells.
    const FName AmmoId = Inventory->GetEquippedAmmoItemId();
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

    // Muzzle: slightly in front of the player's eyes, aimed along the view.
    const FVector AimOrigin = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * ProjectileSpawnOffset + FVector(0.0f, 0.0f, 30.0f);
    const FVector AimDirection = OwnerCharacter->GetActorForwardVector();

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.Instigator = OwnerCharacter;
    AAstrawildProjectileActor* Bolt = World->SpawnActor<AAstrawildProjectileActor>(
        AAstrawildProjectileActor::StaticClass(), AimOrigin, AimDirection.Rotation(), Params);
    if (!Bolt)
    {
        return false;
    }

    Bolt->Launch(AimDirection, GetOutgoingAttackDamage(false), GetResolvedAttackElement(), OwnerCharacter);
    OnAttackExecuted.Broadcast(false, 0.0f);
    return true;
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
    const float Base = bHeavy ? HeavyAttackDamage : LightAttackDamage;
    return Base + GetEquippedWeaponAttackPower();
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
