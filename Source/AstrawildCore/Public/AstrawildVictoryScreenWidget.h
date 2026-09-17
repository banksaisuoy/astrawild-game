#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildVictoryScreenWidget.generated.h"

class UBorder;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

/**
 * Completion stage (loop stage VICTORY): fires when the closing quest of the
 * main chain (Quest_VanguardProtocol by default) completes. Pure-C++ UMG.
 *
 *   ┌────────────────────────────────────┐
 *   │   THE FIRST DAWN AWAKENS           │
 *   │   The Vale answers your signal.    │
 *   │                                    │
 *   │   Days survived      12            │
 *   │   Echoes bonded      4             │
 *   │   Journal observed   38 species    │
 *   │                                    │
 *   │   [ Keep Exploring ]  [ Save ]     │
 *   └────────────────────────────────────┘
 *
 * The world never tears down after victory — the player returns to the free
 * hunt with the full roster, base and research intact (directive: the chain
 * "closes — free hunt begins").
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildVictoryScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** Fill the summary stats before/at display time (server-computed values). */
    void InitializeVictory(int32 InDaysSurvived, int32 InEchoesBonded, int32 InJournalEntries);

private:
    void BuildWidgetTree();
    void RefreshStatsText();

    UFUNCTION()
    void HandleKeepExploringClicked();

    UFUNCTION()
    void HandleSaveClicked();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UBorder> PanelBorder;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> SubtitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> StatsText;

    UPROPERTY()
    TObjectPtr<UButton> KeepExploringButton;

    UPROPERTY()
    TObjectPtr<UButton> SaveButton;

    int32 DaysSurvived = 0;
    int32 EchoesBonded = 0;
    int32 JournalEntries = 0;
};
