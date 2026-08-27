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

bool UAstrawildCaptureComponent::TryCapture(AActor* Target, const float InitialTrust)
{
    if (IsOnCooldown())
    {
        return false;
    }

    LastCaptureTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(Target);
    const bool bSuccess = IsValid(Echo) && Echo->Capture(InitialTrust);
    OnCaptureResult.Broadcast(Echo, bSuccess);

    if (!bSuccess)
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("Capture failed for target %s."), IsValid(Target) ? *Target->GetName() : TEXT("None"));
    }
    return bSuccess;
}
