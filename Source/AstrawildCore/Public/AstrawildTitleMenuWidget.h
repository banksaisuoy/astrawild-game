#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildTitleMenuWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

/**
 * Completion stage (loop stage TITLE): the ASTRAWILD title screen. Pure-C++ UMG.
 *
 *   ┌──────────────────────────────────┐
 *   │  ASTRAWILD                       │
 *   │  Echoes of the First Dawn        │
 *   │                                  │
 *   │  [ Begin New Journey ]           │
 *   │  [ Continue ]                    │
 *   │  [ Controls ]                    │
 *   │  [ Quit ]                        │
 *   └──────────────────────────────────┘
 *
 * Shown by AAstrawildPlayerController on standalone boot (PIE keeps iterating
 * without it). Continue is disabled when no save slot exists. The world is
 * already procedural by design, so "New Journey" simply drops the player into
 * the freshly-bootstrapped session, while Continue routes through
 * UAstrawildSaveSubsystem::LoadLatest (autosave wins when newer).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildTitleMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** Disable the Continue button when no save exists (called after construction). */
    void RefreshContinueAvailability();

protected:
    virtual void NativeDestruct() override;

private:
    void BuildWidgetTree();
    void BuildControlsPanel();

    UFUNCTION()
    void HandleNewGameClicked();

    UFUNCTION()
    void HandleContinueClicked();

    UFUNCTION()
    void HandleControlsClicked();

    UFUNCTION()
    void HandleBackFromControlsClicked();

    UFUNCTION()
    void HandleQuitClicked();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UBorder> PanelBorder;

    /** The main button column (hidden while the controls panel is open). */
    UPROPERTY()
    TObjectPtr<UBorder> MenuPanel;

    UPROPERTY()
    TObjectPtr<UBorder> ControlsPanel;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SubtitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> FooterText;

    UPROPERTY()
    TObjectPtr<UButton> NewGameButton;

    UPROPERTY()
    TObjectPtr<UButton> ContinueButton;

    UPROPERTY()
    TObjectPtr<UButton> ControlsButton;

    UPROPERTY()
    TObjectPtr<UButton> QuitButton;

    UPROPERTY()
    TObjectPtr<UButton> ControlsBackButton;
};
