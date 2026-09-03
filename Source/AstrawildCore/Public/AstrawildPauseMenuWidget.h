#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildPauseMenuWidget.generated.h"

class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

/**
 * Final production run (loop stage QUIT): the pause menu. Pure-C++ UMG.
 *
 *   ┌ ASTRAWILD — Paused ┐
 *   │ [Resume]           │
 *   │ [Save Now]         │
 *   │ [Quit To Desktop]  │
 *   └────────────────────┘
 *
 * ESC toggles (AAstrawildPlayerController::TogglePauseMenu). Quit routes through
 * the engine's quit path so PIE ends cleanly and packaged builds exit.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildPauseMenuWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Final-audit F-05: focusable so ESC-resume actually fires in UIOnly input mode. */
    UAstrawildPauseMenuWidget();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void BuildWidgetTree();

    UFUNCTION()
    void HandleResumeClicked();

    UFUNCTION()
    void HandleSaveClicked();

    UFUNCTION()
    void HandleQuitClicked();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UVerticalBox> MenuBox;

    UPROPERTY()
    TObjectPtr<UButton> ResumeButton;

    UPROPERTY()
    TObjectPtr<UButton> SaveButton;

    UPROPERTY()
    TObjectPtr<UButton> QuitButton;
};
