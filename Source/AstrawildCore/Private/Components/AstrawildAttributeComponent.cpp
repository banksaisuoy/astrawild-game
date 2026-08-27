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

void UAstrawildAttributeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearAllStatusEffects();
	Super::EndPlay(EndPlayReason);
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

	UpdateStatusEffects(DeltaTime);
}

void UAstrawildAttributeComponent::UpdateStatusEffects(float DeltaTime)
{
	for (int32 i = ActiveStatusEffects.Num() - 1; i >= 0; --i)
	{
		FAstrawildActiveStatusEffect& Effect = ActiveStatusEffects[i];
		Effect.RemainingDuration -= DeltaTime;
		Effect.TimeUntilNextTick -= DeltaTime;

		// Tick periodic effect (e.g. Burn damage)
		if (Effect.TimeUntilNextTick <= 0.0f)
		{
			Effect.TimeUntilNextTick = Effect.TickInterval;
			if (Effect.StatusTag.ToString().Contains(TEXT("Ignited")) || Effect.StatusTag.ToString().Contains(TEXT("Burn")))
			{
				const float BurnDamage = FMath::Max(2.0f, Effect.Magnitude * 5.0f);
				ModifyHealth(-BurnDamage, Effect.InstigatorActor.Get());
				UE_LOG(LogAstrawildCombat, Log, TEXT("%s took %.1f periodic burn damage."), *GetOwner()->GetName(), BurnDamage);
			}
		}

		if (Effect.RemainingDuration <= 0.0f)
		{
			const FGameplayTag RemovedTag = Effect.StatusTag;
			ActiveStatusEffects.RemoveAt(i);
			UE_LOG(LogAstrawildCombat, Log, TEXT("Status effect %s expired on %s."), *RemovedTag.ToString(), *GetOwner()->GetName());
			OnStatusEffectRemoved.Broadcast(RemovedTag);
		}
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
		ClearAllStatusEffects();
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
			return false;
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
	ClearAllStatusEffects();
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, 0.0f, GetOwner());
	OnStaminaChanged.Broadcast(CurrentStamina, MaxStamina, 0.0f, GetOwner());
}

void UAstrawildAttributeComponent::ApplyStatusEffect(const FGameplayTag& StatusTag, float Duration, float Magnitude, AActor* Instigator)
{
	if (!StatusTag.IsValid() || Duration <= 0.0f || bIsDead)
	{
		return;
	}

	// Check if already active -> refresh duration
	for (FAstrawildActiveStatusEffect& Existing : ActiveStatusEffects)
	{
		if (Existing.StatusTag == StatusTag)
		{
			Existing.RemainingDuration = FMath::Max(Existing.RemainingDuration, Duration);
			Existing.Magnitude = FMath::Max(Existing.Magnitude, Magnitude);
			Existing.InstigatorActor = Instigator;
			UE_LOG(LogAstrawildCombat, Log, TEXT("Refreshed status effect %s on %s for %.1fs"), *StatusTag.ToString(), *GetOwner()->GetName(), Duration);
			return;
		}
	}

	FAstrawildActiveStatusEffect NewEffect;
	NewEffect.StatusTag = StatusTag;
	NewEffect.RemainingDuration = Duration;
	NewEffect.TotalDuration = Duration;
	NewEffect.Magnitude = Magnitude;
	NewEffect.TickInterval = 1.0f;
	NewEffect.TimeUntilNextTick = 1.0f;
	NewEffect.InstigatorActor = Instigator;

	ActiveStatusEffects.Add(NewEffect);
	UE_LOG(LogAstrawildCombat, Log, TEXT("Applied status effect %s to %s for %.1fs"), *StatusTag.ToString(), *GetOwner()->GetName(), Duration);
	OnStatusEffectApplied.Broadcast(StatusTag, Duration);
}

void UAstrawildAttributeComponent::RemoveStatusEffect(const FGameplayTag& StatusTag)
{
	for (int32 i = ActiveStatusEffects.Num() - 1; i >= 0; --i)
	{
		if (ActiveStatusEffects[i].StatusTag == StatusTag)
		{
			ActiveStatusEffects.RemoveAt(i);
			UE_LOG(LogAstrawildCombat, Log, TEXT("Removed status effect %s from %s"), *StatusTag.ToString(), *GetOwner()->GetName());
			OnStatusEffectRemoved.Broadcast(StatusTag);
			return;
		}
	}
}

bool UAstrawildAttributeComponent::HasStatusEffect(const FGameplayTag& StatusTag) const
{
	for (const FAstrawildActiveStatusEffect& Effect : ActiveStatusEffects)
	{
		if (Effect.StatusTag == StatusTag)
		{
			return true;
		}
	}
	return false;
}

void UAstrawildAttributeComponent::ClearAllStatusEffects()
{
	for (const FAstrawildActiveStatusEffect& Effect : ActiveStatusEffects)
	{
		OnStatusEffectRemoved.Broadcast(Effect.StatusTag);
	}
	ActiveStatusEffects.Empty();
}