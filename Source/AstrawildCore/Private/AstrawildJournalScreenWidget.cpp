#include "AstrawildJournalScreenWidget.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"

namespace
{
    constexpr float JournalPanelWidth = 760.0f;
    constexpr float JournalPanelHeight = 620.0f;
}

// ---------------------------------------------------------------------------
// Knowledge classification (pure — the automation contract pins this)
// ---------------------------------------------------------------------------

UAstrawildJournalScreenWidget::EKnowledgeState UAstrawildJournalScreenWidget::ClassifyKnowledgeState(const FAstrawildJournalEntry& Entry)
{
    if (IsEntryDiscovered(Entry))
    {
        const bool bAllKnowledge = Entry.bScanned && Entry.bFoodDiscovered && Entry.bHabitatDiscovered && Entry.bWeaknessDiscovered;
        return bAllKnowledge ? EKnowledgeState::Studied : EKnowledgeState::Observed;
    }
    return EKnowledgeState::Unknown;
}

bool UAstrawildJournalScreenWidget::IsEntryDiscovered(const FAstrawildJournalEntry& Entry)
{
    // Any real contact with the species reveals it in the journal: a completed
    // scan, any knowledge flag, partial observation progress, or an encounter.
    return Entry.bScanned || Entry.bFoodDiscovered || Entry.bHabitatDiscovered
        || Entry.bWeaknessDiscovered || Entry.ObservationProgress > 0.0f || Entry.TimesEncountered > 0;
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

UAstrawildJournalScreenWidget::UAstrawildJournalScreenWidget()
{
    // Final-audit F-05 convention: focusable so P/ESC close without a prior
    // mouse click in UIOnly input mode.
    bIsFocusable = true;
}

FReply UAstrawildJournalScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (InKeyEvent.GetKey() == EKeys::P || InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleJournalScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildJournalScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshJournal();
}

void UAstrawildJournalScreenWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("JournalRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JournalTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.92f, 0.78f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("Field Journal")));

    SummaryText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JournalSummary"));
    SummaryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.85f, 0.5f, 1.0f)));
    SummaryText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14));

    SpeciesList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("JournalSpeciesList"));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("JournalClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("JournalCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [P]")));
    CloseButton->AddChild(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildJournalScreenWidget::HandleCloseClicked);

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-JournalPanelWidth * 0.5f, -JournalPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(JournalPanelWidth, 32.0f));
    }
    if (UCanvasPanelSlot* SummarySlot = Canvas->AddChildToCanvas(SummaryText))
    {
        SummarySlot->SetAnchors(FAnchors(0.5f, 0.5f));
        SummarySlot->SetPosition(FVector2D(-JournalPanelWidth * 0.5f, -JournalPanelHeight * 0.5f + 36.0f));
        SummarySlot->SetSize(FVector2D(JournalPanelWidth, 24.0f));
    }
    if (UCanvasPanelSlot* ListSlot = Canvas->AddChildToCanvas(SpeciesList))
    {
        ListSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        ListSlot->SetPosition(FVector2D(-JournalPanelWidth * 0.5f, -JournalPanelHeight * 0.5f + 68.0f));
        ListSlot->SetSize(FVector2D(JournalPanelWidth, JournalPanelHeight - 120.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(JournalPanelWidth * 0.5f - 130.0f, JournalPanelHeight * 0.5f - 38.0f));
        CloseSlot->SetSize(FVector2D(130.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildJournalScreenWidget::RefreshJournal()
{
    if (!SpeciesList)
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildJournalSubsystem* Journal = World ? World->GetSubsystem<UAstrawildJournalSubsystem>() : nullptr;
    if (!Registry || !Journal)
    {
        return;
    }

    SpeciesList->ClearChildren();

    const TArray<UAstrawildEchoDefinition*> Definitions = Registry->GetAllEchoDefinitions();

    int32 KnownCount = 0;
    int32 StudiedCount = 0;
    int32 RowIndex = 0;

    for (const UAstrawildEchoDefinition* Def : Definitions)
    {
        if (!Def)
        {
            continue;
        }

        const FAstrawildJournalEntry Entry = Journal->GetEntry(Def->DefinitionId);
        const EKnowledgeState State = ClassifyKnowledgeState(Entry);
        if (State != EKnowledgeState::Unknown)
        {
            ++KnownCount;
        }
        if (State == EKnowledgeState::Studied)
        {
            ++StudiedCount;
        }

        // One row per species — a fixed two-line entry (identity line +
        // knowledge line) built from a single multiline TextBlock so 229 rows
        // stay cheap (no per-row widget class needed for read-only data).
        UTextBlock* Row = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("JournalRow%d"), RowIndex++));

        FString RowText;
        FLinearColor RowColor(0.62f, 0.64f, 0.68f, 1.0f); // dim unknown
        if (State == EKnowledgeState::Unknown)
        {
            RowText = TEXT("???  —  signal unresolved (observe to reveal)");
        }
        else
        {
            RowColor = State == EKnowledgeState::Studied
                ? FLinearColor(0.62f, 0.92f, 0.72f, 1.0f)
                : FLinearColor(0.95f, 0.93f, 0.85f, 1.0f);

            const FString FlagLine = FString::Printf(TEXT("Scanned %s  Food %s  Habitat %s  Weakness %s"),
                Entry.bScanned ? TEXT("\u2713") : TEXT("\u2717"),
                Entry.bFoodDiscovered ? TEXT("\u2713") : TEXT("\u2717"),
                Entry.bHabitatDiscovered ? TEXT("\u2713") : TEXT("\u2717"),
                Entry.bWeaknessDiscovered ? TEXT("\u2713") : TEXT("\u2717"));

            RowText = FString::Printf(TEXT("%s\n%s · %s · %s\n%s  %d%% observed · %d encounter%s"),
                *Def->DisplayName.ToString(),
                *UEnum::GetDisplayValueAsText(Def->Element).ToString(),
                *UEnum::GetDisplayValueAsText(Def->Role).ToString(),
                *UEnum::GetDisplayValueAsText(Def->Rarity).ToString(),
                *FlagLine,
                FMath::RoundToInt(Entry.ObservationProgress),
                Entry.TimesEncountered,
                Entry.TimesEncountered == 1 ? TEXT("") : TEXT("s"));
        }

        Row->SetColorAndOpacity(FSlateColor(RowColor));
        Row->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
        Row->SetText(FText::FromString(RowText));

        if (UScrollBoxSlot* RowSlot = SpeciesList->AddChild(Row))
        {
            RowSlot->SetPadding(FMargin(6.0f, 5.0f, 6.0f, 2.0f));
        }
    }

    // Totals always derive from the registry — never a hardcoded census value.
    SummaryText->SetText(FText::FromString(FString::Printf(TEXT("%d of %d species observed · %d fully studied"),
        KnownCount, Definitions.Num(), StudiedCount)));
}

void UAstrawildJournalScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleJournalScreen();
    }
}
