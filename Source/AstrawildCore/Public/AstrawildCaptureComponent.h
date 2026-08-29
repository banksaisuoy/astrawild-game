#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildCaptureComponent.generated.h"

class AAstrawildEchoCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildCaptureResult, AAstrawildEchoCharacter*, Echo, bool, bSuccess);

UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCaptureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildCaptureComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Capture")
    FAstrawildCaptureResult OnCaptureResult;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Capture", meta=(ClampMin="0.0"))
    float CaptureCooldownSeconds = 1.0f;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Capture")
    bool TryCapture(AActor* Target, float InitialTrust = 0.0f);

    /** Capture chance for a target Echo (0..1) before any roll, for HUD/UI display. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Capture")
    float PreviewCaptureChance(const AAstrawildEchoCharacter* Target) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Capture")
    bool IsOnCooldown() const;

private:
    double LastCaptureTimeSeconds = -BIG_NUMBER;
};
