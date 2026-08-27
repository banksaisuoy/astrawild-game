// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AstrawildAttributeComponent.h"
#include "AstrawildLogChannels.h"

UAstrawildAttributeComponent::UAstrawildAttributeComponent()
	: CurrentHealth(100.0f)
	, MaxHealth(100.0f)
	, bRegenerateHealth(false)
	, HealthRegenRate(2.0f)
	, CurrentStamina(100.0f)
	, MaxStamina(100.0f)
	, StaminaRegenRate(25.0f)
	, StaminaRegenDelay(1.2f)
	, AttackPower(20.0f)
	, DefensePower(10.0f)
	, ElementalAffinity(EAstrawildElement::Neutral)
	, Level(1)
	, CurrentEXP(0.0f)
	, RequiredEXP(100.0f)
	, TimeSinceLastStaminaDrain(0.0f)
	, bIsDead(false)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildAttributeComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	CurrentStamina = MaxStamina;
	bIsDead = false;
}

void UAstrawildAttributeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsDead)
	{
		return;
	}

	// Health regeneration
	if (bRegenerateHealth && CurrentHealth < MaxHealth)
	{
		ModifyHealth(HealthRegenRate * DeltaTime, GetOwner());
	}

	// Stamina regeneration
	TimeSinceLastStaminaDrain += DeltaTime;
	if (TimeSinceLastStaminaDrain >= StaminaRegenDelay && CurrentStamina < MaxStamina)
	{
		const float RegenAmount = StaminaRegenRate * DeltaTime;
		CurrentStamina = FMath::Clamp(CurrentStamina + RegenAmount, 0.0f, MaxStamina);
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, RegenAmount, GetOwner());
	}
}

float UAstrawildAttributeComponent::ModifyHealth(float Delta, AActor* Instigator)
{
	if (bIsDead && Delta < 0.0f)
	{
		return 0.0f;
	}

	const float OldHealth = CurrentHealth;
	CurrentHealth = FMath::Clamp(CurrentHealth + Delta, 0.0f, MaxHealth);
	const float ActualDelta = CurrentHealth - OldHealth;

	if (!FMath::IsNearlyZero(ActualDelta))
	{
		OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, ActualDelta, Instigator);
	}

	if (CurrentHealth <= 0.0f && !bIsDead)
	{
		bIsDead = true;
		UE_LOG(LogAstrawildCombat, Log, TEXT("%s has died. Instigator: %s"), *GetOwner()->GetName(), Instigator ? *Instigator->GetName() : TEXT("None"));
		OnDeath.Broadcast(GetOwner());
	}

	return ActualDelta;
}

bool UAstrawildAttributeComponent::ModifyStamina(float Delta)
{
	if (Delta < 0.0f)
	{
		const float Cost = -Delta;
		if (CurrentStamina < Cost)
		{
			return false; // Insufficient stamina
		}
		CurrentStamina = FMath::Clamp(CurrentStamina - Cost, 0.0f, MaxStamina);
		TimeSinceLastStaminaDrain = 0.0f;
		OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, Delta, GetOwner());
		return true;
	}

	CurrentStamina = FMath::Clamp(CurrentStamina + Delta, 0.0f, MaxStamina);
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, Delta, GetOwner());
	return true;
}

void UAstrawildAttributeComponent::AddExperience(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	CurrentEXP += Amount;
	while (CurrentEXP >= RequiredEXP)
	{
		CurrentEXP -= RequiredEXP;
		Level++;
		RequiredEXP = FMath::RoundToFloat(RequiredEXP * 1.35f);
		MaxHealth += 20.0f;
		MaxStamina += 10.0f;
		AttackPower += 4.0f;
		DefensePower += 2.0f;
		CurrentHealth = MaxHealth;
		CurrentStamina = MaxStamina;

		UE_LOG(LogAstrawild, Log, TEXT("%s leveled up to Level %d!"), *GetOwner()->GetName(), Level);
		OnLevelUp.Broadcast(Level, CurrentEXP);
	}
}

void UAstrawildAttributeComponent::ResetToMax()
{
	bIsDead = false;
	CurrentHealth = MaxHealth;
	CurrentStamina = MaxStamina;
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0.0f, GetOwner());
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, 0.0f, GetOwner());
}