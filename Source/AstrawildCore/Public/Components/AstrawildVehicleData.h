#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildVehicleData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildVehicleType : uint8
{
    HoverbikeStriker,
    SandSkiffDuneRider,
    MonowheelGyroStriker,
    RoverAstraExplorer,
    SiegeTankTitanCrawler,
    MobileBaseMammothHauler,
    JetSkiHydroGlider,
    MiniSubNautilus,
    BattleCruiserAstraFrigate,
    GyrocopterZephyr,
    VTOLExplorerSkyhawk,
    AirshipSkyGalleon
};

UENUM(BlueprintType)
enum class EAstrawildVehicleSlot : uint8
{
    Engine,
    Armor,
    Weapon,
    Utility,
    Passenger
};

UENUM(BlueprintType)
enum class EAstrawildVehicleCategory : uint8
{
    LandHover,
    ArmoredRover,
    MarineSubmersible,
    AtmosphericAircraft,
    OrbitalSpacecraft
};


USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildVehicleRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    FGameplayTag VehicleTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    EAstrawildVehicleCategory Category = EAstrawildVehicleCategory::LandHover;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    EAstrawildVehicleType VehicleType = EAstrawildVehicleType::HoverbikeStriker;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle", meta=(ClampMin="1.0"))
    float MaxSpeedCentimetersPerSecond = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle", meta=(ClampMin="0.0"))
    float BoostSpeedCentimetersPerSecond = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle", meta=(ClampMin="0.0"))
    float MaxDepthMeters = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle", meta=(ClampMin="0.0"))
    float CargoCapacityKilograms = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle", meta=(ClampMin="1"))
    int32 PassengerSeatCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle", meta=(ClampMin="0.0"))
    float Handling = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    FGameplayTag FuelTypeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    FGameplayTagContainer WeaponTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    FGameplayTagContainer UtilityTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    bool bCanHover = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    bool bCanSubmerge = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    bool bCanFly = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle")
    bool bIsMobileBase = false;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildVehiclePartRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part")
    FGameplayTag PartTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part")
    EAstrawildVehicleSlot Slot = EAstrawildVehicleSlot::Utility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part")
    FGameplayTagContainer CompatibleVehicleTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part", meta=(ClampMin="0.01"))
    float SpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part", meta=(ClampMin="0.0"))
    float FuelConsumptionMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part", meta=(ClampMin="0.0"))
    float ArmorBonus = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part", meta=(ClampMin="0.0"))
    float BatteryCapacity = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part", meta=(ClampMin="0.0"))
    float BoostMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part", meta=(ClampMin="0.0"))
    float WeaponPower = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Part")
    FGameplayTag UtilityEffectTag;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildVehicleInstalledPart
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    EAstrawildVehicleSlot Slot = EAstrawildVehicleSlot::Utility;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    FGameplayTag PartTag;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    float SpeedMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    float BoostMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildVehicleControlInput
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    float Throttle = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    float Steer = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    float Brake = 0.0f;

    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    bool bHandbrake = false;

    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    bool bBoost = false;

    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    bool bFirePrimary = false;

    UPROPERTY(BlueprintReadWrite, Category="Vehicle")
    bool bFireSecondary = false;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildVehicleRuntimeState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    float CurrentSpeedCentimetersPerSecond = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    float FuelNormalized = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    float DurabilityNormalized = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    float BoostRemainingSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    bool bEngineActive = false;

    UPROPERTY(BlueprintReadOnly, Category="Vehicle")
    AActor* Driver = nullptr;
};
