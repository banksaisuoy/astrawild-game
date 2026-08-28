#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildWeatherData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWeatherRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
    FGameplayTag WeatherTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather")
    int32 TemperatureModifier = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="0.0", ClampMax="1.0"))
    float VisibilityMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="0.0", ClampMax="1.0"))
    float WindStrength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="0.0", ClampMax="1.0"))
    float RainIntensity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="30.0"))
    float MinimumDurationSeconds = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weather", meta=(ClampMin="30.0"))
    float MaximumDurationSeconds = 420.0f;
};
