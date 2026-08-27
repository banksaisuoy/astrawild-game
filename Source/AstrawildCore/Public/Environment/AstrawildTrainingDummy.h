// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "Interfaces/AstrawildDamageableInterface.h"
#include "Interfaces/AstrawildInteractableInterface.h"
#include "AstrawildTrainingDummy.generated.h"

class UAstrawildAttributeComponent;
class UStaticMeshComponent;

UCLASS()
class ASTRAWILDCORE_API AAstrawildTrainingDummy : public AActor, public IAstrawildDamageableInterface, public IAstrawildInteractableInterface
{
	GENERATED_BODY()

public:
	AAstrawildTrainingDummy();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> DummyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildAttributeComponent> Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat Stats")
	float TotalDamageTaken;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat Stats")
	int32 TotalHitCount;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat Stats")
	float RecentDPS;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Stats")
	float AutoResetHealthDelay;

	UFUNCTION(BlueprintCallable, Category = "Combat Stats")
	void ResetDummyStats();

	// --- IAstrawildDamageableInterface ---
	virtual float TakeAstrawildDamage_Implementation(const FAstrawildDamageEvent& DamageEvent) override;
	virtual bool CanTakeDamage_Implementation(AActor* Attacker) override;
	virtual EAstrawildElement GetElementalAffinity_Implementation() const override;

	// --- IAstrawildInteractableInterface ---
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual bool PerformInteraction_Implementation(AActor* Interactor) override;

private:
	float TimeSinceLastHit;
	float DamageInCurrentWindow;
	float WindowTimer;
	FVector OriginalScale;
	float FlinchTimer;
};