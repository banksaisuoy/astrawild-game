// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AstrawildTypes.h"
#include "AstrawildItemDataAsset.generated.h"

UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildItemDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UAstrawildItemDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Identification")
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Identification")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Identification")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	EAstrawildItemCategory Category;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (ClampMin = "1", ClampMax = "999"))
	int32 MaxStackSize;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	float Weight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	float MaxDurability;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Tool/Weapon")
	EAstrawildHarvestType PreferredHarvestType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Tool/Weapon")
	float HarvestEfficiencyPower;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Tool/Weapon")
	float WeaponDamageBonus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visuals")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Visuals")
	TSoftClassPtr<AActor> WorldPickupClass;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("AstrawildItem"), *ItemTag.ToString());
	}
};