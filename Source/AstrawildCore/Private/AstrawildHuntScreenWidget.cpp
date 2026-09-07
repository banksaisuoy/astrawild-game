#include "AstrawildHuntScreenWidget.h"

#include "AstrawildCore.h"
#include "AstrawildHuntSubsystem.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildPlayerController.h"
#include "AstrawildDataAssets.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/World.h"

namespace
{
    constexpr float HuntPanelWidth = 720.0f;
    constexpr float HuntPanelHeight = 560.0f;
}

// ---------------------------------------------------------------------------
// Row widget
// ---------------------------------------------------------------------------

void UAstrawildHuntRowWidget::InitializeRow(UAstrawildHuntScreenWidget* ParentScreenPtr, const FName InHuntId, const int32 InProgress, const int32 InRequired)
{
    ParentScreen = ParentScreenPtr;
    RowHuntId = InHuntId;
    RowProgress = InProgress;
    RowRequired = FMath::Max(1, InRequired);

    if (WidgetTree && WidgetTree->RootWidget && !RowText)
    {
        BuildRowTree();
    }
}

void UAstrawildHuntRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRowTree();
}

void UAstrawildHuntRowWidget::BuildRowTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UWorld* World = GetWorld();
    AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildHuntSubsystem* Hunts = World ? World->GetSubsystem<UAstrawildHuntSubsystem>() : nullptr;
    if (!PC || !Registry || !Hunts || !ParentScreen)
    {
        return;
    }

    const FName SpeciesId = Hunts->GetHuntSpeciesId(RowHuntId);
    const UAstrawildEchoDefinition* Species = Registry->FindEcho(SpeciesId);
    const FName RewardId = Hunts->GetHuntRewardItemId(RowHuntId);
    const UAstrawildItemDefinition* Reward = Registry->FindItem(RewardId);
    const int32 RewardQty = Hunts->GetHuntRewardQuantity(RowHuntId);
    const bool bComplete = RowProgress >= RowRequired;

    RowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HuntRowText"));
    RowText->SetColorAndOpacity(FSlateColor(bComplete
        ? FLinearColor(0.62f, 0.92f, 0.72f, 1.0f)
        : FLinearColor(0.95f, 0.93f, 0.85f, 1.0f)));
    RowText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    RowText->SetAutoWrapText(true);
    RowText->SetText(FText::FromString(FString::Printf(TEXT("%s\nCull %s anywhere in the world\nProgress %d/%d  \u2192  reward: %s x%d"),
        *Hunts->GetHuntIds().Contains(RowHuntId) ? RowHuntId.ToString() : TEXT("?"),
        Species ? *Species->DisplayName.ToString() : *SpeciesId.ToString(),
        RowProgress, RowRequired,
        Reward ? *Reward->DisplayName.ToString() : *RewardId.ToString(),
        RewardQty)));

    ClaimButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HuntRowClaim"));
    ClaimButton->SetBackgroundColor(bComplete
        ? FLinearColor(0.28f, 0.45f, 0.3f, 1.0f)
        : FLinearColor(0.35f, 0.35f, 0.38f, 1.0f));
    ClaimButton->SetIsEnabled(bComplete);
    UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HuntRowClaimLabel"));
    ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    ButtonLabel->SetText(FText::FromString(bComplete ? TEXT("Claim") : TEXT("In progress")));
    ClaimButton->AddChild(ButtonLabel);
    if (bComplete)
    {
        ClaimButton->OnClicked.AddDynamic(this, &UAstrawildHuntRowWidget::HandleClaimClicked);
    }

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HuntRow"));
    if (auto* TextSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(RowText)))
    {
        TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TextSlot->SetVerticalAlignment(VAlign_Center);
    }
    if (auto* ButtonSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(ClaimButton)))
    {
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
    }

    WidgetTree->RootWidget = Row;
}

void UAstrawildHuntRowWidget::HandleClaimClicked()
{
    if (ParentScreen)
    {
        ParentScreen->RequestClaim(RowHuntId);
    }
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

UAstrawildHuntScreenWidget::UAstrawildHuntScreenWidget()
{
    // F-05 convention: focusable so U/ESC close without a prior mouse click.
    bIsFocusable = true;
}

FReply UAstrawildHuntScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::U || InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleHuntScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildHuntScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshHunts();
}

void UAstrawildHuntScreenWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("HuntRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HuntTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.75f, 0.45f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("Hunt Board")));

    SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HuntSubtitle"));
    SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.88f, 0.92f, 1.0f)));
    SubtitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    SubtitleText->SetAutoWrapText(true);
    SubtitleText->SetText(FText::FromString(TEXT("Repeatable cull contracts — cull the target anywhere (party counts together), then claim. Rounds reset after every claim.")));

    RowList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("HuntRowList"));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HuntClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HuntCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [U]")));
    CloseButton->AddChild(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildHuntScreenWidget::HandleCloseClicked);

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-HuntPanelWidth * 0.5f, -HuntPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(HuntPanelWidth, 30.0f));
    }
    if (UCanvasPanelSlot* SubtitleSlot = Canvas->AddChildToCanvas(SubtitleText))
    {
        SubtitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        SubtitleSlot->SetPosition(FVector2D(-HuntPanelWidth * 0.5f, -HuntPanelHeight * 0.5f + 34.0f));
        SubtitleSlot->SetSize(FVector2D(HuntPanelWidth, 40.0f));
    }
    if (UCanvasPanelSlot* ListSlot = Canvas->AddChildToCanvas(RowList))
    {
        ListSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        ListSlot->SetPosition(FVector2D(-HuntPanelWidth * 0.5f, -HuntPanelHeight * 0.5f + 80.0f));
        ListSlot->SetSize(FVector2D(HuntPanelWidth, HuntPanelHeight - 130.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(HuntPanelWidth * 0.5f - 130.0f, HuntPanelHeight * 0.5f - 38.0f));
        CloseSlot->SetSize(FVector2D(130.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildHuntScreenWidget::RefreshHunts()
{
    if (!RowList)
    {
        return;
    }

    AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>();
    UWorld* World = GetWorld();
    const UAstrawildHuntSubsystem* Hunts = World ? World->GetSubsystem<UAstrawildHuntSubsystem>() : nullptr;
    if (!PC || !Hunts)
    {
        return;
    }

    RowList->ClearChildren();

    // Pure LAN clients see zero-progress rows (hunt state lives server-side —
    // the same mirror-free read model the roster had before PCR-2; claims
    // route through the server RPC, which validates against real state).
    const FName PlayerKey = PC->GetPlayerKey();
    const TArray<FName> HuntIds = Hunts->GetHuntIds();

    int32 RowIndex = 0;
    for (const FName HuntId : HuntIds)
    {
        UAstrawildHuntRowWidget* Row = WidgetTree->ConstructWidget<UAstrawildHuntRowWidget>(
            UAstrawildHuntRowWidget::StaticClass(), *FString::Printf(TEXT("HuntRow%d"), RowIndex++));
        Row->InitializeRow(this, HuntId,
            Hunts->GetHuntProgress(HuntId, PlayerKey),
            Hunts->GetHuntRequiredDefeats(HuntId));
        if (UScrollBoxSlot* RowSlot = RowList->AddChild(Row))
        {
            RowSlot->SetPadding(FMargin(6.0f, 5.0f, 6.0f, 2.0f));
        }
    }
}

void UAstrawildHuntScreenWidget::RequestClaim(const FName HuntId)
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->RequestClaimHunt(HuntId);
        RefreshHunts();
    }
}

void UAstrawildHuntScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleHuntScreen();
    }
}
