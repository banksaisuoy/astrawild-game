#include "AstrawildVictoryScreenWidget.h"

#include "AstrawildCore.h"
#include "AstrawildPlayerController.h"
#include "AstrawildSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"

namespace
{
    constexpr float VictoryPanelWidth = 480.0f;
    constexpr float VictoryButtonHeight = 44.0f;

    const FLinearColor VictoryBg(0.03f, 0.07f, 0.05f, 0.92f);
    const FLinearColor VictoryGold(1.0f, 0.88f, 0.55f, 1.0f);
    const FLinearColor VictoryBody(0.9f, 0.95f, 0.93f, 1.0f);
}

void UAstrawildVictoryScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshStatsText();
}

void UAstrawildVictoryScreenWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("VictoryRoot"));
    RootCanvas = Canvas;

    // Golden scrim — dawn light over the world.
    UBorder* Scrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VictoryScrim"));
    Scrim->SetBrushColor(FLinearColor(0.08f, 0.06f, 0.02f, 0.5f));
    if (UCanvasPanelSlot* ScrimSlot = Canvas->AddChildToCanvas(Scrim))
    {
        ScrimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        ScrimSlot->SetOffsets(FMargin(0.0f, 0.0f));
        ScrimSlot->SetAlignment(FVector2D::ZeroVector);
    }

    PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("VictoryPanel"));
    PanelBorder->SetBrushColor(VictoryBg);
    PanelBorder->SetPadding(FMargin(30.0f, 30.0f, 30.0f, 26.0f));

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VictoryBox"));

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VictoryTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(VictoryGold));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 34));
    TitleText->SetText(FText::FromString(TEXT("THE FIRST DAWN AWAKENS")));
    TitleText->SetJustification(ETextJustify::Center);

    SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VictorySubtitle"));
    SubtitleText->SetColorAndOpacity(FSlateColor(VictoryBody));
    SubtitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 15));
    SubtitleText->SetText(FText::FromString(TEXT("Your signal crossed the Vale — the wilderness answers as an ally now.\nThe main chain is complete; the free hunt begins.")));
    SubtitleText->SetJustification(ETextJustify::Center);

    StatsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VictoryStats"));
    StatsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.9f, 0.82f, 1.0f)));
    StatsText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14));
    StatsText->SetJustification(ETextJustify::Center);

    auto MakeButton = [this](const FName& Name, const FString& Label, const FLinearColor& Color) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name.ToString());
        Button->SetBackgroundColor(Color);
        UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VictoryButtonLabel"));
        ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 14));
        ButtonLabel->SetText(FText::FromString(Label));
        ButtonLabel->SetJustification(ETextJustify::Center);
        Button->AddChild(ButtonLabel);
        return Button;
    };

    KeepExploringButton = MakeButton(TEXT("VictoryKeepExploring"), TEXT("Keep Exploring"), FLinearColor(0.18f, 0.42f, 0.38f, 1.0f));
    KeepExploringButton->OnClicked.AddDynamic(this, &UAstrawildVictoryScreenWidget::HandleKeepExploringClicked);

    SaveButton = MakeButton(TEXT("VictorySave"), TEXT("Save Journey"), FLinearColor(0.2f, 0.3f, 0.5f, 1.0f));
    SaveButton->OnClicked.AddDynamic(this, &UAstrawildVictoryScreenWidget::HandleSaveClicked);

    auto AddRow = [&Box, this](UWidget* Widget, const float Padding)
    {
        if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Widget)))
        {
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            Slot->SetPadding(FMargin(0.0f, Padding));
            Slot->SetHorizontalAlignment(HAlign_Fill);
        }
    };
    AddRow(TitleText, 0.0f);
    AddRow(SubtitleText, 14.0f);
    AddRow(StatsText, 20.0f);
    AddRow(KeepExploringButton, 26.0f);
    AddRow(SaveButton, 10.0f);

    PanelBorder->SetContent(Box);

    if (UCanvasPanelSlot* PanelSlot = Canvas->AddChildToCanvas(PanelBorder))
    {
        PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        PanelSlot->SetPosition(FVector2D(-VictoryPanelWidth * 0.5f, -190.0f));
        PanelSlot->SetSize(FVector2D(VictoryPanelWidth, (VictoryButtonHeight + 18.0f) * 2.0f + 240.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildVictoryScreenWidget::InitializeVictory(const int32 InDaysSurvived, const int32 InEchoesBonded, const int32 InJournalEntries)
{
    DaysSurvived = InDaysSurvived;
    EchoesBonded = InEchoesBonded;
    JournalEntries = InJournalEntries;
    RefreshStatsText();
}

void UAstrawildVictoryScreenWidget::RefreshStatsText()
{
    if (StatsText)
    {
        StatsText->SetText(FText::FromString(FString::Printf(
            TEXT("Days survived      %d\nEchoes bonded      %d\nJournal observed   %d species"),
            DaysSurvived, EchoesBonded, JournalEntries)));
    }
}

void UAstrawildVictoryScreenWidget::HandleKeepExploringClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->HideVictoryScreen();
    }
}

void UAstrawildVictoryScreenWidget::HandleSaveClicked()
{
    UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildSaveSubsystem* SaveSubsystem = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
        {
            SaveSubsystem->SaveWorld(World, TEXT("ASTRAWILD_Main"));
        }
    }

    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->Notify(FText::FromString(TEXT("Journey saved.")));
    }
}
