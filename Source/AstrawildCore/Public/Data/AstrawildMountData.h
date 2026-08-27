#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Data/AstrawildMountData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildMountProfile : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mount")
    FName MountProfileId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mount")
    FName SaddleSocketName = TEXT("SaddleSocket");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mount", meta=(ClampMin="0.1"))
    float SpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mount", meta=(ClampMin="0.0"))
    float StaminaCostPerSecond = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mount", meta=(ClampMin="0.1"))
    float JumpMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mount")
    bool bAllowsCombatFromMount = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mount")
    FGameplayTag MountFamilyTag;
};
