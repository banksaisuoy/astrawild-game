// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "Interfaces/AstrawildInteractableInterface.h"
#include "AstrawildBuildingPiece.generated.h"

class UStaticMeshComponent;
class USphereComponent;
class UAstrawildInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBuildingInteractedSignature, AActor*, Interactor, AActor*, Building);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingDestroyedSignature, AActor*, Building);

UCLASS()
class ASTRAWILDCORE_API AAstrawildBuildingPiece : public AActor, public IAstrawildInteractableInterface
{
	GENERATED_BODY()

public:
	AAstrawildBuildingPiece();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildInventoryComponent> ContainerInventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building Properties")
	FGuid BuildingUniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building Properties")
	FGameplayTag BuildingTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building Properties")
	EAstrawildBuildingType BuildingType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building Properties")
	float MaxHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building Properties")
	float CurrentHealth;

	UPROPERTY(BlueprintAssignable, Category = "Building Events")
	FOnBuildingInteractedSignature OnBuildingInteracted;

	UPROPERTY(BlueprintAssignable, Category = "Building Events")
	FOnBuildingDestroyedSignature OnBuildingDestroyed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building Properties")
	TArray<FAstrawildRecipeIngredient> DismantleRefund;

	UFUNCTION(BlueprintCallable, Category = "Building")
	void Interact(AActor* InteractorActor);

	UFUNCTION(BlueprintCallable, Category = "Building")
	bool DismantleBuilding(AActor* Instigator);

	UFUNCTION(BlueprintCallable, Category = "Building")
	void TakeBuildingDamage(float DamageAmount);

	UFUNCTION(BlueprintPure, Category = "Building")
	FAstrawildBuildingSaveData GetSaveData() const;

	UFUNCTION(BlueprintCallable, Category = "Building")
	void LoadSaveData(const FAstrawildBuildingSaveData& SaveData);

	// --- IAstrawildInteractableInterface Implementation ---
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual bool PerformInteraction_Implementation(AActor* Interactor) override;
};