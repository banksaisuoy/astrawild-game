#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildCaptureComponent.generated.h"

class AAstrawildEchoCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildCaptureResult, AAstrawildEchoCharacter*, Echo, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildCaptureAttempt, AAstrawildEchoCharacter*, Echo, float, CaptureChance);

/**
 * Multi-step capture pipeline (directive §8): Observe (journal) -> Track (proximity) ->
 * Weaken (combat) / Feed (trust) -> Capture roll. Requires and consumes an Echo Resonator.
 * Server-authoritative.
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCaptureComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildCaptureComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Capture")
    FAstrawildCaptureResult OnCaptureResult;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Capture")
    FAstrawildCaptureAttempt OnCaptureAttempt;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Capture", meta=(ClampMin="0.0"))
    float CaptureCooldownSeconds = 1.0f;

    /** Item consumed per capture attempt (directive §8 capture device). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Capture")
    FName ResonatorItemId = TEXT("Item_Resonator");

    /** Distance within which the player is "tracking" the target (capture bonus). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Capture", meta=(ClampMin="100.0"))
    float TrackingDistance = 900.0f;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Capture")
    bool TryCapture(AActor* Target, float InitialTrust = 0.0f);

    /** Capture chance for a target Echo (0..1) before any roll, for HUD/UI display. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Capture")
    float PreviewCaptureChance(const AAstrawildEchoCharacter* Target) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Capture")
    bool IsOnCooldown() const;

    /** Is the target within tracking range (pipeline step 2)? */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Capture")
    bool IsTracking(const AAstrawildEchoCharacter* Target) const;

private:
    double LastCaptureTimeSeconds = -BIG_NUMBER;

    class UAstrawildInventoryComponent* GetInventory() const;
};
