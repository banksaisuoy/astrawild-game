// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AstrawildTypes.h"
#include "AstrawildRecipeDataAsset.generated.h"

UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildRecipeDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recipe")
	FAstrawildRecipe Recipe;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("AstrawildRecipe"), *Recipe.RecipeTag.ToString());
	}
};