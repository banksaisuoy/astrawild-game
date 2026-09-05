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

    /** PCR-1: open the Field Journal (bestiary) — the gamepad-reachable path (P on KB/M). */
    UFUNCTION()
    void HandleJournalClicked();

    /** PCR-2: open the Echo Roster (party-ring management) — the gamepad-reachable path (L on KB/M). */
    UFUNCTION()
    void HandleRosterClicked();

    /** PCR-3: open the world map — the gamepad-reachable path (M on KB/M). */
    UFUNCTION()
    void HandleMapClicked();

    /** PCR-5: open the Hunt Board — the gamepad-reachable path (U on KB/M). */
    UFUNCTION()
    void HandleHuntClicked();

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

    // --- LCP-6 handlers ---

    UFUNCTION()
    void HandleLanHostClicked();

    UFUNCTION()
    void HandleLanFindJoinClicked();

    UFUNCTION()
    void HandleLanDirectConnectClicked();

    /** LCP-6: session state line (found games / errors / mode). */
    void RefreshLanStatus();

    /** LCP-6: the owning game instance's LAN session subsystem. */
    class UAstrawildLANSessionSubsystem* GetLanSubsystem() const;

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

    /** PCR-1: Field Journal entry point for gamepad players (keyboard: P). */
    UPROPERTY()
    TObjectPtr<UButton> JournalButton;

    /** PCR-2: Echo Roster entry point for gamepad players (keyboard: L). */
    UPROPERTY()
    TObjectPtr<UButton> RosterButton;

    /** PCR-3: World Map entry point for gamepad players (keyboard: M). */
    UPROPERTY()
    TObjectPtr<UButton> MapButton;

    /** PCR-5: Hunt Board entry point for gamepad players (keyboard: U). */
    UPROPERTY()
    TObjectPtr<UButton> HuntButton;

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

    // --- LCP-6: LAN CO-OP panel (host / find + join / direct connect) ---

    UPROPERTY()
    TObjectPtr<UTextBlock> LanTitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> LanStatusText;

    UPROPERTY()
    TObjectPtr<UButton> LanHostButton;

    UPROPERTY()
    TObjectPtr<UButton> LanFindJoinButton;

    UPROPERTY()
    TObjectPtr<UButton> LanDirectConnectButton;

    UPROPERTY()
    TObjectPtr<class UEditableTextBox> LanAddressBox;
};
