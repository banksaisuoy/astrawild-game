// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildAttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnAttributeChangedSignature, float, CurrentValue, float, MaxValue, float, Delta, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorDeathSignature, AActor*, DeadActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelUpSignature, int32, NewLevel, float, RemainingEXP);

UCLASS(ClassGroup = (Astrawild), meta = (BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildAttributeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAstrawildAttributeComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// --- Health ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Health")
	float CurrentHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Health")
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Health")
	bool bRegenerateHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Health")
	float HealthRegenRate;

	// --- Stamina ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Stamina")
	float CurrentStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Stamina")
	float MaxStamina;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Stamina")
	float StaminaRegenRate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Stamina")
	float StaminaRegenDelay;

	// --- Combat Attributes ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Combat")
	float AttackPower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Combat")
	float DefensePower;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Combat")
	EAstrawildElement ElementalAffinity;

	// --- Progression ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Progression")
	int32 Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Progression")
	float CurrentEXP;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attributes|Progression")
	float RequiredEXP;

	// --- Delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnAttributeChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnAttributeChangedSignature OnStaminaChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnActorDeathSignature OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Attributes|Events")
	FOnLevelUpSignature OnLevelUp;

public:
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	float ModifyHealth(float Delta, AActor* Instigator = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	bool ModifyStamina(float Delta);

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void AddExperience(float Amount);

	UFUNCTION(BlueprintPure, Category = "Attributes")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetHealthPercent() const { return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetStaminaPercent() const { return MaxStamina > 0.0f ? CurrentStamina / MaxStamina : 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void ResetToMax();

private:
	float TimeSinceLastStaminaDrain;
	bool bIsDead;
};