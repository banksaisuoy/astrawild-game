#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildTypes.h"
#include "AstrawildJournalScreenWidget.generated.h"

class UButton;
class UCanvasPanel;
class UTextBlock;
class UScrollBox;
class UVerticalBox;

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

    /** Rebuild the species listing (call on open; entries change while scanning). */
    void RefreshJournal();

    UAstrawildJournalScreenWidget();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    void BuildWidgetTree();

    UFUNCTION()
    void HandleCloseClicked();

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
};
