// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoBase.generated.h"

class UAstrawildAttributeComponent;
class UAstrawildCombatComponent;
class UAstrawildEchoDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEchoStateChangedSignature, AActor*, Echo, EAstrawildEchoState, NewState);

UCLASS()
class ASTRAWILDCORE_API AAstrawildEchoBase : public ACharacter
{
	GENERATED_BODY()

public:
	AAstrawildEchoBase();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildAttributeComponent> Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildCombatComponent> Combat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Data")
	TObjectPtr<UAstrawildEchoDataAsset> SpeciesData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Echo State")
	EAstrawildEchoState CurrentState;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Echo State")
	FGuid CapturedEchoGuid;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Echo State")
	FText Nickname;

	UPROPERTY(BlueprintAssignable, Category = "Echo Events")
	FOnEchoStateChangedSignature OnEchoStateChanged;

public:
	UFUNCTION(BlueprintCallable, Category = "Echo")
	void InitializeFromSpeciesData(UAstrawildEchoDataAsset* InData);

	UFUNCTION(BlueprintCallable, Category = "Echo")
	void SetEchoState(EAstrawildEchoState NewState);

	UFUNCTION(BlueprintCallable, Category = "Echo")
	bool CastAbility(int32 AbilityIndex, AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "Echo")
	FAstrawildCapturedEchoData ExportCapturedData() const;

	UFUNCTION(BlueprintCallable, Category = "Echo")
	void ImportCapturedData(const FAstrawildCapturedEchoData& Data);

	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth, float Delta, AActor* Instigator);
};