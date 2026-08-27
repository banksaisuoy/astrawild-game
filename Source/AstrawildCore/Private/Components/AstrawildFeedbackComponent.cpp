#include "Components/AstrawildFeedbackComponent.h"

#include "Components/AstrawildCaptureComponent.h"
#include "Components/AstrawildCombatComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

UAstrawildFeedbackComponent::UAstrawildFeedbackComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAstrawildFeedbackComponent::BeginPlay()
{
    Super::BeginPlay();

    if (AActor* Owner = GetOwner())
    {
        if (UAstrawildCombatComponent* Combat = Owner->FindComponentByClass<UAstrawildCombatComponent>())
        {
            Combat->OnDamageDealt.AddDynamic(this, &UAstrawildFeedbackComponent::HandleDamageDealt);
            Combat->OnDamageReceived.AddDynamic(this, &UAstrawildFeedbackComponent::HandleDamageReceived);
            Combat->OnComboStep.AddDynamic(this, &UAstrawildFeedbackComponent::HandleComboStep);
        }

        if (UAstrawildCaptureComponent* Capture = Owner->FindComponentByClass<UAstrawildCaptureComponent>())
        {
            Capture->OnCaptureSuccess.AddDynamic(this, &UAstrawildFeedbackComponent::HandleCaptureSuccess);
            Capture->OnCaptureFailed.AddDynamic(this, &UAstrawildFeedbackComponent::HandleCaptureFailed);
            Capture->OnCaptureFeedback.AddDynamic(this, &UAstrawildFeedbackComponent::HandleCaptureFeedback);
        }
    }
}

void UAstrawildFeedbackComponent::HandleDamageDealt(AActor* Target, const float Damage, const EAstrawildElement Element, const bool bIsCrit, const int32 ComboStep)
{
    SpawnElementalVFX(Target, Element);
    PlaySoundAtActor(Target, bIsCrit ? CriticalDamageSound : DamageSound);
}

void UAstrawildFeedbackComponent::HandleDamageReceived(AActor* Instigator, const float Damage, const EAstrawildElement Element, const bool bIsCrit)
{
    SpawnElementalVFX(GetOwner(), Element);
    PlaySoundAtActor(GetOwner(), bIsCrit ? CriticalDamageSound : DamageSound);
}

void UAstrawildFeedbackComponent::HandleComboStep(const int32 ComboStep)
{
    if (ComboStep > 0)
    {
        PlaySoundAtActor(GetOwner(), ComboSound);
    }
}

void UAstrawildFeedbackComponent::HandleCaptureSuccess(AActor* TargetEcho, const FAstrawildCapturedEchoData& EchoData, const int32 PartySlot)
{
    if (TargetEcho && CaptureSuccessVFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CaptureSuccessVFX, TargetEcho->GetActorLocation());
    }
    PlaySoundAtActor(TargetEcho, CaptureSuccessSound);
}

void UAstrawildFeedbackComponent::HandleCaptureFailed(AActor* TargetEcho, const int32 ShakesCompleted, const FText& FailureReason)
{
    if (TargetEcho && CaptureFailVFX)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, CaptureFailVFX, TargetEcho->GetActorLocation());
    }
    PlaySoundAtActor(TargetEcho, CaptureFailSound);
}

void UAstrawildFeedbackComponent::HandleCaptureFeedback(const FText& FeedbackMessage, const bool bIsSuccess)
{
    if (!bIsSuccess && GetOwner())
    {
        PlaySoundAtActor(GetOwner(), CaptureFailSound);
    }
}

void UAstrawildFeedbackComponent::SpawnElementalVFX(AActor* Target, const EAstrawildElement Element) const
{
    if (!Target)
    {
        return;
    }

    UNiagaraSystem* Effect = nullptr;
    switch (Element)
    {
    case EAstrawildElement::Solar:
        Effect = SolarSparksVFX;
        break;
    case EAstrawildElement::Geo:
        Effect = GeoDustVFX;
        break;
    case EAstrawildElement::Torrent:
        Effect = TorrentSplashVFX;
        break;
    default:
        break;
    }

    if (Effect)
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Effect, Target->GetActorLocation());
    }
}

void UAstrawildFeedbackComponent::PlaySoundAtActor(AActor* Target, USoundBase* Sound) const
{
    if (Target && Sound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, Sound, Target->GetActorLocation());
    }
}
