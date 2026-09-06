#include "AstrawildEndingCinematicComponent.h"

#include "AstrawildCore.h"
#include "AstrawildEndingLetterboxWidget.h"
#include "AstrawildGameState.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"

namespace
{
    // Ending title cards (mirrors GameState::GetEndingBannerText verdicts —
    // presentation copies, never read from the state: the cinematic renders,
    // it does not arbitrate).
    const TCHAR* EndingTitleA = TEXT("THE DAWN THAT STAYS");
    const TCHAR* EndingSubA = TEXT("The storm crown is broken. The Vale wakes under an honest sky.");
    const TCHAR* EndingTitleB = TEXT("THE STORM THAT SLEEPS");
    const TCHAR* EndingSubB = TEXT("The crown sleeps beneath the waves. The Vale endures — guarded, watchful, free.");
}

UAstrawildEndingCinematicComponent::UAstrawildEndingCinematicComponent()
{
    PrimaryComponentTick.bStartWithTickEnabled = false; // The ending switches it on.
    PrimaryComponentTick.TickInterval = 0.0f;           // Smooth blend-phase drives.
}

void UAstrawildEndingCinematicComponent::BeginPlay()
{
    Super::BeginPlay();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Host/server: instant response to the verdict broadcast.
    if (AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
    {
        GameState->OnEndingTriggered.AddDynamic(this, &UAstrawildEndingCinematicComponent::HandleEndingTriggered);
    }

    // All machines (host included): the replicated-verdict poll covers remote
    // clients (OnRep-style) and any path the broadcast might miss. 0.5s
    // cadence with a one-bool early-out is negligible.
    World->GetTimerManager().SetTimer(PollHandle, this, &UAstrawildEndingCinematicComponent::PollEndingState, 0.5f, true);
}

AAstrawildGameState* UAstrawildEndingCinematicComponent::GetGameState() const
{
    return GetWorld() ? GetWorld()->GetGameState<AAstrawildGameState>() : nullptr;
}

UAstrawildPlayerController* UAstrawildEndingCinematicComponent::GetAstrawildController() const
{
    return Cast<UAstrawildPlayerController>(GetOwner());
}

void UAstrawildEndingCinematicComponent::HandleEndingTriggered(EAstrawildEndingState Ending, EAstrawildEndingState OldEnding)
{
    StartCinematic(Ending);
}

void UAstrawildEndingCinematicComponent::PollEndingState()
{
    if (bCinematicPlayed)
    {
        return;
    }
    if (const AAstrawildGameState* GameState = GetGameState())
    {
        if (GameState->EndingState != EAstrawildEndingState::None)
        {
            StartCinematic(GameState->EndingState);
        }
    }
}

void UAstrawildEndingCinematicComponent::StartCinematic(const EAstrawildEndingState Ending)
{
    // One presentation per ending per session (the poll + broadcast + any
    // future OnRep all funnel here; the guard keeps them idempotent).
    if (bCinematicPlayed || bRunning || Ending == EAstrawildEndingState::None || Ending == EAstrawildEndingState::Count)
    {
        return;
    }

    // Only the LOCAL player watches a cinematic — server-side copies of
    // remote controllers skip (their own machine runs its own presentation).
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!PC || !PC->IsLocalController())
    {
        bCinematicPlayed = true; // Still mark: no double-start from later polls.
        return;
    }

    UWorld* World = GetWorld();
    APawn* Pawn = PC->GetPawn();
    if (!World || !Pawn)
    {
        bCinematicPlayed = true;
        return;
    }

    bCinematicPlayed = true;
    bRunning = true;
    StagedEnding = Ending;
    SequenceTime = 0.0f;
    bShotAEntered = bShotBEntered = bShotCEntered = false;

    // --- Overlay (pure-C++ UMG, above the HUD; the persistent banner survives) ---
    Overlay = NewObject<UAstrawildEndingLetterboxWidget>(PC, UAstrawildEndingLetterboxWidget::StaticClass());
    Overlay->AddToViewport(50); // Above the HUD (Z 0) — the ending owns the frame.
    Overlay->OnSkipRequested.AddDynamic(this, &UAstrawildEndingCinematicComponent::HandleSkipRequested);
    const bool bEndingA = Ending == EAstrawildEndingState::TheDawnThatStays;
    Overlay->SetEndingTexts(
        FText::FromString(bEndingA ? EndingTitleA : EndingTitleB),
        FText::FromString(bEndingA ? EndingSubA : EndingSubB));
    Overlay->SetBarsProgress(0.0f);
    Overlay->SetFadeAlpha(0.0f);
    Overlay->SetHintVisible(false);

    // --- Transient viewpoints: three staged cuts around the pawn ---
    ShotCameras.Reset();
    const FVector PawnLoc = Pawn->GetActorLocation();
    const FVector Forward = Pawn->GetActorForwardVector();
    const FVector Right = Pawn->GetActorRightVector();
    const FVector Up = FVector::UpVector;

    const FVector ShotLocs[3] = {
        PawnLoc + Forward * 260.0f + Up * 42.0f,       // A: low hero, looking up.
        PawnLoc + Right * 540.0f + Forward * 130.0f + Up * 440.0f, // B: high wide.
        PawnLoc - Forward * 380.0f + Up * 170.0f,      // C: return behind.
    };
    const FVector ShotFocus[3] = {
        PawnLoc + Up * 95.0f,
        PawnLoc + Up * 40.0f,
        PawnLoc + Up * 70.0f,
    };

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.ObjectFlags |= RF_Transient;
    for (int32 i = 0; i < 3; ++i)
    {
        const FRotator LookAt = FRotationMatrix::MakeFromX(ShotFocus[i] - ShotLocs[i]).Rotator();
        ACameraActor* ShotCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), ShotLocs[i], LookAt, Params);
        if (ShotCamera)
        {
            ShotCamera->SetActorLabel(FString::Printf(TEXT("AW_EndingShot_%d"), i));
            if (UCameraComponent* Cam = ShotCamera->GetCameraComponent())
            {
                Cam->SetFieldOfView(64.0f);
            }
            ShotCameras.Add(ShotCamera);
        }
    }

    // --- Input lock (the dialogue discipline: UIOnly swallows gameplay input;
    //     no cursor — this is a film, not a menu) ---
    PC->SetInputMode(FInputModeUIOnly());
    PC->bShowMouseCursor = false;

    SetComponentTickEnabled(true);
    UE_LOG(LogAstrawild, Log, TEXT("Ending cinematic staged (ending %d): 3 shots, %.1fs, skippable."),
        static_cast<int32>(Ending), SequenceEnd);
}

