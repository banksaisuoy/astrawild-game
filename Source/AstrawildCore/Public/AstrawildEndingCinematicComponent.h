#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildEndingCinematicComponent.generated.h"

class ACameraActor;
class AAstrawildGameState;
class UAstrawildEndingLetterboxWidget;

/**
 * DCP-3 (2026-09-06 user directive — "re-open every deferred item,
 * playable-first") — the ending cinematic SEQUENCE DRIVER.
 *
 * Pure C++ camera staging (no Sequencer, no assets — the zero-asset
 * doctrine): when the ending verdict lands, the LOCAL player gets a staged
 * shot sequence (letterbox + fade + three camera cuts + title cards) before
 * control returns. The persistent HUD ending banner stays exactly as it was
 * — the cinematic is presentation, never state.
 *
 * Wiring (covers every topology):
 *  - server/host: AAstrawildGameState::OnEndingTriggered (instant),
 *  - remote clients: a 0.5s poll of the replicated EndingState (OnRep-style;
 *    GameState::OnRep_EndingState stays reserved for future polish),
 *  - both paths funnel into ONE guarded StartCinematic (bCinematicPlayed).
 *
 * Co-op: every machine runs its own local presentation (this component lives
 * on the PlayerController, which exists locally per player). Non-local
 * controllers skip — a cinematic only ever plays for the player watching.
 *
 * Skip: any key/click (the overlay widget forwards to HandleSkipRequested)
 * folds the sequence to the fade-out phase — one exit path, same restore.
 */
UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildEndingCinematicComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildEndingCinematicComponent();

    /** True while the staged sequence is running (screens stay closed). */
    bool IsCinematicRunning() const { return bRunning; }

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    /** Server/host path — the verdict broadcast (instant response). */
    UFUNCTION()
    void HandleEndingTriggered(EAstrawildEndingState Ending, EAstrawildEndingState OldEnding);

    /** Client path — poll the replicated verdict (0.5s cadence). */
    void PollEndingState();

    /** The ONE guarded entry (idempotent per ending per session). */
    void StartCinematic(EAstrawildEndingState Ending);

    /** Phase machine — drives bars/fades/texts/camera cuts from SequenceTime. */
    void DriveSequence(const float DeltaTime);

    /** Restore control: view target back to the pawn, input back to game. */
    void FinishCinematic();

    /** Skip fold: jump to the fade-out phase (single exit path). */
    UFUNCTION()
    void HandleSkipRequested();

    void EnterShot(int32 ShotIndex, float BlendTimeSeconds);

    AAstrawildGameState* GetGameState() const;
    class UAstrawildPlayerController* GetAstrawildController() const;

    // --- Sequence timing (seconds; total ≈ 18.7, skippable) ---
    static constexpr float BarsInEnd = 1.2f;        // letterbox reveal.
    static constexpr float ShotAStart = 1.2f;       // low hero shot.
    static constexpr float TitleInStart = 3.2f;     // title card.
    static constexpr float ShotBStart = 6.2f;       // high wide orbit.
    static constexpr float SubtitleInStart = 7.7f;  // subtitle card.
    static constexpr float ShotCStart = 11.7f;      // return-behind shot.
    static constexpr float HintInStart = 14.7f;     // skip affordance.
    static constexpr float FadeOutStart = 17.2f;    // to black.
    static constexpr float SequenceEnd = 18.7f;     // control restored.
    static constexpr float ShotBlendTime = 1.4f;

    // --- State ---
    bool bCinematicPlayed = false;
    bool bRunning = false;
    bool bShotAEntered = false;
    bool bShotBEntered = false;
    bool bShotCEntered = false;
    float SequenceTime = 0.0f;
    EAstrawildEndingState StagedEnding = EAstrawildEndingState::None;

    UPROPERTY()
    TObjectPtr<UAstrawildEndingLetterboxWidget> Overlay;

    /** Transient viewpoints (destroyed after the restore blend). */
    UPROPERTY()
    TArray<TObjectPtr<ACameraActor>> ShotCameras;

    FTimerHandle CameraCleanupHandle;

    /** Client-side verdict poll (0.5s cadence, cheap bool guard). */
    FTimerHandle PollHandle;
};
