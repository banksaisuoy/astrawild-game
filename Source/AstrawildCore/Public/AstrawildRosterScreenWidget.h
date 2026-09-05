#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildTypes.h"
#include "AstrawildRosterScreenWidget.generated.h"

class AAstrawildPlayerController;
class UAstrawildRosterScreenWidget;
class UButton;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;

/**
 * PCR-2 (PG-2 gap closed): one captured-Echo row of the roster screen.
 *
 *   ┌ Terraquill        Flora · Gatherer · Lv 4 · Bond 62 ── [Bench] ┐
 *   │ Top work: Gathering ×1.9            IN PARTY RING (2/3)        │
 *   └────────────────────────────────────────────────────────────────┘
 *
 * The button benches/deploys the instance through the server-authoritative
 * path (PC->RequestSetEchoBenched) — never a direct roster mutation.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildRosterRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeRow(UAstrawildRosterScreenWidget* ParentScreen, const FAstrawildEchoInstanceV2& InRow, int32 InRingUsed, int32 InRingMax);

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleToggleClicked();

    void BuildRowTree();

    UPROPERTY()
    TObjectPtr<UAstrawildRosterScreenWidget> ParentScreen;

    FGuid RowInstanceId;

    bool bRowBenched = false;

    /** Row snapshot for rendering (identity + progression + ring status). */
    FName RowDefinitionId = NAME_None;
    int32 RowLevel = 1;
    float RowBond = 0.0f;
    float RowTrust = 0.0f;
    int32 RingUsed = 0;
    int32 RingMax = 3;

    UPROPERTY()
    TObjectPtr<UButton> ToggleButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> RowText;
};

/**
 * PCR-2 (PG-2): the captured-Echo roster / party-ring management screen.
 * The roster subsystem always held per-player captured Echoes (saved,
 * partitioned in co-op) with MaxPartySize ring slots — but which Echoes
 * occupied the ring was roster ORDER, invisible to the player. This screen
 * is the management surface: every captured Echo listed with identity +
 * progression, bench/deploy chooses the ring.
 *
 *   ┌ Echo Roster ── 3 in party ring of 3 · 7 captured ── [Close] ┐
 *   │ (rows — see UAstrawildRosterRowWidget)                       │
 *   │ Benched Echoes keep working from the roster data; only the   │
 *   │ follow-ring is chosen here.                                  │
 *   └──────────────────────────────────────────────────────────────┘
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildRosterScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * Pure ring-capacity rule (automation-tested): a deploy is allowed when
     * the ring has a free slot; benching is always allowed. World-free.
     */
    static bool CanDeployIntoRing(int32 CurrentRingSize, int32 RingMax, bool bRowCurrentlyBenched);

    /** Rebuild the row list (host: live subsystem slice; client: the replicated mirror). */
    void RefreshRoster();

    /** Row-button target: routes through the controller's authoritative path. */
    bool RequestToggleBench(const FGuid& InstanceId, bool bBenched);

    UAstrawildRosterScreenWidget();

protected:
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
    friend class UAstrawildRosterRowWidget;

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
    TObjectPtr<UScrollBox> RowList;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
