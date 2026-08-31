#include "AstrawildCaptureComponent.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildVfxActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UAstrawildCaptureComponent::UAstrawildCaptureComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UAstrawildInventoryComponent* UAstrawildCaptureComponent::GetInventory() const
{
    AActor* Owner = GetOwner();
    return Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
}

bool UAstrawildCaptureComponent::IsOnCooldown() const
{
    const UWorld* World = GetWorld();
    return World && (World->GetTimeSeconds() - LastCaptureTimeSeconds) < CaptureCooldownSeconds;
}

bool UAstrawildCaptureComponent::IsTracking(const AAstrawildEchoCharacter* Target) const
{
    if (!IsValid(Target) || !GetOwner())
    {
        return false;
    }
    return FVector::Dist(GetOwner()->GetActorLocation(), Target->GetActorLocation()) <= TrackingDistance;
}

float UAstrawildCaptureComponent::PreviewCaptureChance(const AAstrawildEchoCharacter* Target) const
{
    if (!IsValid(Target))
    {
        return 0.0f;
    }

    float Chance = Target->ComputeCaptureChance();

    // Pipeline bonuses (directive §8): completed field-journal observation + active tracking.
    if (const UWorld* World = GetWorld())
    {
        if (const UAstrawildJournalSubsystem* Journal = World->GetSubsystem<UAstrawildJournalSubsystem>())
        {
            if (const FAstrawildJournalEntry* Entry = Journal->FindEntry(Target))
            {
                Chance += Entry->ObservationProgress / 100.0f * 0.15f;
            }
        }
    }
    if (IsTracking(Target))
    {
        Chance += 0.05f;
    }

    return FMath::Clamp(Chance, 0.0f, 0.95f);
}

bool UAstrawildCaptureComponent::TryCapture(AActor* Target, const float InitialTrust)
{
    if (GetOwnerRole() != ROLE_Authority || IsOnCooldown())
    {
        return false;
    }

    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(Target);
    if (!IsValid(Echo))
    {
        return false;
    }

    LastCaptureTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    // Consume a Resonator per attempt (directive §8).
    UAstrawildInventoryComponent* Inventory = GetInventory();
    if (!Inventory || !Inventory->HasItem(ResonatorItemId, 1) || !Inventory->RemoveItem(ResonatorItemId, 1))
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Capture attempt without a Resonator."));
        OnCaptureResult.Broadcast(Echo, false);
        return false;
    }

    // Capture reads the situation: weaken first, build trust, observe, then roll.
    const float CaptureChance = PreviewCaptureChance(Echo);
    OnCaptureAttempt.Broadcast(Echo, CaptureChance);

    // Spawn spinning resonance capture rings around the Echo
    const FLinearColor CaptureColor = Echo->EchoDefinition 
        ? FAstrawildVfxPalette::GetElementTint(Echo->EchoDefinition->Element) 
        : FLinearColor(0.2f, 0.8f, 1.0f);
    AAstrawildCaptureVfxActor::SpawnCaptureVfx(GetWorld(), Echo->GetActorLocation(), CaptureColor, 180.0f, 1.1f);

    const bool bRolledSuccess = CaptureChance > 0.0f && FMath::FRandRange(0.0f, 1.0f) <= CaptureChance;
    bool bSuccess = false;
    if (bRolledSuccess)
    {
        AActor* Owner = GetOwner();
        Echo->OwnerPlayerId = Owner ? Owner->GetFName() : NAME_None;
        bSuccess = Echo->Capture(InitialTrust);
    }

    OnCaptureResult.Broadcast(Echo, bSuccess);
    if (!bSuccess)
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("Capture failed for %s (chance %.2f)."), *Echo->GetName(), CaptureChance);
    }
    return bSuccess;
}
