#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildPowerGridData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildPowerNodeType : uint8
{
    Generator,
    Battery,
    Consumer
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildPowerGridNodeRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid")
    FGameplayTag NodeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid")
    EAstrawildPowerNodeType NodeType = EAstrawildPowerNodeType::Consumer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid", meta=(ClampMin="0.0"))
    float GenerationWatts = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid", meta=(ClampMin="0.0"))
    float StorageCapacityWattHours = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid", meta=(ClampMin="0.0"))
    float ConsumptionWatts = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid", meta=(ClampMin="0.0"))
    float Priority = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid")
    FGameplayTag RequiredTechnologyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid")
    FGameplayTag RequiredBiomeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid")
    bool bGeneratesOnlyDuringDay = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid")
    bool bRequiresVolcanicVent = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid")
    bool bStopsFoodSpoilage = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Power Grid")
    float CraftSpeedMultiplier = 1.0f;
};
