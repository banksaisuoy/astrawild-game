#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildTypes.h"
#include "AstrawildCraftingData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildCraftingRecipeRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe")
    FGameplayTag RecipeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe")
    TArray<FGameplayTag> IngredientTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe")
    TArray<int32> IngredientQuantities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe")
    FGameplayTag OutputItemTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe", meta=(ClampMin="1"))
    int32 OutputQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe", meta=(ClampMin="0.1"))
    float CraftTimeSeconds = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe")
    EAstrawildBuildingType RequiredStation = EAstrawildBuildingType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Recipe|Technology")
    FGameplayTag RequiredTechnologyTag;
};
