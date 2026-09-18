#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildEndingLetterboxWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UTextBlock;

/**
 * DCP-3 (2026-09-06 user directive — "re-open every deferred item,
 * playable-first") — the ending cinematic presentation overlay.
 *
 * Pure-C++ UMG (zero-asset doctrine, the DialogueWidget lineage): letterbox
 * bars, a full-screen fade layer, and the staged ending titles. The widget
 * renders ONLY — the shot timing, camera work and input lock live in
 * UAstrawildEndingCinematicComponent (the sequence driver), which feeds this
 * overlay its per-phase progress values every tick.
 *
 * Any key / any click during the sequence requests a skip (the component
 * folds the skip into its phase machine — one code path, no duplicated
 * restore logic).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildEndingLetterboxWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Configure the title/subtitle for the ending being staged. */
    void SetEndingTexts(const FText& Title, const FText& Subtitle);

    /** Bar reveal progress 0..1 (0 = off-screen, 1 = full cinematic bars). */
    void SetBarsProgress(const float Progress);

    /** Full-screen fade layer alpha 0..1 (0 = clear, 1 = black). */
    void SetFadeAlpha(const float Alpha);

    /** Title text opacity 0..1 (staged in by the sequence driver). */
    void SetTitleAlpha(const float Alpha);

    /** Subtitle text opacity 0..1. */
    void SetSubtitleAlpha(const float Alpha);

    /** The "press any key" affordance (visible in the final hold only). */
    void SetHintVisible(const bool bVisible);

    /** Fired on any key/click — the component decides what a skip means. */
    DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndingCinematicSkipRequested);
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Ending")
    FOnEndingCinematicSkipRequested OnSkipRequested;

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    void BuildOverlayTree();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    /** Full-screen fade layer (sits under the bars, over the world). */
    UPROPERTY()
    TObjectPtr<UBorder> FadeOverlay;

    UPROPERTY()
    TObjectPtr<UBorder> TopBar;

    UPROPERTY()
    TObjectPtr<UBorder> BottomBar;

    /** Cached slots — bar height animation drives these every tick. */
    UPROPERTY()
    TObjectPtr<UCanvasPanelSlot> TopBarSlot;

    UPROPERTY()
    TObjectPtr<UCanvasPanelSlot> BottomBarSlot;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SubtitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SkipHintText;

    /** Letterbox bar height in slate units at full reveal (≈11% of a 1080 frame). */
    static constexpr float BarHeightUnits = 122.0f;
};
