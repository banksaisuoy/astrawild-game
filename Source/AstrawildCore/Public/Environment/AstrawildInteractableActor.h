// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/AstrawildInteractableInterface.h"
#include "GameplayTagContainer.h"
#include "AstrawildTypes.h"
#include "AstrawildInteractableActor.generated.h"

class UStaticMeshComponent;
class USphereComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInteractableTriggeredSignature, AActor*, Interactor, AActor*, TargetObject);

UCLASS()
class ASTRAWILDCORE_API AAstrawildInteractableActor : public AActor, public IAstrawildInteractableInterface
{
	GENERATED_BODY()

public:
	AAstrawildInteractableActor();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> TriggerSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable")
	FText PromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable")
	FText DetailedDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable")
	bool bIsOneTimeOnly;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Quest")
	FGameplayTag QuestTargetTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Quest")
	EAstrawildQuestObjectiveType QuestObjectiveType = EAstrawildQuestObjectiveType::Interact;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interactable")
	bool bHasBeenInteracted;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Reward")
	FGameplayTag RewardItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Reward")
	int32 RewardItemQuantity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Survival", meta = (ClampMin = "0.0"))
	float HungerRestoredOnInteract = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interactable|Survival", meta = (ClampMin = "0.0"))
	float ThirstRestoredOnInteract = 0.0f;

	UPROPERTY(BlueprintAssignable, Category = "Interactable|Events")
	FOnInteractableTriggeredSignature OnInteracted;

public:
	// --- IAstrawildInteractableInterface Implementation ---
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual bool PerformInteraction_Implementation(AActor* Interactor) override;
};