#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildHuntScreenWidget.generated.h"

class AAstrawildPlayerController;
class UAstrawildHuntScreenWidget;
class UButton;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;

/**
 * PCR-5 (PG-5): one hunt-contract row of the Hunt Board.
 *
 *   ┌ Cull Duskmoth (5) ── Duskmoth ×3/5 ── reward: Dawn Shard ×3 [Claim] ┐
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildHuntRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeRow(UAstrawildHuntScreenWidget* ParentScreen, FName InHuntId, int32 InProgress, int32 InRequired);

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleClaimClicked();

    void BuildRowTree();

    UPROPERTY()
    TObjectPtr<UAstrawildHuntScreenWidget> ParentScreen;

    FName RowHuntId = NAME_None;

    int32 RowProgress = 0;

    int32 RowRequired = 1;

    UPROPERTY()
    TObjectPtr<UButton> ClaimButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> RowText;
};

/**
 * PCR-5 (PG-5): the Hunt Board — the post-game repeatable activity surface.
 * Rows show every cull contract with live progress + rewards; the Claim
 * button routes through the controller's server-authoritative path.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildHuntScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Rebuild the contract rows (host: live subsystem; client: read-only view). */
    void RefreshHunts();

    /** Row-button target: routes through the controller's authority path. */
    void RequestClaim(FName HuntId);

    UAstrawildHuntScreenWidget();

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
    TObjectPtr<UTextBlock> SubtitleText;

    UPROPERTY()
    TObjectPtr<UScrollBox> RowList;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
