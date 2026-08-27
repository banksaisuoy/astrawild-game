#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildPlayerProgressionData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildPlayerStat : uint8
{
    MaxHealth,
    MaxStamina,
    AttackPower,
    WorkSpeed,
    WeightCapacity
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildPlayerPerkRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk")
    FGameplayTag PerkTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk", meta=(ClampMin="1"))
    int32 Tier = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk")
    TArray<FGameplayTag> PrerequisitePerkTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk|Stat")
    EAstrawildPlayerStat StatType = EAstrawildPlayerStat::MaxHealth;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk|Stat")
    float StatBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk|Modifier")
    float SprintStaminaMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk|Modifier")
    float FoodNutritionMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk|Modifier")
    float RepairRefundMultiplier = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk|Modifier")
    float CaptureOddsBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk|Modifier")
    float ReloadSpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Perk|Modifier")
    float CriticalDamageMultiplier = 1.0f;
};
