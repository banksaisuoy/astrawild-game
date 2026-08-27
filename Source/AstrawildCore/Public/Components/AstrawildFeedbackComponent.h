#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildFeedbackComponent.generated.h"

class UAstrawildCombatComponent;
class UAstrawildCaptureComponent;
class UNiagaraSystem;
class USoundBase;
struct FAstrawildCapturedEchoData;

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildFeedbackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildFeedbackComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|VFX")
    TObjectPtr<UNiagaraSystem> SolarSparksVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|VFX")
    TObjectPtr<UNiagaraSystem> GeoDustVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|VFX")
    TObjectPtr<UNiagaraSystem> TorrentSplashVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|VFX")
    TObjectPtr<UNiagaraSystem> CaptureSuccessVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|VFX")
    TObjectPtr<UNiagaraSystem> CaptureFailVFX;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|Audio")
    TObjectPtr<USoundBase> DamageSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|Audio")
    TObjectPtr<USoundBase> CriticalDamageSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|Audio")
    TObjectPtr<USoundBase> CaptureSuccessSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|Audio")
    TObjectPtr<USoundBase> CaptureFailSound;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Feedback|Audio")
    TObjectPtr<USoundBase> ComboSound;

protected:
    virtual void BeginPlay() override;

private:
    UFUNCTION()
    void HandleDamageDealt(AActor* Target, float Damage, EAstrawildElement Element, bool bIsCrit, int32 ComboStep);

    UFUNCTION()
    void HandleDamageReceived(AActor* Instigator, float Damage, EAstrawildElement Element, bool bIsCrit);

    UFUNCTION()
    void HandleComboStep(int32 ComboStep);

    UFUNCTION()
    void HandleCaptureSuccess(AActor* TargetEcho, const FAstrawildCapturedEchoData& EchoData, int32 PartySlot);

    UFUNCTION()
    void HandleCaptureFailed(AActor* TargetEcho, int32 ShakesCompleted, const FText& FailureReason);

    UFUNCTION()
    void HandleCaptureFeedback(const FText& FeedbackMessage, bool bIsSuccess);

    void SpawnElementalVFX(AActor* Target, EAstrawildElement Element) const;
    void PlaySoundAtActor(AActor* Target, USoundBase* Sound) const;
};
