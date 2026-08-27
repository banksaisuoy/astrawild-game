#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/AstrawildPartnerGearData.h"
#include "AstrawildMountedWeaponBase.generated.h"

class UAudioComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMountedWeaponFiredSignature, AActor*, HitActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMountedWeaponFailedSignature, const FText&, FailureReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMountedPartnerSkillActivatedSignature);

UCLASS()
class ASTRAWILDCORE_API AAstrawildMountedWeaponBase : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildMountedWeaponBase();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Mounted Weapon")
    TObjectPtr<UAstrawildPartnerGearDataAsset> GearData;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Mounted Weapon")
    TObjectPtr<AActor> MountedOwner;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Mounted Weapon|Ammo")
    int32 CurrentAmmo = 0;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Mounted Weapon|Heat")
    float CurrentHeat = 0.0f;

    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Mounted Weapon|Heat")
    bool bOverheated = false;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Mounted Weapon|Events")
    FOnMountedWeaponFiredSignature OnWeaponFired;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Mounted Weapon|Events")
    FOnMountedWeaponFailedSignature OnWeaponFailed;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Mounted Weapon|Events")
    FOnMountedPartnerSkillActivatedSignature OnPartnerSkillActivated;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Mounted Weapon")
    bool EquipToMount(AActor* MountActor);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Mounted Weapon")
    void UnequipFromMount();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Mounted Weapon")
    bool Fire();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Mounted Weapon")
    bool ActivatePartnerSkill();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mounted Weapon")
    bool IsEquipped() const { return IsValid(MountedOwner); }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mounted Weapon")
    float GetHeatNormalized() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mounted Weapon")
    float GetAmmoNormalized() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Mounted Weapon")
    TObjectPtr<USceneComponent> WeaponRoot;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Mounted Weapon|Aim", meta=(ClampMin="0.0"))
    float AimPitchDegrees = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Mounted Weapon|Aim", meta=(ClampMin="-180.0", ClampMax="180.0"))
    float AimYawDegrees = 0.0f;

    UFUNCTION(BlueprintImplementableEvent, Category="ASTRAWILD|Mounted Weapon")
    void BP_OnPartnerSkillActivated();

private:
    UPROPERTY(Replicated)
    float FireCooldownRemaining = 0.0f;

    UPROPERTY(Replicated)
    float OverheatLockoutRemaining = 0.0f;

    bool HasAuthorityForWeapon() const;
    bool CanFire(FText& OutFailureReason) const;
    bool HasRequiredTechnology() const;
    FVector GetMuzzleLocation() const;
    FRotator GetAimRotation() const;
    void CoolHeat(float DeltaSeconds);
    void Fail(const FText& Reason);
};
