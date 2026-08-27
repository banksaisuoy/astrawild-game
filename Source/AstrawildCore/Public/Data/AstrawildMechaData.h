// Copyright Epic Games, Inc. / ASTRAWILD Team. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildTypes.h"
#include "AstrawildMechaData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildMechaClass : uint8
{
	LightStriker    UMETA(DisplayName = "Light High-Mobility Striker"),
	HeavyAssault    UMETA(DisplayName = "Heavy Armor Assault Frame"),
	AerialTactical  UMETA(DisplayName = "Aerial Long-Range Tactical Frame"),
	SiegeFortress   UMETA(DisplayName = "Super-Heavy Siege Fortress"),
	CyberBeast      UMETA(DisplayName = "Bio-Cybernetic Beast Fusion")
};

UENUM(BlueprintType)
enum class EAstrawildMechaHardpoint : uint8
{
	PrimaryRightHand,
	SecondaryLeftHand,
	ShoulderLeft,
	ShoulderRight,
	ChestCore,
	RearThruster
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildMechaWeaponRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	FGameplayTag WeaponTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	EAstrawildMechaHardpoint HardpointSlot = EAstrawildMechaHardpoint::PrimaryRightHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	float BaseDamage = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	float FireRate = 5.0f; // Rounds per second

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	float EnergyCostPerShot = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	float HeatGeneratedPerShot = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	float ProjectileSpeed = 12000.0f; // cm/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	bool bIsHomingMissile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Weapon")
	bool bIsContinuousBeam = false;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildMechaFrameRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	FGameplayTag FrameTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	FText FrameName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	EAstrawildMechaClass MechaClass = EAstrawildMechaClass::LightStriker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	float MaxEnergy = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	float EnergyRechargeRate = 120.0f; // per second

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	float MaxShieldHP = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	float ArmorDefense = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	float GroundRunSpeed = 1400.0f; // cm/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	float FlightCruiseSpeed = 2600.0f; // cm/s

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	float OverboostSpeed = 4200.0f; // cm/s supersonic dash

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mecha Frame")
	TArray<FGameplayTag> DefaultWeaponTags;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildCyberneticEvolutionRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cyber Evolution")
	FGameplayTag BaseEchoTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cyber Evolution")
	FGameplayTag ResultingMechaEchoTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cyber Evolution")
	FText CyberVariantName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cyber Evolution")
	int32 RequiredPlayerLevel = 35;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cyber Evolution")
	FGameplayTag RequiredAstraCoreItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cyber Evolution")
	int32 RequiredIngotsCount = 25;
};