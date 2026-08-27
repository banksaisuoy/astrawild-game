#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Data/AstrawildEchoDataAsset.h"
#include "Data/AstrawildEvolutionData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEvolutionRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evolution")
    FName EvolutionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evolution")
    FGameplayTag SourceSpeciesTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evolution")
    FGameplayTag TargetSpeciesTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evolution")
    TSoftObjectPtr<UAstrawildEchoDataAsset> TargetSpeciesData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evolution", meta=(ClampMin="1"))
    int32 RequiredLevel = 20;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evolution")
    FGameplayTag RequiredItemTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Evolution", meta=(ClampMin="0"))
    int32 RequiredItemQuantity = 0;
};
