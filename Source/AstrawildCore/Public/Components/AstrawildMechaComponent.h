// Copyright Epic Games, Inc. / ASTRAWILD Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Data/AstrawildMechaData.h"
#include "AstrawildMechaComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMechaEnergyChanged, float, CurrentEnergy, float, MaxEnergy, float, CurrentHeat);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMechaShieldChanged, float, CurrentShield, float, MaxShield);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMechaFlightStateChanged, bool, bIsFlying);

UCLASS(ClassGroup = (Astrawild), meta = (BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildMechaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildMechaComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
    bool EquipMechaFrame(const FAstrawildMechaFrameRow& FrameData);

    UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
    void EjectMechaFrame();

    UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
    bool IsInMechaMode() const { return bIsMechaActive; }

    UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
    void SetFlightActive(bool bActive);

    UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
    void TriggerOverboost(bool bEnable);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Frames")
    TObjectPtr<UDataTable> FrameTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Weapons")
    TObjectPtr<UDataTable> WeaponTable;

    UPROPERTY(ReplicatedUsing = OnRepMechaState, VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|Weapons")
    FGameplayTag EquippedWeaponTag;

    UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
    bool FireHardpointWeapon(EAstrawildMechaHardpoint Slot, FVector TargetLocation);

    UFUNCTION(Server, Reliable)
    void ServerEquipMechaFrame(FGameplayTag FrameTag);

    UFUNCTION(Server, Reliable)
    void ServerEjectMechaFrame();

    UFUNCTION(Server, Reliable)
    void ServerSetFlightActive(bool bActive);

    UFUNCTION(Server, Reliable)
    void ServerTriggerOverboost(bool bEnable);

    UFUNCTION(Server, Reliable)
    void ServerFireHardpointWeapon(EAstrawildMechaHardpoint Slot, FVector_NetQuantize TargetLocation);

    UFUNCTION(Server, Reliable)
    void ServerActivateBeamSaberMelee();

    UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
    void ActivateBeamSaberMelee();

    UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
    float GetEnergyPercent() const { return MaxEnergy > 0.0f ? FMath::Clamp(CurrentEnergy / MaxEnergy, 0.0f, 1.0f) : 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
    float GetHeatPercent() const { return FMath::Clamp(CurrentHeat / 100.0f, 0.0f, 1.0f); }

    UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
    float GetShieldPercent() const { return MaxShield > 0.0f ? FMath::Clamp(CurrentShield / MaxShield, 0.0f, 1.0f) : 0.0f; }

    UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
    bool IsOverheated() const { return bIsOverheated; }

    UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
    bool IsFlightActive() const { return bIsFlying; }

    UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
    bool IsOverboosting() const { return bIsOverboosting; }

    UPROPERTY(BlueprintAssignable, Category = "Astrawild|Mecha|Events")
    FOnMechaEnergyChanged OnEnergyChanged;

    UPROPERTY(BlueprintAssignable, Category = "Astrawild|Mecha|Events")
    FOnMechaShieldChanged OnShieldChanged;

    UPROPERTY(BlueprintAssignable, Category = "Astrawild|Mecha|Events")
    FOnMechaFlightStateChanged OnFlightStateChanged;

protected:
    virtual void BeginPlay() override;

    UPROPERTY(ReplicatedUsing = OnRepMechaState, VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|State")
    bool bIsMechaActive = false;

    UPROPERTY(ReplicatedUsing = OnRepMechaState, VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|State")
    bool bIsFlying = false;

    UPROPERTY(ReplicatedUsing = OnRepMechaState, VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|State")
    bool bIsOverboosting = false;

    UPROPERTY(ReplicatedUsing = OnRepMechaState, VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|State")
    bool bIsOverheated = false;

    UPROPERTY(ReplicatedUsing = OnRepEnergy, EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
    float CurrentEnergy = 1000.0f;

    UPROPERTY(ReplicatedUsing = OnRepEnergy, EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
    float MaxEnergy = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
    float EnergyRechargeRate = 120.0f;

    UPROPERTY(ReplicatedUsing = OnRepEnergy, EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
    float CurrentHeat = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
    float HeatCoolingRate = 25.0f;

    UPROPERTY(ReplicatedUsing = OnRepShield, EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
    float CurrentShield = 2500.0f;

    UPROPERTY(ReplicatedUsing = OnRepShield, EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
    float MaxShield = 2500.0f;

    UPROPERTY(ReplicatedUsing = OnRepMechaState, VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|Config")
    FAstrawildMechaFrameRow ActiveFrameData;

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|Weapons", meta=(AllowPrivateAccess="true"))
    float HardpointCooldownRemaining = 0.0f;

    UFUNCTION()
    void OnRepMechaState();

    UFUNCTION()
    void OnRepEnergy();

    UFUNCTION()
    void OnRepShield();

    const FAstrawildMechaFrameRow* FindFrameByTag(const FGameplayTag& FrameTag) const;
    const FAstrawildMechaWeaponRow* FindWeaponForSlot(EAstrawildMechaHardpoint Slot) const;
    bool HasAuthorityForMecha() const;
};
