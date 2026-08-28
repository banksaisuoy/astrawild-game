#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildTypes.h"
#include "AstrawildRangedWeaponData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildRangedWeaponType : uint8
{
    Bow UMETA(DisplayName="Bow"),
    Repeater UMETA(DisplayName="Repeater"),
    Beam UMETA(DisplayName="Beam")
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildRangedWeaponRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    FGameplayTag WeaponTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    EAstrawildRangedWeaponType WeaponType = EAstrawildRangedWeaponType::Bow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    EAstrawildElement DamageElement = EAstrawildElement::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    FGameplayTag AmmoTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin="1.0"))
    float BaseDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin="100.0"))
    float RangeCentimeters = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin="1.0"))
    float FireIntervalSeconds = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin="1"))
    int32 MagazineSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon", meta=(ClampMin="0.1"))
    float ReloadDurationSeconds = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon")
    bool bUseHitscan = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Weapon|Technology")
    FGameplayTag RequiredTechnologyTag;
};
