#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildDeathOverlayWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;

/**
 * Completion stage (loop stage DEATH): a readable death beat instead of a silent
 * 5-second freeze. Pure-C++ UMG.
 *
 *   ┌────────────────────────────────────┐
 *   │          YOU FELL TO THE VALE      │
 *   │     Returning to camp in 5...      │
 *   └────────────────────────────────────┘
 *
 * Shown by AAstrawildPlayerController::ShowDeathOverlay when the pawn dies;
 * hidden on HandleRespawn. NativeTick drives the countdown text. In PIE the
 * overlay still shows (useful for testing the respawn path).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildDeathOverlayWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    /** Arm the countdown (seconds until the game mode respawns the pawn). */
    void StartCountdown(const float RespawnSeconds);

private:
    void BuildWidgetTree();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UBorder> ScrimBorder;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> CountdownText;

    float RemainingSeconds = 0.0f;
    int32 LastWholeSecond = -1;
};
