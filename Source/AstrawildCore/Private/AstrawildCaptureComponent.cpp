#include "AstrawildCaptureComponent.h"

#include "AstrawildCore.h"
#include "AstrawildEchoCharacter.h"
#include "Engine/World.h"

UAstrawildCaptureComponent::UAstrawildCaptureComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UAstrawildCaptureComponent::IsOnCooldown() const
{
    const UWorld* World = GetWorld();
    return World && (World->GetTimeSeconds() - LastCaptureTimeSeconds) < CaptureCooldownSeconds;
}

bool UAstrawildCaptureComponent::PreviewCaptureChance(const AAstrawildEchoCharacter* Target) const
{
    return IsValid(Target) ? Target->ComputeCaptureChance() : 0.0f;
}

bool UAstrawildCaptureComponent::TryCapture(AActor* Target, const float InitialTrust)
{
    if (IsOnCooldown())
    {
        return false;
    }

    LastCaptureTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(Target);

    // Design rule: capture reads the situation. Weaken the Echo first or build trust;
    // a healthy, distrustful Echo almost always breaks free, a defeated one cannot be captured.
    const float CaptureChance = IsValid(Echo) ? Echo->ComputeCaptureChance() : 0.0f;
    const bool bRolledSuccess = CaptureChance > 0.0f && FMath::FRandRange(0.0f, 1.0f) <= CaptureChance;
    const bool bSuccess = bRolledSuccess && IsValid(Echo) && Echo->Capture(InitialTrust);
    OnCaptureResult.Broadcast(Echo, bSuccess);

    if (!bSuccess)
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("Capture failed for target %s (chance %.2f)."), IsValid(Target) ? *Target->GetName() : TEXT("None"), CaptureChance);
    }
    return bSuccess;
}
