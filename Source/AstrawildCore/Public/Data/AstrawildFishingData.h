#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildFishingData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildFishRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    FGameplayTag FishTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    FGameplayTag HabitatTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0.0"))
    float MinDepthMeters = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0.0"))
    float MaxDepthMeters = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    FGameplayTag BaitTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing")
    FGameplayTag CatchItemTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="1"))
    int32 SellPrice = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0.1"))
    float PullStrength = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0.1"))
    float RequiredReelSeconds = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0.0", ClampMax="100.0"))
    float SafeTensionMin = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0.0", ClampMax="100.0"))
    float SafeTensionMax = 85.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing", meta=(ClampMin="0.0"))
    float RarityWeight = 1.0f;
};
