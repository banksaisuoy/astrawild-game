#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildWorldKaijuBossData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorldKaijuBossRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju")
    FGameplayTag BossTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju")
    FGameplayTag BiomeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju", meta=(ClampMin="1"))
    int32 RecommendedLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju", meta=(ClampMin="1.0"))
    float MaxHealth = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju", meta=(ClampMin="1"))
    int32 PhaseCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju")
    FGameplayTag DisasterAffinityTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju")
    FGameplayTag RequiredArenaTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju")
    FGameplayTagContainer RewardItemTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju", meta=(ClampMin="1.0"))
    float EncounterRadius = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World Kaiju")
    bool bRequiresWorldEvent = true;
};
