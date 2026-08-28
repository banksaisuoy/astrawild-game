#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AstrawildGuildData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildGuildBuffNode
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guild")
    FGameplayTag BuffTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guild")
    FGameplayTag RequiredBuffTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guild", meta=(ClampMin="1"))
    int32 MaxLevel = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guild", meta=(ClampMin="0.0"))
    float AttackMultiplierPerLevel = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guild", meta=(ClampMin="0.0"))
    float DefenseMultiplierPerLevel = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guild", meta=(ClampMin="0.0"))
    float GatheringMultiplierPerLevel = 0.0f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildGuildTerritory
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Guild")
    FGameplayTag GuildTag;

    UPROPERTY(BlueprintReadOnly, Category="Guild")
    FGameplayTag TotemTag;

    UPROPERTY(BlueprintReadOnly, Category="Guild")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Guild")
    float Radius = 1500.0f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildGuildArenaTeam
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Guild|Arena")
    FGameplayTag TeamTag;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> Members;

    UPROPERTY(BlueprintReadOnly, Category="Guild|Arena")
    int32 Score = 0;

    UPROPERTY(BlueprintReadOnly, Category="Guild|Arena")
    bool bEliminated = false;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildGuildArenaMatchState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Guild|Arena")
    bool bMatchActive = false;

    UPROPERTY(BlueprintReadOnly, Category="Guild|Arena")
    float MatchElapsedSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Guild|Arena")
    int32 WinningTeamIndex = INDEX_NONE;
};
