#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildCampaignData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildCampaignChapterRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    FName ChapterId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    FText Summary;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    TArray<FName> RequiredQuestIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    TArray<FName> OptionalQuestIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    FGameplayTag RequiredBossEncounterTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    FGameplayTag UnlockRegionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    FGameplayTag UnlockSpireTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    TArray<FGameplayTag> EndingChoiceTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Campaign")
    bool bIsFinalChapter = false;
};
