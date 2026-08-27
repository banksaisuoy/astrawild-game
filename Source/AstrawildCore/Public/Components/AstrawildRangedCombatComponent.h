#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AstrawildRangedWeaponData.h"
#include "AstrawildRangedCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRangedWeaponFiredSignature, FGameplayTag, WeaponTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRangedWeaponReloadedSignature, FGameplayTag, WeaponTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRangedWeaponFailedSignature, const FText&, FailureReason);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildRangedCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildRangedCombatComponent();

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Ranged")
    TObjectPtr<UDataTable> WeaponTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Ranged")
    FGameplayTag EquippedWeaponTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Ranged")
    int32 CurrentAmmo = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Ranged")
    bool bIsReloading = false;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Ranged|Events")
    FOnRangedWeaponFiredSignature OnWeaponFired;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Ranged|Events")
    FOnRangedWeaponReloadedSignature OnWeaponReloaded;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Ranged|Events")
    FOnRangedWeaponFailedSignature OnWeaponFailed;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Ranged")
    bool EquipWeapon(const FGameplayTag& WeaponTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Ranged")
    bool Fire();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Ranged")
    void Reload();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ranged")
    bool IsWeaponEquipped() const { return EquippedWeaponTag.IsValid(); }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ranged")
    int32 GetCurrentAmmo() const { return CurrentAmmo; }

private:
    float FireCooldownRemaining = 0.0f;
    float ReloadRemaining = 0.0f;
    const FAstrawildRangedWeaponRow* FindWeaponRow(const FGameplayTag& WeaponTag) const;
    bool HasTechnology(const FGameplayTag& TechnologyTag) const;
};
