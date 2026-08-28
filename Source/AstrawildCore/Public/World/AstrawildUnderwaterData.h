#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildUnderwaterData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildUnderwaterZoneRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater")
    FName ZoneId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater", meta=(ClampMin="0.0"))
    float MinDepthMeters = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater", meta=(ClampMin="0.0"))
    float MaxDepthMeters = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater", meta=(ClampMin="0.0"))
    float PressureDamagePerSecond = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater", meta=(ClampMin="0.0"))
    float OxygenDrainMultiplier = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater", meta=(ClampMin="0.0"))
    float BuoyancyMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater")
    FGameplayTag BiomeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater")
    TArray<FGameplayTag> HazardTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Underwater")
    TArray<FGameplayTag> SpawnSpeciesTags;
};
