#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "AstrawildLaunchPad.generated.h"

UCLASS()
class ASTRAWILDCORE_API AAstrawildLaunchPad : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildLaunchPad();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Space Flight")
    FGameplayTag PadTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Space Flight")
    FGameplayTag DestinationBiomeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Space Flight", meta=(ClampMin="1.0"))
    float InteractionRadius = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Space Flight", meta=(ClampMin="0.1"))
    float LaunchDurationSeconds = 8.0f;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Space Flight")
    bool RequestLaunch(AActor* Pilot);
};
