#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AstrawildSpaceFlightData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildFlightState : uint8
{
    Docked,
    Launching,
    InOrbit,
    Returning,
    VacuumEmergency,
    Crashed
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildLaunchPadDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Space Flight")
    FGameplayTag PadTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Space Flight")
    FGameplayTag DestinationBiomeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Space Flight")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Space Flight", meta=(ClampMin="1.0"))
    float InteractionRadius = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Space Flight", meta=(ClampMin="0.1"))
    float LaunchDurationSeconds = 8.0f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildSpaceFlightState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Space Flight")
    EAstrawildFlightState FlightState = EAstrawildFlightState::Docked;

    UPROPERTY(BlueprintReadOnly, Category="Space Flight")
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Space Flight")
    float CabinPressureKPa = 101.325f;

    UPROPERTY(BlueprintReadOnly, Category="Space Flight")
    float LaunchProgressNormalized = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Space Flight")
    float LowGravityScale = 0.16f;

    UPROPERTY(BlueprintReadOnly, Category="Space Flight")
    bool bVacuumEmergency = false;
};
