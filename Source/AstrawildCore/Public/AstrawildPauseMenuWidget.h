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
 *   │ SKILL LOADOUT [Y]  │
 *   │ [Slot 1: —]        │
 *   │ [Slot 2: —]        │
 *   │ [Slot 3: —]        │
 *   │ [Quit To Desktop]  │
 *   └────────────────────┘
 *
 * ESC toggles (AAstrawildPlayerController::TogglePauseMenu). Quit routes through
 * the engine's quit path so PIE ends cleanly and packaged builds exit.
 *
 * DP-4: the SKILL LOADOUT section is the player-chosen build identity —
 * clicking a slot button cycles it through the unlocked skills (skills bound
 * in other slots are skipped), wrapping to empty. Any non-empty loadout
 * narrows the Y-key smart-cast ladder to the bound skills; all-empty keeps
 * the legacy all-unlocked smart-cast (see UAstrawildAttributeComponent).
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

    /** DP-4: slot button clicks — OnClicked delegates cannot carry the slot
     *  index, so each slot routes through its own thunk. */
    UFUNCTION()
    void HandleSkillSlot0Clicked();

    UFUNCTION()
    void HandleSkillSlot1Clicked();

    UFUNCTION()
    void HandleSkillSlot2Clicked();

    /** DP-4: cycle one loadout slot to its next unlocked, unbound skill. */
    void CycleSkillSlot(int32 SlotIndex);

    /** DP-4: rebuild the slot button labels from the live loadout state. */
    void RefreshSkillSlotLabels();

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

    /** DP-4: the loadout section caption. */
    UPROPERTY()
    TObjectPtr<UTextBlock> SkillLoadoutText;

    /** DP-4: the three loadout slot buttons (index = slot). */
    UPROPERTY()
    TArray<TObjectPtr<UButton>> SkillSlotButtons;

    /** DP-4: the labels inside the slot buttons (refreshed on every cycle). */
    UPROPERTY()
    TArray<TObjectPtr<UTextBlock>> SkillSlotLabels;
};
