#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildTypes.h"
#include "AstrawildCookingData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildCookingRecipeRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking")
    FGameplayTag RecipeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking")
    TArray<FGameplayTag> IngredientTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking")
    TArray<int32> IngredientQuantities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking")
    FGameplayTag OutputItemTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking", meta=(ClampMin="1"))
    int32 OutputQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking", meta=(ClampMin="0.0"))
    float HungerRestored = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking", meta=(ClampMin="0.0"))
    float ThirstRestored = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking", meta=(ClampMin="0.0"))
    float NutritionValue = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking", meta=(ClampMin="1.0"))
    float SpoilageDurationSeconds = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking", meta=(ClampMin="0.0", ClampMax="1.0"))
    float RefrigeratedSpoilageRate = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking")
    FGameplayTag BuffTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking")
    float BuffMagnitude = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking", meta=(ClampMin="0.0"))
    float BuffDurationSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cooking")
    EAstrawildBuildingType RequiredStation = EAstrawildBuildingType::Campfire;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildActiveFoodBuff
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food Buff")
    FGameplayTag BuffTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food Buff")
    float Magnitude = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food Buff", meta=(ClampMin="0.0"))
    float RemainingDurationSeconds = 0.0f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildTrackedFoodState
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food")
    FGuid ItemInstanceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food")
    FGameplayTag FoodItemTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food", meta=(ClampMin="1"))
    int32 RemainingQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food", meta=(ClampMin="0.0"))
    float RemainingFreshnessSeconds = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Food")
    bool bRefrigerated = false;
};
