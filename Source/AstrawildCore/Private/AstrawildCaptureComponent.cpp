#include "AstrawildCaptureComponent.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerController.h"
#include "AstrawildVfxActor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

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

    // Final-audit L-9: validate capturability BEFORE consuming the Resonator and
    // BEFORE stamping the cooldown — a dead/already-captured target used to burn
    // the resonator and start the cooldown for a guaranteed-failure attempt.
    if (Echo->IsDefeated() || Echo->bCaptured)
    {
        OnCaptureResult.Broadcast(Echo, false);
        return false;
    }

    UAstrawildInventoryComponent* Inventory = GetInventory();
    if (!Inventory || !Inventory->HasItem(ResonatorItemId, 1))
    {
        // Final-audit F-20: no Resonator → report and leave the cooldown unstamped
        // (the failed check itself must not lock the player out of a later attempt).
        UE_LOG(LogAstrawild, Warning, TEXT("Capture attempt without a Resonator."));
        OnCaptureResult.Broadcast(Echo, false);
        return false;
    }

    // The roll can succeed — stamp the cooldown and consume the Resonator now.
    LastCaptureTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (!Inventory->RemoveItem(ResonatorItemId, 1))
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Capture: Resonator removal failed after the gate."));
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

    // Final Run (FR-11): capture feedback — HUD toast on both outcomes plus the
    // success stinger (A_Echo_Capture_Success) at the capture site. The audio is
    // a soft load: absent asset = silent pass-through (zero-asset rule, CP-00).
    if (AAstrawildPlayerController* PC = GetOwnerPlayerController())
    {
        if (bSuccess)
        {
            const FText SpeciesName = Echo->EchoDefinition ? Echo->EchoDefinition->DisplayName : FText::FromString(TEXT("Echo"));
            PC->Notify(FText::FromString(FString::Printf(TEXT("Echo captured: %s"), *SpeciesName.ToString())));
        }
        else
        {
            PC->Notify(FText::FromString(TEXT("The Echo broke free — weaken it, feed it, or observe longer.")));
        }
    }
    if (bSuccess)
    {
        if (USoundBase* CaptureStinger = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/A_Echo_Capture_Success")))
        {
            UGameplayStatics::PlaySoundAtLocation(GetWorld(), CaptureStinger, Echo->GetActorLocation());
        }
    }

    if (!bSuccess)
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("Capture failed for %s (chance %.2f)."), *Echo->GetName(), CaptureChance);
    }
    return bSuccess;
}

AAstrawildPlayerController* UAstrawildCaptureComponent::GetOwnerPlayerController() const
{
    const APawn* Pawn = Cast<APawn>(GetOwner());
    return Pawn ? Cast<AAstrawildPlayerController>(Pawn->GetController()) : nullptr;
}
