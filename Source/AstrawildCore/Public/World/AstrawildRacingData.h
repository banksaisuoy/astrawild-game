#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "AstrawildRacingData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildRaceCheckpoint
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing")
    int32 CheckpointIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing", meta=(ClampMin="1.0"))
    float Radius = 400.0f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildRaceBoostPad
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing")
    FGameplayTag PadTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing", meta=(ClampMin="1.0"))
    float Radius = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing", meta=(ClampMin="1.0"))
    float SpeedMultiplier = 1.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing", meta=(ClampMin="0.1"))
    float DurationSeconds = 2.0f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildRaceTrackDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing")
    FGameplayTag TrackTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing", meta=(ClampMin="1"))
    int32 LapCount = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing")
    TArray<FAstrawildRaceCheckpoint> Checkpoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Racing")
    TArray<FAstrawildRaceBoostPad> BoostPads;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildRaceParticipantState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Racing")
    int32 NextCheckpointIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category="Racing")
    int32 CompletedLaps = 0;

    UPROPERTY(BlueprintReadOnly, Category="Racing")
    float FinishTimeSeconds = -1.0f;

    UPROPERTY(BlueprintReadOnly, Category="Racing")
    float BoostMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="Racing")
    float BoostRemainingSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Racing")
    bool bFinished = false;
};
