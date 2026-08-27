// Copyright Epic Games, Inc. / ASTRAWILD Team. All Rights Reserved.

#include "Components/AstrawildMechaComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UAstrawildMechaComponent::UAstrawildMechaComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildMechaComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UAstrawildMechaComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bIsMechaActive || !HasAuthorityForMecha())
    {
        return;
    }

    HardpointCooldownRemaining = FMath::Max(0.0f, HardpointCooldownRemaining - DeltaTime);
    if (CurrentHeat > 0.0f)
    {
        CurrentHeat = FMath::Max(0.0f, CurrentHeat - (FMath::Max(0.0f, HeatCoolingRate) * DeltaTime));
        if (bIsOverheated && CurrentHeat < 20.0f)
        {
            bIsOverheated = false;
        }
    }
    if (!bIsOverboosting && CurrentEnergy < MaxEnergy && !bIsOverheated)
    {
        CurrentEnergy = FMath::Min(MaxEnergy, CurrentEnergy + (FMath::Max(0.0f, EnergyRechargeRate) * DeltaTime));
    }
    if (bIsFlying || bIsOverboosting)
    {
        const float DrainPerSecond = bIsOverboosting ? 180.0f : 40.0f;
        CurrentEnergy = FMath::Max(0.0f, CurrentEnergy - (DrainPerSecond * DeltaTime));
        if (CurrentEnergy <= 0.0f)
        {
            TriggerOverboost(false);
            SetFlightActive(false);
            bIsOverheated = true;
            CurrentHeat = 100.0f;
        }
    }
    if (CurrentShield < MaxShield && !bIsOverheated)
    {
        CurrentShield = FMath::Min(MaxShield, CurrentShield + (50.0f * DeltaTime));
    }
    OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy, CurrentHeat);
    OnShieldChanged.Broadcast(CurrentShield, MaxShield);
}

bool UAstrawildMechaComponent::EquipMechaFrame(const FAstrawildMechaFrameRow& FrameData)
{
    if (!HasAuthorityForMecha() || !FrameData.FrameTag.IsValid())
    {
        return false;
    }
    ActiveFrameData = FrameData;
    MaxEnergy = FMath::Max(1.0f, FrameData.MaxEnergy);
    CurrentEnergy = MaxEnergy;
    EnergyRechargeRate = FMath::Max(0.0f, FrameData.EnergyRechargeRate);
    MaxShield = FMath::Max(0.0f, FrameData.MaxShieldHP);
    CurrentShield = MaxShield;
    CurrentHeat = 0.0f;
    HardpointCooldownRemaining = 0.0f;
    bIsOverheated = false;
    bIsOverboosting = false;
    bIsMechaActive = true;

    if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* Movement = OwnerChar->GetCharacterMovement())
        {
            Movement->MaxWalkSpeed = FMath::Max(0.0f, FrameData.GroundRunSpeed);
        }
    }
    OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy, CurrentHeat);
    OnShieldChanged.Broadcast(CurrentShield, MaxShield);
    return true;
}

void UAstrawildMechaComponent::EjectMechaFrame()
{
    if (!HasAuthorityForMecha())
    {
        return;
    }
    TriggerOverboost(false);
    SetFlightActive(false);
    bIsMechaActive = false;
    EquippedWeaponTag = FGameplayTag::EmptyTag;
    CurrentHeat = 0.0f;
    if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* Movement = OwnerChar->GetCharacterMovement())
        {
            Movement->MaxWalkSpeed = 500.0f;
        }
    }
}

void UAstrawildMechaComponent::SetFlightActive(const bool bActive)
{
    if (!HasAuthorityForMecha() || (bActive && (!bIsMechaActive || CurrentEnergy <= 50.0f || bIsOverheated)))
    {
        return;
    }
    bIsFlying = bActive;
    if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* Movement = OwnerChar->GetCharacterMovement())
        {
            if (bIsFlying)
            {
                Movement->SetMovementMode(MOVE_Flying);
                Movement->MaxFlySpeed = FMath::Max(0.0f, ActiveFrameData.FlightCruiseSpeed);
            }
            else
            {
                Movement->SetMovementMode(MOVE_Walking);
            }
        }
    }
    OnFlightStateChanged.Broadcast(bIsFlying);
}

void UAstrawildMechaComponent::TriggerOverboost(const bool bEnable)
{
    if (!HasAuthorityForMecha() || (bEnable && (!bIsMechaActive || CurrentEnergy <= 100.0f || bIsOverheated)))
    {
        return;
    }
    bIsOverboosting = bEnable;
    if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
    {
        if (UCharacterMovementComponent* Movement = OwnerChar->GetCharacterMovement())
        {
            Movement->MaxFlySpeed = bIsOverboosting ? ActiveFrameData.OverboostSpeed : ActiveFrameData.FlightCruiseSpeed;
            Movement->MaxWalkSpeed = bIsOverboosting ? ActiveFrameData.OverboostSpeed : ActiveFrameData.GroundRunSpeed;
        }
    }
}

