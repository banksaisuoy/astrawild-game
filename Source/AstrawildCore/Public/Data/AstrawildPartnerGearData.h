#pragma once

#include "CoreMinimal.h"
#include "Engine/PrimaryDataAsset.h"
#include "GameplayTagContainer.h"
#include "AstrawildTypes.h"
#include "AstrawildPartnerGearData.generated.h"

class AAstrawildMountedWeaponBase;

UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildPartnerGearDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Identity")
    FGameplayTag GearTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Identity")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Identity")
    FGameplayTag RequiredTechnologyTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Identity")
    FGameplayTag RequiredPartnerSkillTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Mount")
    FName MountProfileId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Mount")
    FName MuzzleSocketName = TEXT("MuzzleSocket");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Combat", meta=(ClampMin="0.1"))
    float DamagePerShot = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Combat")
    EAstrawildElement DamageElement = EAstrawildElement::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Combat", meta=(ClampMin="1.0"))
    float MaxRange = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Combat", meta=(ClampMin="0.01"))
    float FireIntervalSeconds = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Ammo", meta=(ClampMin="1"))
    int32 MagazineSize = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Ammo", meta=(ClampMin="1"))
    int32 AmmoPerShot = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Heat", meta=(ClampMin="1.0"))
    float MaxHeat = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Heat", meta=(ClampMin="0.0"))
    float HeatPerShot = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Heat", meta=(ClampMin="0.0"))
    float CoolRatePerSecond = 24.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Heat", meta=(ClampMin="0.0"))
    float OverheatLockoutSeconds = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Input")
    FName AimActionId = TEXT("MountAim");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Input")
    FName FireActionId = TEXT("MountFire");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Input")
    FName PartnerSkillActionId = TEXT("MountPartnerSkill");

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Partner Gear|Presentation")
    TSoftClassPtr<AAstrawildMountedWeaponBase> WeaponActorClass;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(TEXT("AstrawildPartnerGear"), GearTag.GetTagName());
    }
};
