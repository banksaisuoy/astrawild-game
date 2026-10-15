#include "AstrawildDeathOverlayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

namespace
{
    const FLinearColor DeathRed(0.85f, 0.16f, 0.1f, 1.0f);
    const FLinearColor DeathBody(0.95f, 0.85f, 0.82f, 1.0f);
}

void UAstrawildDeathOverlayWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
}

void UAstrawildDeathOverlayWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (RemainingSeconds > 0.0f)
    {
        RemainingSeconds = FMath::Max(0.0f, RemainingSeconds - InDeltaTime);
        const int32 Whole = FMath::CeilToInt(RemainingSeconds);
        if (Whole != LastWholeSecond)
        {
            LastWholeSecond = Whole;
            if (CountdownText)
            {
                CountdownText->SetText(FText::FromString(FString::Printf(
                    TEXT("Returning to camp in %d..."), Whole)));
            }
        }
    }
}

void UAstrawildDeathOverlayWidget::StartCountdown(const float RespawnSeconds)
{
    RemainingSeconds = FMath::Max(0.0f, RespawnSeconds);
    LastWholeSecond = -1;
    if (CountdownText)
    {
        CountdownText->SetText(FText::FromString(FString::Printf(
            TEXT("Returning to camp in %d..."), FMath::CeilToInt(RemainingSeconds))));
    }
}

void UAstrawildDeathOverlayWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("DeathRoot"));
    RootCanvas = Canvas;

    // Blood-warm scrim that dims the world behind the text.
    ScrimBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DeathScrim"));
    ScrimBorder->SetBrushColor(FLinearColor(0.12f, 0.01f, 0.0f, 0.45f));
    if (UCanvasPanelSlot* ScrimSlot = Canvas->AddChildToCanvas(ScrimBorder))
    {
        ScrimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        ScrimSlot->SetOffsets(FMargin(0.0f, 0.0f));
        ScrimSlot->SetAlignment(FVector2D::ZeroVector);
    }

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeathTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(DeathRed));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 38));
    TitleText->SetText(FText::FromString(TEXT("YOU FELL TO THE VALE")));
    TitleText->SetJustification(ETextJustify::Center);
    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.42f));
        TitleSlot->SetPosition(FVector2D(-320.0f, 0.0f));
        TitleSlot->SetSize(FVector2D(640.0f, 52.0f));
    }

    CountdownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeathCountdown"));
    CountdownText->SetColorAndOpacity(FSlateColor(DeathBody));
    CountdownText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 18));
    CountdownText->SetText(FText::FromString(TEXT("Returning to camp...")));
    CountdownText->SetJustification(ETextJustify::Center);
    if (UCanvasPanelSlot* CountdownSlot = Canvas->AddChildToCanvas(CountdownText))
    {
        CountdownSlot->SetAnchors(FAnchors(0.5f, 0.42f));
        CountdownSlot->SetPosition(FVector2D(-320.0f, 62.0f));
        CountdownSlot->SetSize(FVector2D(640.0f, 28.0f));
    }

    WidgetTree->RootWidget = Canvas;
}