void UAstrawildEndingCinematicComponent::EnterShot(const int32 ShotIndex, const float BlendTimeSeconds)
{
    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (PC && ShotCameras.IsValidIndex(ShotIndex) && ShotCameras[ShotIndex])
    {
        PC->SetViewTargetWithBlend(ShotCameras[ShotIndex], BlendTimeSeconds, EViewTargetBlendFunction::VTBlend_Cubic);
    }
}

void UAstrawildEndingCinematicComponent::DriveSequence(const float DeltaTime)
{
    if (!bRunning)
    {
        return;
    }
    SequenceTime += DeltaTime;

    // Smoothstep helper for the staged ramps.
    const auto Ramp = [](const float Now, const float Start, const float Duration) -> float
    {
        if (Now <= Start)
        {
            return 0.0f;
        }
        const float Alpha = FMath::Clamp((Now - Start) / Duration, 0.0f, 1.0f);
        return Alpha * Alpha * (3.0f - 2.0f * Alpha);
    };

    if (Overlay)
    {
        // Bars: in over the first 1.2s.
        Overlay->SetBarsProgress(Ramp(SequenceTime, 0.0f, BarsInEnd));
        // Fade: brief dim-in (0→0.22 by 1.2s), clear by 2.6s, final to-black at the end.
        float Fade = 0.22f * Ramp(SequenceTime, 0.0f, BarsInEnd);
        Fade *= (1.0f - Ramp(SequenceTime, ShotAStart, 1.4f));
        Fade += Ramp(SequenceTime, FadeOutStart, SequenceEnd - FadeOutStart);
        Overlay->SetFadeAlpha(Fade);
        // Title/subtitle cards + skip hint.
        Overlay->SetTitleAlpha(Ramp(SequenceTime, TitleInStart, 1.6f));
        Overlay->SetSubtitleAlpha(Ramp(SequenceTime, SubtitleInStart, 1.6f));
        Overlay->SetHintVisible(SequenceTime >= HintInStart);
    }

    // Camera cuts (once each — the blend takes ShotBlendTime).
    if (!bShotAEntered && SequenceTime >= ShotAStart)
    {
        bShotAEntered = true;
        EnterShot(0, ShotBlendTime);
    }
    if (!bShotBEntered && SequenceTime >= ShotBStart)
    {
        bShotBEntered = true;
        EnterShot(1, ShotBlendTime);
    }
    if (!bShotCEntered && SequenceTime >= ShotCStart)
    {
        bShotCEntered = true;
        EnterShot(2, ShotBlendTime);
    }

    if (SequenceTime >= SequenceEnd)
    {
        FinishCinematic();
    }
}

void UAstrawildEndingCinematicComponent::HandleSkipRequested()
{
    // Fold to the fade-out phase — the SAME exit path, just sooner.
    if (bRunning)
    {
        SequenceTime = FMath::Max(SequenceTime, FadeOutStart);
        // Make sure the shot ordering flags agree with the jumped timeline.
        bShotAEntered = bShotBEntered = bShotCEntered = true;
        if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                PC->SetViewTargetWithBlend(Pawn, 0.6f, EViewTargetBlendFunction::VTBlend_Cubic);
            }
        }
        UE_LOG(LogAstrawild, Log, TEXT("Ending cinematic skipped — folding to the fade-out exit."));
    }
}

void UAstrawildEndingCinematicComponent::FinishCinematic()
{
    if (!bRunning)
    {
        return;
    }
    bRunning = false;
    SetComponentTickEnabled(false);

    APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (PC)
    {
        // View back to the pawn — the player re-enters the world they earned.
        if (APawn* Pawn = PC->GetPawn())
        {
            PC->SetViewTargetWithBlend(Pawn, 0.8f, EViewTargetBlendFunction::VTBlend_Cubic);
        }
        // Input back to the game (the standard restore discipline).
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }

    // Overlay leaves WITH its final fade — the persistent HUD ending banner
    // (already live) takes over the verdict display from here.
    if (Overlay)
    {
        Overlay->RemoveFromParent();
        Overlay = nullptr;
    }

    // Cameras die after the restore blend needs them no longer.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(CameraCleanupHandle,
            [this]()
            {
                for (TObjectPtr<ACameraActor>& Camera : ShotCameras)
                {
                    if (Camera)
                    {
                        Camera->Destroy();
                    }
                }
                ShotCameras.Reset();
            },
            1.5f, false);
    }

    UE_LOG(LogAstrawild, Log, TEXT("Ending cinematic complete — control restored (ending %d, post-game free roam)."),
        static_cast<int32>(StagedEnding));
}

void UAstrawildEndingCinematicComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    DriveSequence(DeltaTime);
}
