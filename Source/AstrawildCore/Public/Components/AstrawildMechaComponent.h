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

	// Suit Up / Fusion Control
	UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
	bool EquipMechaFrame(const FAstrawildMechaFrameRow& FrameData);

	UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
	void EjectMechaFrame();

	UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
	bool IsInMechaMode() const { return bIsMechaActive; }

	// Flight & Thruster Controls
	UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
	void SetFlightActive(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
	void TriggerOverboost(bool bEnable);

	// Weapon Firing
	UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
	bool FireHardpointWeapon(EAstrawildMechaHardpoint Slot, FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "Astrawild|Mecha")
	void ActivateBeamSaberMelee();

	// State Queries
	UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
	float GetEnergyPercent() const { return MaxEnergy > 0.0f ? CurrentEnergy / MaxEnergy : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
	float GetHeatPercent() const { return CurrentHeat / 100.0f; }

	UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
	float GetShieldPercent() const { return MaxShield > 0.0f ? CurrentShield / MaxShield : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Astrawild|Mecha")
	bool IsOverheated() const { return bIsOverheated; }

	// Events
	UPROPERTY(BlueprintAssignable, Category = "Astrawild|Mecha|Events")
	FOnMechaEnergyChanged OnEnergyChanged;

	UPROPERTY(BlueprintAssignable, Category = "Astrawild|Mecha|Events")
	FOnMechaShieldChanged OnShieldChanged;

	UPROPERTY(BlueprintAssignable, Category = "Astrawild|Mecha|Events")
	FOnMechaFlightStateChanged OnFlightStateChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|State")
	bool bIsMechaActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|State")
	bool bIsFlying = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|State")
	bool bIsOverboosting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|State")
	bool bIsOverheated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
	float CurrentEnergy = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
	float MaxEnergy = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
	float EnergyRechargeRate = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
	float CurrentHeat = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
	float HeatCoolingRate = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
	float CurrentShield = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Astrawild|Mecha|Stats")
	float MaxShield = 2500.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Astrawild|Mecha|Config")
	FAstrawildMechaFrameRow ActiveFrameData;
};