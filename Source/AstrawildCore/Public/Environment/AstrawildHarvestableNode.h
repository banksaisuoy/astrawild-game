// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "Interfaces/AstrawildInteractableInterface.h"
#include "AstrawildHarvestableNode.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnNodeHarvestedSignature, AActor*, Harvester, const FGameplayTag&, ItemTag, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeDepletedSignature, AActor*, NodeActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNodeRespawnedSignature, AActor*, NodeActor);

UCLASS()
class ASTRAWILDCORE_API AAstrawildHarvestableNode : public AActor, public IAstrawildInteractableInterface
{
	GENERATED_BODY()

public:
	AAstrawildHarvestableNode();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest Node Properties")
	FGuid NodeUniqueId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest Node Properties")
	EAstrawildHarvestType HarvestType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest Node Properties")
	FGameplayTag PrimaryResourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest Node Properties")
	int32 MinYieldPerHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest Node Properties")
	int32 MaxYieldPerHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest Node Properties")
	FGameplayTag RareSecondaryResourceTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest Node Properties", meta = (ClampMin = "0", ClampMax = "100"))
	int32 RareDropChancePercent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest Node Properties")
	float MaxNodeHealth;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Harvest Node State")
	float CurrentNodeHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Harvest Node Properties")
	float RespawnDurationSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Harvest Node State")
	bool bIsDepleted;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Harvest Node State")
	float CurrentRespawnTimer;

	UPROPERTY(BlueprintAssignable, Category = "Harvest Node Events")
	FOnNodeHarvestedSignature OnNodeHarvested;

	UPROPERTY(BlueprintAssignable, Category = "Harvest Node Events")
	FOnNodeDepletedSignature OnNodeDepleted;

	UPROPERTY(BlueprintAssignable, Category = "Harvest Node Events")
	FOnNodeRespawnedSignature OnNodeRespawned;

public:
	UFUNCTION(BlueprintCallable, Category = "Harvesting")
	bool Harvest(float ToolPower, EAstrawildHarvestType ToolType, AActor* HarvesterActor, int32& OutQuantityHarvested);

	UFUNCTION(BlueprintCallable, Category = "Harvesting")
	void DepleteNode();

	UFUNCTION(BlueprintCallable, Category = "Harvesting")
	void RespawnNode();

	UFUNCTION(BlueprintPure, Category = "Harvesting")
	FAstrawildHarvestNodeSaveData GetSaveData() const;

	UFUNCTION(BlueprintCallable, Category = "Harvesting")
	void LoadSaveData(const FAstrawildHarvestNodeSaveData& SaveData);

	// --- IAstrawildInteractableInterface Implementation ---
	virtual FText GetInteractionPrompt_Implementation(AActor* Interactor) override;
	virtual bool CanInteract_Implementation(AActor* Interactor) override;
	virtual bool PerformInteraction_Implementation(AActor* Interactor) override;
};