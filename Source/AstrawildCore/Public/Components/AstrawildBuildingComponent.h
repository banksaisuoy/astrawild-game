// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildBuildingComponent.generated.h"

class AAstrawildBuildingPiece;
class UAstrawildInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingPlacedSignature, AAstrawildBuildingPiece*, PlacedBuilding);

UCLASS(ClassGroup = (Astrawild), meta = (BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildBuildingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAstrawildBuildingComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	bool bIsBuildModeActive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TSubclassOf<AAstrawildBuildingPiece> ActiveBuildingClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FGameplayTag ActiveBuildingTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	float MaxPlacementDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TArray<FAstrawildRecipeIngredient> ActiveBuildingCost;

	UPROPERTY(BlueprintAssignable, Category = "Building|Events")
	FOnBuildingPlacedSignature OnBuildingPlaced;

public:
	UFUNCTION(BlueprintCallable, Category = "Building")
	void EnterBuildMode(TSubclassOf<AAstrawildBuildingPiece> InBuildingClass, const FGameplayTag& InBuildingTag, const TArray<FAstrawildRecipeIngredient>& Cost);

	UFUNCTION(BlueprintCallable, Category = "Building")
	void ExitBuildMode();

	UFUNCTION(BlueprintCallable, Category = "Building")
	bool PlaceBuilding();

	UFUNCTION(BlueprintPure, Category = "Building")
	bool CanAffordBuilding() const;

	UFUNCTION(BlueprintPure, Category = "Building")
	bool GetPlacementTransform(FTransform& OutTransform) const;

private:
	FVector CurrentGhostLocation;
	FRotator CurrentGhostRotation;
	bool bIsValidPlacementLocation;

	TWeakObjectPtr<UAstrawildInventoryComponent> CachedInventory;
	UAstrawildInventoryComponent* GetInventory();
};