bool UAstrawildMechaComponent::FireHardpointWeapon(const EAstrawildMechaHardpoint Slot, const FVector TargetLocation)
{
    if (!HasAuthorityForMecha() || !bIsMechaActive || bIsOverheated || HardpointCooldownRemaining > 0.0f || TargetLocation.ContainsNaN())
    {
        return false;
    }
    const FAstrawildMechaWeaponRow* Weapon = FindWeaponForSlot(Slot);
    if (!Weapon || !Weapon->WeaponTag.IsValid() || CurrentEnergy < FMath::Max(0.0f, Weapon->EnergyCostPerShot))
    {
        return false;
    }
    EquippedWeaponTag = Weapon->WeaponTag;
    CurrentEnergy = FMath::Max(0.0f, CurrentEnergy - FMath::Max(0.0f, Weapon->EnergyCostPerShot));
    CurrentHeat = FMath::Clamp(CurrentHeat + FMath::Max(0.0f, Weapon->HeatGeneratedPerShot), 0.0f, 100.0f);
    HardpointCooldownRemaining = Weapon->FireRate > 0.0f ? 1.0f / Weapon->FireRate : 0.1f;
    if (CurrentHeat >= 100.0f)
    {
        bIsOverheated = true;
    }

    // The native component owns the authoritative hit validation. Blueprint can still
    // provide projectile/Niagara presentation, but a weapon request must not ignore
    // the target position or apply client-only damage.
    if (AActor* OwnerActor = GetOwner())
    {
        const FVector TraceStart = OwnerActor->GetActorLocation();
        FHitResult Hit;
        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(AstrawildMechaWeaponTrace), true, OwnerActor);
        if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TargetLocation, ECC_Visibility, QueryParams))
        {
            if (AActor* HitActor = Hit.GetActor())
            {
                const FVector ShotDirection = (TargetLocation - TraceStart).GetSafeNormal();
                UGameplayStatics::ApplyPointDamage(HitActor, FMath::Max(0.0f, Weapon->BaseDamage), ShotDirection, Hit, OwnerActor->GetInstigatorController(), OwnerActor, nullptr);
            }
        }
    }

    OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy, CurrentHeat);
    return true;
}

void UAstrawildMechaComponent::ActivateBeamSaberMelee()
{
    if (!HasAuthorityForMecha() || !bIsMechaActive || bIsOverheated)
    {
        return;
    }
    const FAstrawildMechaWeaponRow* Weapon = FindWeaponForSlot(EAstrawildMechaHardpoint::SecondaryLeftHand);
    if (Weapon && CurrentEnergy >= FMath::Max(0.0f, Weapon->EnergyCostPerShot))
    {
        EquippedWeaponTag = Weapon->WeaponTag;
        CurrentEnergy = FMath::Max(0.0f, CurrentEnergy - FMath::Max(0.0f, Weapon->EnergyCostPerShot));
        CurrentHeat = FMath::Clamp(CurrentHeat + FMath::Max(0.0f, Weapon->HeatGeneratedPerShot), 0.0f, 100.0f);
        HardpointCooldownRemaining = Weapon->FireRate > 0.0f ? 1.0f / Weapon->FireRate : 0.1f;
    }
    else
    {
        CurrentHeat = FMath::Clamp(CurrentHeat + 8.0f, 0.0f, 100.0f);
    }
    if (CurrentHeat >= 100.0f)
    {
        bIsOverheated = true;
    }
    OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy, CurrentHeat);
}

const FAstrawildMechaWeaponRow* UAstrawildMechaComponent::FindWeaponForSlot(const EAstrawildMechaHardpoint Slot) const
{
    if (!WeaponTable)
    {
        return nullptr;
    }
    TArray<FAstrawildMechaWeaponRow*> Rows;
    WeaponTable->GetAllRows<FAstrawildMechaWeaponRow>(TEXT("AstrawildMechaWeaponLookup"), Rows);
    for (const FGameplayTag& DefaultTag : ActiveFrameData.DefaultWeaponTags)
    {
        if (const FAstrawildMechaWeaponRow* Preferred = Rows.FindByPredicate([Slot, DefaultTag](const FAstrawildMechaWeaponRow* Row)
        {
            return Row && Row->HardpointSlot == Slot && Row->WeaponTag == DefaultTag;
        }))
        {
            return Preferred;
        }
    }
    return Rows.FindByPredicate([Slot](const FAstrawildMechaWeaponRow* Row)
    {
        return Row && Row->HardpointSlot == Slot;
    });
}

bool UAstrawildMechaComponent::HasAuthorityForMecha() const
{
    return !GetOwner() || GetOwner()->HasAuthority();
}
