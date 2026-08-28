#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AstrawildDisasterData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildDisasterType : uint8
{
    MeteorShower,
    Tornado,
    VolcanicAsh,
    Aurora
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDisasterDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Disaster")
    FGameplayTag DisasterTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Disaster")
    EAstrawildDisasterType DisasterType = EAstrawildDisasterType::MeteorShower;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Disaster")
    FGameplayTag BiomeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Disaster", meta=(ClampMin="0.1"))
    float DurationSeconds = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Disaster", meta=(ClampMin="0.0"))
    float CooldownSeconds = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Disaster", meta=(ClampMin="0.0", ClampMax="1.0"))
    float Intensity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Disaster", meta=(ClampMin="1.0"))
    float EffectRadius = 2500.0f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDisasterState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Disaster")
    FGameplayTag DisasterTag;

    UPROPERTY(BlueprintReadOnly, Category="Disaster")
    EAstrawildDisasterType DisasterType = EAstrawildDisasterType::MeteorShower;

    UPROPERTY(BlueprintReadOnly, Category="Disaster")
    float RemainingSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Disaster")
    float Intensity = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Disaster")
    bool bActive = false;
};
