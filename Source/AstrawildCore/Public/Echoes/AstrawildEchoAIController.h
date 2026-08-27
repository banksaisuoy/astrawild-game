// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoAIController.generated.h"

class AAstrawildEchoBase;

UCLASS()
class ASTRAWILDCORE_API AAstrawildEchoAIController : public AAIController
{
	GENERATED_BODY()

public:
	AAstrawildEchoAIController();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Properties")
	float WanderRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Properties")
	float AggroDetectionRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Properties")
	float CombatAttackRange;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Properties")
	float LeashDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Properties|Optimization")
	float SimulationLODCloseDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Properties|Optimization")
	float SimulationLODDormantDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Debug")
	bool bShowAIDebug;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State")
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI State")
	TWeakObjectPtr<AActor> FollowLeaderActor;

public:
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetFollowLeader(AActor* Leader);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetCombatTarget(AActor* InTarget);

	UFUNCTION(BlueprintCallable, Category = "AI")
	void ToggleAIDebug();

private:
	FVector HomeLocation;
	float TimeUntilNextWander;
	float CombatCooldownTimer;
	float ThrottledTickTimer;
	TWeakObjectPtr<AAstrawildEchoBase> ControlledEcho;

	void UpdateWildPassive(float DeltaTime);
	void UpdateWildHostile(float DeltaTime);
	void UpdateFleeing(float DeltaTime);
	void UpdateSummonedCompanion(float DeltaTime);
	void UpdateReturnToTerritory(float DeltaTime);
	void DrawAIDebugVisuals();
};