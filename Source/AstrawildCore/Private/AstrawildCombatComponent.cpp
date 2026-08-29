#include "AstrawildCombatComponent.h"

#include "AstrawildCore.h"
#include "AstrawildDamageTarget.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
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
    if (CanAttack(false))
    {
        ServerLightAttack();
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

        const float BaseDamage = bHeavy ? HeavyAttackDamage : LightAttackDamage;
        if (AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(HitActor))
        {
            if (Echo->IsDefeated())
            {
                continue;
            }
            const float Actual = Echo->ApplyElementalDamage(BaseDamage, AttackElement);
            TotalDamageDealt += Actual;
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

float UAstrawildCombatComponent::GetMitigatedIncomingDamage(const float RawDamage) const
{
    if (IsDodging())
    {
        return 0.0f; // Invulnerability frames (directive §9).
    }
    if (bIsBlocking)
    {
        return RawDamage * (1.0f - BlockMitigation);
    }
    return RawDamage;
}
