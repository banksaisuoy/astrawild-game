#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildTypes.h"
#include "Data/AstrawildDungeonData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDungeonRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    FName DungeonId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    FGameplayTag RegionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    FGameplayTag RequiredKeyTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    bool bConsumeRequiredKey = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    FGameplayTag BossSpeciesTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    EAstrawildElement BossElement = EAstrawildElement::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon", meta=(ClampMin="1"))
    int32 RecommendedLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon", meta=(ClampMin="60.0"))
    float TimeLimitSeconds = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    bool bSupportsCoop = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    TArray<FGameplayTag> RewardItemTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon")
    TArray<int32> RewardQuantities;
};
