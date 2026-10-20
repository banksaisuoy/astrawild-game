#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildTypes.h"
#include "AstrawildJournalScreenWidget.generated.h"

class UButton;
class UBorder;
class UCanvasPanel;
class UTextBlock;
class UScrollBox;
class UVerticalBox;
class UAstrawildEchoDefinition;

/**
 * PCR-1 (PG-1 gap closed): the Field Journal (bestiary) screen. The
 * JournalSubsystem has tracked scan/food/habitat/weakness knowledge,
 * observation progress and encounter counts (saved) since the Final Run —
 * this is the first player-facing surface for that data.
 *
 * Pure-C++ UMG (same construction pattern as the research/inventory screens).
 *
 *   ┌ Field Journal ── 12 of 229 observed · 4 fully studied ── [Close] ┐
 *   │ Terraquill      Flora · Gatherer · Common                       │
 *   │   Scanned ✓  Food ✓  Habitat ✗  Weakness ✗   74% · 9 encounters │
 *   │ ???             signal unresolved — observe to reveal            │
 *   └──────────────────────────────────────────────────────────────────┘
 *
 * DCP-5 (2026-09-06): discovered rows are now CLICKABLE (the row widget
 * below) and open a per-species detail panel — full identity, description,
 * stats, weakness/resist, habits, habitat/food (knowledge-gated), ability
 * kit, mutation spec, evolution line, loot and capture difficulty. The list
 * and the detail view swap in the same frame region; [Back] returns.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildJournalScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * Knowledge classification for one journal entry — extracted as a pure
     * function so the automation contract can pin the rules world-free:
     *   UNKNOWN    — nothing observed (all flags false, no progress, no encounters)
     *   OBSERVED   — some progress but knowledge flags still missing
     *   STUDIED    — all four knowledge flags true (the capture-bonus state)
     */
    enum class EKnowledgeState : uint8
    {
        Unknown,
        Observed,
        Studied
    };

    /** Pure classification — no world, no widgets; the test contract pins this. */
    static EKnowledgeState ClassifyKnowledgeState(const FAstrawildJournalEntry& Entry);

    /** True when the entry should be listed as a discovered species. */
    static bool IsEntryDiscovered(const FAstrawildJournalEntry& Entry);

    /**
     * DCP-5: the per-species detail line set (pure — world-free, the test
     * contract pins the knowledge-gating rules). Returns the detail body for
     * a discovered species: everything the codex knows, gated exactly like
     * the row list (weakness/habitat/food hidden until discovered).
     */
    static FString BuildSpeciesDetailText(const UAstrawildEchoDefinition* Def, const FAstrawildJournalEntry& Entry);

    /** Rebuild the species listing (call on open; entries change while scanning). */
    void RefreshJournal();

    /** DCP-5: open the per-species detail view (hides the list until Back). */
    void ShowSpeciesDetail(FName SpeciesId);

    /** DCP-5: return from the detail view to the full list. */
    void BackToList();

    UAstrawildJournalScreenWidget();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void BuildWidgetTree();

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleBackToListClicked();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SummaryText;

    UPROPERTY()
    TObjectPtr<UScrollBox> SpeciesList;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;

    // --- DCP-5: the detail view (swaps with SpeciesList in the same region) ---

    UPROPERTY()
    TObjectPtr<UBorder> DetailPanel;

    UPROPERTY()
    TObjectPtr<UScrollBox> DetailScroll;

    UPROPERTY()
    TObjectPtr<UTextBlock> DetailText;

    UPROPERTY()
    TObjectPtr<UButton> BackButton;

    /** The species currently detailed (NAME_None = list mode). */
    FName DetailSpeciesId = NAME_None;
};

/**
 * DCP-5: one clickable journal row (the RosterRow pattern). Read-only text
 * inside a subtle button; the click routes to the owning screen's detail
 * view. Unknown species keep the bare TextBlock (nothing to detail).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildJournalRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Bind to the owning screen + species; sets the row label. */
    void InitializeRow(UAstrawildJournalScreenWidget* InParent, FName InSpeciesId, const FString& RowText, const FLinearColor& RowColor);

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleClicked();

    UPROPERTY()
    TObjectPtr<UButton> RowButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> RowLabel;

    UPROPERTY()
    TObjectPtr<UAstrawildJournalScreenWidget> ParentScreen;

    FName SpeciesId = NAME_None;
};
