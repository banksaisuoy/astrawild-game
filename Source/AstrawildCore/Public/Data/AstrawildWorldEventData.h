#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildWorldEventData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildWorldEventType : uint8
{
    Migration,
    Storm,
    ResourceBloom,
    Eclipse,
    TitanArrival,
    BaseRaid
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorldEventRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event")
    FGameplayTag EventTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event")
    EAstrawildWorldEventType EventType = EAstrawildWorldEventType::Migration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event")
    FGameplayTag BiomeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event")
    FGameplayTag WeatherTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event")
    TArray<FGameplayTag> SpawnRuleTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event")
    TArray<FGameplayTag> RewardItemTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event", meta=(ClampMin="30.0"))
    float DurationSeconds = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event", meta=(ClampMin="0.0"))
    float CooldownSeconds = 1800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event", meta=(ClampMin="0"))
    int32 MaxActiveSpecialSpawns = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event")
    bool bRequiresNight = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Event")
    bool bRequiresStorm = false;
};
