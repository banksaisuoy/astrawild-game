#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDialogueWidget.generated.h"

class AAstrawildNPCCharacter;
class UAstrawildDialogueComponent;
class UAstrawildDialogueWidget;
class UButton;
class UBorder;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UAstrawildDialogueTreeDefinition;

/**
 * Production V2 Batch 3 — one dialogue choice row. Pure-C++ UMG (same zero-asset
 * doctrine as the shop screen): builds its own button tree, evaluates nothing
 * itself — visibility filtering and consequence application stay in the
 * dialogue component so the exact same logic runs in the automation tests.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildDialogueChoiceRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeRow(UAstrawildDialogueWidget* InParent, const FAstrawildDialogueChoice& InChoice, int32 InChoiceIndex);

    /** LCP-3: this row's index within the current node's choice list (remote submission routing). */
    int32 RowChoiceIndex = INDEX_NONE;

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleClicked();

    void BuildRowTree();

    UPROPERTY()
    TObjectPtr<UAstrawildDialogueWidget> ParentDialogue;

    UPROPERTY()
    TObjectPtr<UButton> ChoiceButton;

    /** Consequence copy (the tree may be rebuilt between frames). */
    FAstrawildDialogueChoice Choice;
};

/**
 * Production V2 Batch 3 — the conversation screen (P12 Story/NPC).
 * Pure-C++ UMG built entirely in InitializeDialogue:
 *
 *   ┌─ Warden Maren ──────────────────────── (×) ┐
 *   │  "The fields are calm — for now."          │
 *   │  ── your reply ──                           │
 *   │  >  Ask about the Vale                      │
 *   │  >  Accept: First Light                     │
 *   │  >  Leave                                   │
 *   └─────────────────────────────────────────────┘
 *
 * NPC lines play in order (click the panel / the Continue button to advance);
 * once the last line is on screen the player replies. Replies with unmet
 * conditions never render; taking a reply applies its consequences through
 * UAstrawildDialogueComponent (quests, flags, items, research) and either
 * continues to the next node or closes. bOpenShop replies close the dialogue
 * and hand off to the vendor shop screen. Input returns to game-only on close.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildDialogueWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Binds NPC + tree and (re)builds the panel for the entry node. */
    void InitializeDialogue(AAstrawildNPCCharacter* InNpc, UAstrawildDialogueTreeDefinition* InTree);

    /** Advance to the next NPC line (called by the panel click / continue button). */
    void AdvanceLine();

    /** Player picked a choice — apply consequences, continue or close. */
    void SelectChoice(const FAstrawildDialogueChoice& Choice);

    /** LCP-3: index-aware selection (remote clients submit the index to the server). */
    void SelectChoiceByIndex(int32 ChoiceIndex, const FAstrawildDialogueChoice& Choice);

    /** Close the conversation (leave button, choice end, or shop hand-off). */
    void CloseDialogue();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dialogue|UI")
    bool IsChoicePhase() const { return CurrentLineIndex >= CurrentLines.Num(); }

    AAstrawildNPCCharacter* GetDialogueNpc() const { return DialogueNpc; }

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    UFUNCTION()
    void HandleContinueClicked();

    UFUNCTION()
    void HandleLeaveClicked();

    void BuildPanelTree();
    void EnterNode(FName NodeId);
    void RefreshBody();
    bool RootCanvasGuard() const;

    UAstrawildDialogueComponent* GetDialogueComponent() const;
    class AAstrawildPlayerController* GetPlayerController() const;

    UPROPERTY()
    TObjectPtr<AAstrawildNPCCharacter> DialogueNpc;

    UPROPERTY()
    TObjectPtr<UAstrawildDialogueTreeDefinition> Tree;

    /** Node cursor. */
    UPROPERTY()
    FName CurrentNodeId = NAME_None;

    /** Line cursor inside the current node. */
    int32 CurrentLineIndex = 0;

    TArray<FAstrawildDialogueLine> CurrentLines;

    // --- Built widgets (rebuilt per node) ---

    UPROPERTY()
    TObjectPtr<UBorder> BackdropBorder;

    UPROPERTY()
    TObjectPtr<UTextBlock> SpeakerText;

    UPROPERTY()
    TObjectPtr<UBorder> LineBorder;

    UPROPERTY()
    TObjectPtr<UTextBlock> LineText;

    UPROPERTY()
    TObjectPtr<UTextBlock> ReplyHeaderText;

    UPROPERTY()
    TObjectPtr<UButton> ContinueButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> ContinueText;

    UPROPERTY()
    TObjectPtr<UScrollBox> ChoiceList;

    UPROPERTY()
    TObjectPtr<UButton> LeaveButton;
};
