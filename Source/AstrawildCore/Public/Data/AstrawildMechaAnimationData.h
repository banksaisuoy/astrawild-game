#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildMechaAnimationData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildMechaAnimationProfileRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    FGameplayTag FrameTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    FSoftObjectPath AnimBlueprintPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    FSoftObjectPath GroundLocomotionPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    FSoftObjectPath FlightHoverPath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    FSoftObjectPath FlightCruisePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    FSoftObjectPath OverboostMontagePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    FSoftObjectPath PlasmaEdgeMontagePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    FSoftObjectPath HeavyCannonMontagePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    FSoftObjectPath ShutdownMontagePath;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mecha Animation")
    TArray<FName> RequiredSocketNames;
};
