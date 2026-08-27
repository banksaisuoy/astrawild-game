#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildMechaVFXData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildMechaVFXBindingRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha VFX")
    FGameplayTag EffectTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha VFX")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha VFX")
    FSoftObjectPath NiagaraSystemPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha VFX")
    FName AttachSocket = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha VFX")
    FName StartParameter = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha VFX")
    FName EndParameter = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha VFX")
    FName IntensityParameter = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha VFX")
    bool bLooping = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha VFX")
    bool bFallbackToEmitter = true;
};
