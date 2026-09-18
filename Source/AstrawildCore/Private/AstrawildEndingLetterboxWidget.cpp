#include "AstrawildEndingLetterboxWidget.h"

#include "AstrawildCore.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

void UAstrawildEndingLetterboxWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true); // Key events for the skip affordance.
    BuildOverlayTree();
}

void UAstrawildEndingLetterboxWidget::BuildOverlayTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return; // Already built (double-construct guard).
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("EndingOverlayRoot"));
    WidgetTree->RootWidget = RootCanvas;

    // Full-screen fade layer — alpha driven per phase by the sequence driver.
    FadeOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EndingFade"));
    FadeOverlay->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
    if (UCanvasPanelSlot* FadeSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(FadeOverlay)))
    {
        FadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        FadeSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
    }

    // Letterbox bars — anchored top/bottom, height driven by SetBarsProgress.
    TopBar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EndingTopBar"));
    TopBar->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
    TopBarSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(TopBar));
    if (TopBarSlot)
    {
        TopBarSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
        TopBarSlot->SetPosition(FVector2D(0.0f, 0.0f));
        TopBarSlot->SetSize(FVector2D(0.0f, 0.0f));
    }

    BottomBar = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EndingBottomBar"));
    BottomBar->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
    BottomBarSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(BottomBar));
    if (BottomBarSlot)
    {
        BottomBarSlot->SetAnchors(FAnchors(0.0f, 1.0f, 1.0f, 1.0f));
        BottomBarSlot->SetPosition(FVector2D(0.0f, 0.0f));
        BottomBarSlot->SetSize(FVector2D(0.0f, 0.0f));
    }

    const auto MakeText = [this](const FName Name, const FLinearColor& Color, const int32 FontSize) -> UTextBlock*
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name.ToString());
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), FontSize));
        Text->SetJustification(ETextJustify::Center);
        return Text;
    };

    // Ending title — centered in the lower third (classic credit frame).
    TitleText = MakeText(TEXT("EndingTitle"), FLinearColor(0.98f, 0.92f, 0.75f, 0.0f), 30);
    if (UCanvasPanelSlot* TitleSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(TitleText)))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.74f, 0.5f, 0.74f));
        TitleSlot->SetPosition(FVector2D(-540.0f, -20.0f));
        TitleSlot->SetSize(FVector2D(1080.0f, 44.0f));
    }

    SubtitleText = MakeText(TEXT("EndingSubtitle"), FLinearColor(0.92f, 0.90f, 0.82f, 0.0f), 15);
    if (UCanvasPanelSlot* SubSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(SubtitleText)))
    {
        SubSlot->SetAnchors(FAnchors(0.5f, 0.80f, 0.5f, 0.80f));
        SubSlot->SetPosition(FVector2D(-540.0f, 0.0f));
        SubSlot->SetSize(FVector2D(1080.0f, 26.0f));
    }

    SkipHintText = MakeText(TEXT("EndingSkipHint"), FLinearColor(0.55f, 0.60f, 0.58f, 0.0f), 12);
    SkipHintText->SetText(FText::FromString(TEXT("press any key to continue")));
    if (UCanvasPanelSlot* HintSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(SkipHintText)))
    {
        HintSlot->SetAnchors(FAnchors(0.5f, 0.90f, 0.5f, 0.90f));
        HintSlot->SetPosition(FVector2D(-160.0f, 0.0f));
        HintSlot->SetSize(FVector2D(320.0f, 18.0f));
    }
}

void UAstrawildEndingLetterboxWidget::SetEndingTexts(const FText& Title, const FText& Subtitle)
{
    if (TitleText)
    {
        TitleText->SetText(Title);
    }
    if (SubtitleText)
    {
        SubtitleText->SetText(Subtitle);
    }
}

void UAstrawildEndingLetterboxWidget::SetBarsProgress(const float Progress)
{
    const float Height = BarHeightUnits * FMath::Clamp(Progress, 0.0f, 1.0f);
    if (TopBarSlot)
    {
        TopBarSlot->SetPosition(FVector2D(0.0f, -Height)); // Slides down from above the frame.
        TopBarSlot->SetSize(FVector2D(0.0f, Height));
    }
    if (BottomBarSlot)
    {
        BottomBarSlot->SetPosition(FVector2D(0.0f, -Height)); // Anchored bottom: negative Y slides up from below.
        BottomBarSlot->SetSize(FVector2D(0.0f, Height));
    }
}

void UAstrawildEndingLetterboxWidget::SetFadeAlpha(const float Alpha)
{
    if (FadeOverlay)
    {
        FLinearColor Color = FadeOverlay->GetBrushColor();
        Color.A = FMath::Clamp(Alpha, 0.0f, 1.0f);
        FadeOverlay->SetBrushColor(Color);
    }
}

void UAstrawildEndingLetterboxWidget::SetTitleAlpha(const float Alpha)
{
    if (TitleText)
    {
        FLinearColor Color = TitleText->GetColorAndOpacity().GetColor(FLinearColor::White);
        Color.A = FMath::Clamp(Alpha, 0.0f, 1.0f);
        TitleText->SetColorAndOpacity(FSlateColor(Color));
    }
}

void UAstrawildEndingLetterboxWidget::SetSubtitleAlpha(const float Alpha)
{
    if (SubtitleText)
    {
        FLinearColor Color = SubtitleText->GetColorAndOpacity().GetColor(FLinearColor::White);
        Color.A = FMath::Clamp(Alpha, 0.0f, 1.0f);
        SubtitleText->SetColorAndOpacity(FSlateColor(Color));
    }
}

void UAstrawildEndingLetterboxWidget::SetHintVisible(const bool bVisible)
{
    if (SkipHintText)
    {
        FLinearColor Color = SkipHintText->GetColorAndOpacity().GetColor(FLinearColor::White);
        Color.A = bVisible ? 0.85f : 0.0f;
        SkipHintText->SetColorAndOpacity(FSlateColor(Color));
    }
}

FReply UAstrawildEndingLetterboxWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    OnSkipRequested.Broadcast();
    return FReply::Handled();
}

FReply UAstrawildEndingLetterboxWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    OnSkipRequested.Broadcast();
    return FReply::Handled();
}
