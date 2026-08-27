// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildTypes.h"
#include "Interfaces/AstrawildInteractableInterface.h"
#include "AstrawildEchoBase.generated.h"

class UAstrawildAttributeComponent;
class UAstrawildCombatComponent;
class UAstrawildEchoDataAsset;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEchoStateChangedSignature, AActor*, Echo, EAstrawildEchoState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEchoRoleAbilityUsedSignature, AActor*, Echo, EAstrawildEchoRole, Role);

UCLASS()
class ASTRAWILDCORE_API AAstrawildEchoBase : public ACharacter, public IAstrawildInteractableInterface
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> FallbackMeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Data")
	TObjectPtr<UAstrawildEchoDataAsset> SpeciesData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Echo Instance")
	FAstrawildEchoInstance InstanceData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Echo State")
	EAstrawildEchoState CurrentState;

	UPROPERTY(BlueprintAssignable, Category = "Echo Events")
	FOnEchoStateChangedSignature OnEchoStateChanged;

	UPROPERTY(BlueprintAssignable, Category = "Echo Events")
	FOnEchoRoleAbilityUsedSignature OnRoleAbilityUsed;

public:
	UFUNCTION(BlueprintCallable, Category = "Echo")
	void InitializeFromSpeciesData(UAstrawildEchoDataAsset* InData, int32 InLevel = 1);

	UFUNCTION(BlueprintCallable, Category = "Echo")
	void SetEchoState(EAstrawildEchoState NewState);

	UFUNCTION(BlueprintCallable, Category = "Echo")
	bool CastAbility(int32 AbilityIndex, AActor* TargetActor);

	UFUNCTION(BlueprintCallable, Category = "Echo|Role")
	bool ActivateRolePerk();

	UFUNCTION(BlueprintPure, Category = "Echo")
	FAstrawildEchoInstance ExportCapturedData() const;

	UFUNCTION(BlueprintCallable, Category = "Echo")
	void ImportCapturedData(const FAstrawildEchoInstance& Data);

	UFUNCTION()
	void HandleDeath(AActor* DeadActor);

	UFUNCTION()
	void HandleHealthChanged(float CurrentHealth, float MaxHealth, float Delta, AActor* Instigator);

	// --- IAstrawildInteractableInterface Implementation ---
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual bool PerformInteraction_Implementation(AActor* Interactor) override;

private:
	void ApplyVisualRepresentation();
};