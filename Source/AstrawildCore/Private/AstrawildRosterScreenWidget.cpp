#include "AstrawildRosterScreenWidget.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoRosterSubsystem.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

namespace
{
    constexpr float RosterPanelWidth = 760.0f;
    constexpr float RosterPanelHeight = 620.0f;
}

// ---------------------------------------------------------------------------
// Ring-capacity rule (pure — the automation contract pins this)
// ---------------------------------------------------------------------------

bool UAstrawildRosterScreenWidget::CanDeployIntoRing(const int32 CurrentRingSize, const int32 RingMax, const bool bRowCurrentlyBenched)
{
    // Benching is always available; deploying (unbenching) needs a free slot —
    // a full ring would silently drop the spawned actor instead.
    if (!bRowCurrentlyBenched)
    {
        return true;
    }
    return CurrentRingSize < FMath::Max(1, RingMax);
}

// ---------------------------------------------------------------------------
// Row widget
// ---------------------------------------------------------------------------

void UAstrawildRosterRowWidget::InitializeRow(UAstrawildRosterScreenWidget* ParentScreenPtr, const FAstrawildEchoInstanceV2& InRow, const int32 InRingUsed, const int32 InRingMax)
{
    ParentScreen = ParentScreenPtr;
    RowInstanceId = InRow.InstanceId;
    bRowBenched = InRow.bBenched;
    RowDefinitionId = InRow.DefinitionId;
    RowLevel = InRow.Level;
    RowBond = InRow.Bond;
    RowTrust = InRow.Trust;
    RingUsed = InRingUsed;
    RingMax = InRingMax;

    if (WidgetTree && WidgetTree->RootWidget && !RowText)
    {
        BuildRowTree();
    }
}

void UAstrawildRosterRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRowTree();
}

void UAstrawildRosterRowWidget::BuildRowTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildEchoDefinition* Def = Registry ? Registry->FindEcho(RowDefinitionId) : nullptr;
    if (!Def || !ParentScreen)
    {
        return;
    }

    // Identity + progression line.
    FString Line = FString::Printf(TEXT("%s\n%s · %s · Lv %d · Bond %d · Trust %d"),
        *Def->DisplayName.ToString(),
        *UEnum::GetDisplayValueAsText(Def->Element).ToString(),
        *UEnum::GetDisplayValueAsText(Def->Role).ToString(),
        RowLevel, FMath::RoundToInt(RowBond), FMath::RoundToInt(RowTrust));

    // Top work affinity — the "why would I keep this one" answer.
    if (Def->WorkAffinities.Num() > 0)
    {
        const FAstrawildWorkAffinity* Best = nullptr;
        for (const FAstrawildWorkAffinity& Affinity : Def->WorkAffinities)
        {
            if (!Best || Affinity.Affinity > Best->Affinity)
            {
                Best = &Affinity;
            }
        }
        if (Best)
        {
            Line += FString::Printf(TEXT("\nTop work: %s ×%.1f"),
                *UEnum::GetDisplayValueAsText(Best->WorkType).ToString(), Best->Affinity);
        }
    }

    // Ring status.
    Line += FString::Printf(TEXT("\n%s (ring %d/%d)"),
        bRowBenched ? TEXT("BENCHED — stays in the roster, does not follow") : TEXT("IN PARTY RING — follows and fights"),
        RingUsed, RingMax);

    RowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RosterRowText"));
    RowText->SetColorAndOpacity(FSlateColor(bRowBenched
        ? FLinearColor(0.62f, 0.64f, 0.68f, 1.0f)
        : FLinearColor(0.95f, 0.93f, 0.85f, 1.0f)));
    RowText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    RowText->SetAutoWrapText(true);
    RowText->SetText(FText::FromString(Line));

    const bool bCanToggle = CanDeployIntoRing(RingUsed, RingMax, bRowBenched);

    ToggleButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RosterRowToggle"));
    ToggleButton->SetBackgroundColor(bRowBenched
        ? FLinearColor(0.28f, 0.45f, 0.3f, 1.0f)
        : FLinearColor(0.45f, 0.3f, 0.16f, 1.0f));
    ToggleButton->SetIsEnabled(bCanToggle);
    UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RosterRowToggleLabel"));
    ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    ButtonLabel->SetText(FText::FromString(bRowBenched
        ? (bCanToggle ? TEXT("Deploy") : TEXT("Ring full"))
        : TEXT("Bench")));
    ToggleButton->AddChild(ButtonLabel);
    if (bCanToggle)
    {
        ToggleButton->OnClicked.AddDynamic(this, &UAstrawildRosterRowWidget::HandleToggleClicked);
    }

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RosterRow"));
    if (auto* TextSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(RowText)))
    {
        TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TextSlot->SetVerticalAlignment(VAlign_Center);
    }
    if (auto* ButtonSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(ToggleButton)))
    {
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
    }

    WidgetTree->RootWidget = Row;
}

