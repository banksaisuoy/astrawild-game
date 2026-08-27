// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildCombatComponent.generated.h"

class UAstrawildAttributeComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageDealtSignature, AActor*, Target, float, Damage, EAstrawildElement, Element, bool, bIsCrit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnDamageReceivedSignature, AActor*, Instigator, float, Damage, EAstrawildElement, Element, bool, bIsCrit);

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

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnDamageDealtSignature OnDamageDealt;

	UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
	FOnDamageReceivedSignature OnDamageReceived;

public:
	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool PerformMeleeAttack(float DamageMultiplier = 1.0f, EAstrawildElement DamageElement = EAstrawildElement::Neutral);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	float ApplyDamageToTarget(AActor* TargetActor, float RawDamage, EAstrawildElement DamageElement, AActor* DamageDealer);

	UFUNCTION(BlueprintPure, Category = "Combat")
	bool CanAttack() const;

private:
	float CurrentAttackCooldown;
	TWeakObjectPtr<UAstrawildAttributeComponent> CachedAttributes;
	UAstrawildAttributeComponent* GetAttributes();
};