// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildCombatComponent.generated.h"

class UAstrawildAttributeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnDamageDealtSignature, AActor*, Target, float, Damage, EAstrawildElement, Element, bool, bIsCrit, int32, ComboStep);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageReceivedSignature, AActor*, Instigator, float, Damage, EAstrawildElement, Element, bool, bIsCrit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboStepSignature, int32, ComboStep);

UCLASS(ClassGroup = (Astrawild), meta = (BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAstrawildCombatComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseMeleeRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float BaseMeleeRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float AttackCooldownDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float StaminaCostPerAttack;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CriticalHitChancePercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	float CriticalMultiplier;

	// --- Combo Settings ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Combo")
	int32 CurrentComboIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	float ComboResetTimeout;

	// --- Delegates ---
	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnDamageDealtSignature OnDamageDealt;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnDamageReceivedSignature OnDamageReceived;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnComboStepSignature OnComboStep;

public:
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool PerformMeleeAttack(float DamageMultiplier = 1.0f, EAstrawildElement DamageElement = EAstrawildElement::Neutral);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool PerformMeleeCombo(EAstrawildElement DamageElement = EAstrawildElement::Neutral, const FGameplayTag& StatusToApply = FGameplayTag(), float StatusDuration = 0.0f);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float ApplyDamageToTarget(AActor* TargetActor, float RawDamage, EAstrawildElement DamageElement, AActor* DamageDealer);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float ApplyDamageEvent(AActor* TargetActor, const FAstrawildDamageEvent& DamageEvent);

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanAttack() const;

	UFUNCTION(BlueprintCallable, Category = "Combat|Telegraph")
	void ShowAttackTelegraph(const FVector& Origin, float Radius, float DurationSeconds, const FColor& TelegraphColor);

private:
	float CurrentAttackCooldown;
	float ComboTimer;
	int32 NextAttackInstanceId;
	TSet<TWeakObjectPtr<AActor>> HitActorsThisAttack;

	TWeakObjectPtr<UAstrawildAttributeComponent> CachedAttributes;
	UAstrawildAttributeComponent* GetAttributes();
};