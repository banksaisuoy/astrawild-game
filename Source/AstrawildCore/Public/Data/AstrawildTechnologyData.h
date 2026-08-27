#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Data/AstrawildTechnologyData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildTechnologyNodeRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Technology")
    FGameplayTag TechnologyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Technology")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Technology")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Technology", meta=(ClampMin="0"))
    int32 Tier = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Technology")
    TArray<FGameplayTag> PrerequisiteTechnologyTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Technology")
    TArray<FGameplayTag> UnlockRecipeTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Technology", meta=(ClampMin="0"))
    int32 ResearchCost = 1;
};
