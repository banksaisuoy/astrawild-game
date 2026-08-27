// Copyright Epic Games, Inc. / ASTRAWILD Team. All Rights Reserved.

#include "Components/AstrawildMechaComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

UAstrawildMechaComponent::UAstrawildMechaComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildMechaComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAstrawildMechaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsMechaActive)
	{
		return;
	}

	// Heat dissipation
	if (CurrentHeat > 0.0f)
	{
		CurrentHeat = FMath::Max(0.0f, CurrentHeat - (HeatCoolingRate * DeltaTime));
		if (bIsOverheated && CurrentHeat < 20.0f)
		{
			bIsOverheated = false;
		}
	}

	// Energy recharge
	if (!bIsOverboosting && CurrentEnergy < MaxEnergy && !bIsOverheated)
	{
		CurrentEnergy = FMath::Min(MaxEnergy, CurrentEnergy + (EnergyRechargeRate * DeltaTime));
		OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy, CurrentHeat);
	}

	// Energy drain during flight / overboost
	if (bIsFlying || bIsOverboosting)
	{
		const float DrainMultiplier = bIsOverboosting ? 180.0f : 40.0f;
		CurrentEnergy = FMath::Max(0.0f, CurrentEnergy - (DrainMultiplier * DeltaTime));
		OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy, CurrentHeat);

		if (CurrentEnergy <= 0.0f)
		{
			SetFlightActive(false);
			TriggerOverboost(false);
			bIsOverheated = true;
			CurrentHeat = 100.0f;
		}
	}

	// Shield passive regeneration
	if (CurrentShield < MaxShield && !bIsOverheated)
	{
		CurrentShield = FMath::Min(MaxShield, CurrentShield + (50.0f * DeltaTime));
		OnShieldChanged.Broadcast(CurrentShield, MaxShield);
	}
}

bool UAstrawildMechaComponent::EquipMechaFrame(const FAstrawildMechaFrameRow& FrameData)
{
	ActiveFrameData = FrameData;
	MaxEnergy = FrameData.MaxEnergy;
	CurrentEnergy = MaxEnergy;
	EnergyRechargeRate = FrameData.EnergyRechargeRate;
	MaxShield = FrameData.MaxShieldHP;
	CurrentShield = MaxShield;
	CurrentHeat = 0.0f;
	bIsOverheated = false;
	bIsMechaActive = true;

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (OwnerChar && OwnerChar->GetCharacterMovement())
	{
		OwnerChar->GetCharacterMovement()->MaxWalkSpeed = FrameData.GroundRunSpeed;
	}

	OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy, CurrentHeat);
	OnShieldChanged.Broadcast(CurrentShield, MaxShield);
	return true;
}

void UAstrawildMechaComponent::EjectMechaFrame()
{
	bIsMechaActive = false;
	SetFlightActive(false);
	TriggerOverboost(false);

	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (OwnerChar && OwnerChar->GetCharacterMovement())
	{
		OwnerChar->GetCharacterMovement()->MaxWalkSpeed = 500.0f; // Return to standard sprint
	}
}

void UAstrawildMechaComponent::SetFlightActive(bool bActive)
{
	if (bActive && (CurrentEnergy <= 50.0f || bIsOverheated))
	{
		return;
	}

	bIsFlying = bActive;
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (OwnerChar && OwnerChar->GetCharacterMovement())
	{
		if (bIsFlying)
		{
			OwnerChar->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
			OwnerChar->GetCharacterMovement()->MaxFlySpeed = ActiveFrameData.FlightCruiseSpeed;
		}
		else
		{
			OwnerChar->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
	}

	OnFlightStateChanged.Broadcast(bIsFlying);
}

void UAstrawildMechaComponent::TriggerOverboost(bool bEnable)
{
	if (bEnable && (CurrentEnergy <= 100.0f || bIsOverheated))
	{
		return;
	}

	bIsOverboosting = bEnable;
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (OwnerChar && OwnerChar->GetCharacterMovement())
	{
		if (bIsOverboosting)
		{
			OwnerChar->GetCharacterMovement()->MaxFlySpeed = ActiveFrameData.OverboostSpeed;
			OwnerChar->GetCharacterMovement()->MaxWalkSpeed = ActiveFrameData.OverboostSpeed;
		}
		else
		{
			OwnerChar->GetCharacterMovement()->MaxFlySpeed = ActiveFrameData.FlightCruiseSpeed;
			OwnerChar->GetCharacterMovement()->MaxWalkSpeed = ActiveFrameData.GroundRunSpeed;
		}
	}
}

bool UAstrawildMechaComponent::FireHardpointWeapon(EAstrawildMechaHardpoint Slot, FVector TargetLocation)
{
	if (!bIsMechaActive || bIsOverheated || CurrentEnergy < 15.0f)
	{
		return false;
	}

	// Consume energy & generate heat
	CurrentEnergy = FMath::Max(0.0f, CurrentEnergy - 20.0f);
	CurrentHeat = FMath::Min(100.0f, CurrentHeat + 12.0f);
	if (CurrentHeat >= 100.0f)
	{
		bIsOverheated = true;
	}

	OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy, CurrentHeat);
	return true;
}

void UAstrawildMechaComponent::ActivateBeamSaberMelee()
{
	if (!bIsMechaActive || bIsOverheated)
	{
		return;
	}

	CurrentHeat = FMath::Min(100.0f, CurrentHeat + 8.0f);
	OnEnergyChanged.Broadcast(CurrentEnergy, MaxEnergy, CurrentHeat);
}