void UAstrawildRosterRowWidget::HandleToggleClicked()
{
    if (ParentScreen)
    {
        ParentScreen->RequestToggleBench(RowInstanceId, !bRowBenched);
    }
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

UAstrawildRosterScreenWidget::UAstrawildRosterScreenWidget()
{
    // F-05 convention: focusable so L/ESC close without a prior mouse click.
    bIsFocusable = true;
}

FReply UAstrawildRosterScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::L || InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleRosterScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildRosterScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshRoster();
}

void UAstrawildRosterScreenWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RosterRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RosterTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.9f, 0.95f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("Echo Roster")));

    SummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RosterSummary"));
    SummaryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.85f, 0.5f, 1.0f)));
    SummaryText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14));

    RowList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("RosterRowList"));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RosterClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RosterCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [L]")));
    CloseButton->AddChild(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildRosterScreenWidget::HandleCloseClicked);

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-RosterPanelWidth * 0.5f, -RosterPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(RosterPanelWidth, 32.0f));
    }
    if (UCanvasPanelSlot* SummarySlot = Canvas->AddChildToCanvas(SummaryText))
    {
        SummarySlot->SetAnchors(FAnchors(0.5f, 0.5f));
        SummarySlot->SetPosition(FVector2D(-RosterPanelWidth * 0.5f, -RosterPanelHeight * 0.5f + 36.0f));
        SummarySlot->SetSize(FVector2D(RosterPanelWidth, 24.0f));
    }
    if (UCanvasPanelSlot* ListSlot = Canvas->AddChildToCanvas(RowList))
    {
        ListSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        ListSlot->SetPosition(FVector2D(-RosterPanelWidth * 0.5f, -RosterPanelHeight * 0.5f + 68.0f));
        ListSlot->SetSize(FVector2D(RosterPanelWidth, RosterPanelHeight - 120.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(RosterPanelWidth * 0.5f - 130.0f, RosterPanelHeight * 0.5f - 38.0f));
        CloseSlot->SetSize(FVector2D(130.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildRosterScreenWidget::RefreshRoster()
{
    if (!RowList)
    {
        return;
    }

    AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>();
    UWorld* World = GetWorld();
    if (!PC || !World)
    {
        return;
    }

    // One read path everywhere: the controller mirror (the host keeps it in
    // sync through the subsystem's push; a pure client receives it replicated).
    const TArray<FAstrawildEchoInstanceV2> Rows = PC->RosterMirror;

    const UAstrawildEchoRosterSubsystem* Roster = (World->GetGameInstance())
        ? World->GetGameInstance()->GetSubsystem<UAstrawildEchoRosterSubsystem>() : nullptr;
    const int32 RingMax = Roster ? Roster->MaxPartySize : 3;

    RowList->ClearChildren();

    int32 InRing = 0;
    for (const FAstrawildEchoInstanceV2& Row : Rows)
    {
        if (UAstrawildEchoRosterSubsystem::ShouldSpawnInPartyRing(Row))
        {
            ++InRing;
        }
    }

    if (Rows.IsEmpty())
    {
        UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RosterEmpty"));
        EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.64f, 0.68f, 1.0f)));
        EmptyText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 13));
        EmptyText->SetAutoWrapText(true);
        EmptyText->SetText(FText::FromString(
            TEXT("No captured Echoes yet. Weaken a wild Echo, then use the Resonator [E] to capture it — every capture lands here.")));
        RowList->AddChild(EmptyText);
    }

    int32 RowIndex = 0;
    for (const FAstrawildEchoInstanceV2& Row : Rows)
    {
        UAstrawildRosterRowWidget* RowWidget = WidgetTree->ConstructWidget<UAstrawildRosterRowWidget>(
            UAstrawildRosterRowWidget::StaticClass(), *FString::Printf(TEXT("RosterRow%d"), RowIndex++));
        RowWidget->InitializeRow(this, Row, InRing, RingMax);
        if (UScrollBoxSlot* RowSlot = RowList->AddChild(RowWidget))
        {
            RowSlot->SetPadding(FMargin(6.0f, 5.0f, 6.0f, 2.0f));
        }
    }

    SummaryText->SetText(FText::FromString(FString::Printf(TEXT("%d in the party ring of %d · %d captured"),
        InRing, RingMax, Rows.Num())));
}

bool UAstrawildRosterScreenWidget::RequestToggleBench(const FGuid& InstanceId, const bool bBenched)
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        const bool bAccepted = PC->RequestSetEchoBenched(InstanceId, bBenched);
        if (bAccepted)
        {
            // Host/standalone mutated synchronously — refresh now. A remote
            // client refreshes when the server's mirror push arrives (OnRep).
            RefreshRoster();
        }
        return bAccepted;
    }
    return false;
}

void UAstrawildRosterScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleRosterScreen();
    }
}
