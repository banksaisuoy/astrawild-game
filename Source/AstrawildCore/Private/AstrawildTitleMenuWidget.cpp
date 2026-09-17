#include "AstrawildTitleMenuWidget.h"

#include "AstrawildCore.h"
#include "AstrawildPlayerController.h"
#include "AstrawildSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
    constexpr float TitlePanelWidth = 460.0f;
    constexpr float TitleButtonHeight = 46.0f;

    // ASTRAWILD dawn palette (matches the HUD/pause styling).
    const FLinearColor PanelBg(0.02f, 0.05f, 0.08f, 0.88f);
    const FLinearColor TitleGold(0.98f, 0.92f, 0.75f, 1.0f);
    const FLinearColor SubtitleCyan(0.62f, 0.86f, 0.92f, 1.0f);
    const FLinearColor FooterGray(0.55f, 0.62f, 0.68f, 1.0f);
    const FLinearColor ButtonNew(0.18f, 0.42f, 0.38f, 1.0f);
    const FLinearColor ButtonContinue(0.2f, 0.3f, 0.5f, 1.0f);
    const FLinearColor ButtonControls(0.28f, 0.28f, 0.34f, 1.0f);
    const FLinearColor ButtonQuit(0.5f, 0.2f, 0.16f, 1.0f);
}

void UAstrawildTitleMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshContinueAvailability();
}

void UAstrawildTitleMenuWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void UAstrawildTitleMenuWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("TitleRoot"));
    RootCanvas = Canvas;

    // Fullscreen dark scrim so the 3D world behind reads as a backdrop.
    PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleScrim"));
    PanelBorder->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f));

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleMain"));
    TitleText->SetColorAndOpacity(FSlateColor(TitleGold));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 46));
    TitleText->SetText(FText::FromString(TEXT("ASTRAWILD")));
    TitleText->SetJustification(ETextJustify::Center);

    SubtitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleSub"));
    SubtitleText->SetColorAndOpacity(FSlateColor(SubtitleCyan));
    SubtitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 18));
    SubtitleText->SetText(FText::FromString(TEXT("Echoes of the First Dawn")));
    SubtitleText->SetJustification(ETextJustify::Center);

    FooterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleFooter"));
    FooterText->SetColorAndOpacity(FSlateColor(FooterGray));
    FooterText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 11));
    FooterText->SetText(FText::FromString(TEXT("Vertical Slice 0.1  |  a cooperative survival adventure prototype  |  UE 5.8")));
    FooterText->SetJustification(ETextJustify::Center);

    // --- Menu panel (bordered column with the four primary buttons) ---
    MenuPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleMenuPanel"));
    MenuPanel->SetBrushColor(PanelBg);
    MenuPanel->SetPadding(FMargin(24.0f, 28.0f, 24.0f, 24.0f));

    UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TitleMenuBox"));

    auto MakeMenuButton = [this](const FName& Name, const FString& Label, const FLinearColor& Color) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name.ToString());
        Button->SetBackgroundColor(Color);
        UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleButtonLabel"));
        ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 15));
        ButtonLabel->SetText(FText::FromString(Label));
        ButtonLabel->SetJustification(ETextJustify::Center);
        Button->AddChild(ButtonLabel);
        return Button;
    };

    NewGameButton = MakeMenuButton(TEXT("TitleNewGame"), TEXT("Begin New Journey"), ButtonNew);
    NewGameButton->OnClicked.AddDynamic(this, &UAstrawildTitleMenuWidget::HandleNewGameClicked);

    ContinueButton = MakeMenuButton(TEXT("TitleContinue"), TEXT("Continue"), ButtonContinue);
    ContinueButton->OnClicked.AddDynamic(this, &UAstrawildTitleMenuWidget::HandleContinueClicked);

    ControlsButton = MakeMenuButton(TEXT("TitleControls"), TEXT("Controls"), ButtonControls);
    ControlsButton->OnClicked.AddDynamic(this, &UAstrawildTitleMenuWidget::HandleControlsClicked);

    QuitButton = MakeMenuButton(TEXT("TitleQuit"), TEXT("Quit To Desktop"), ButtonQuit);
    QuitButton->OnClicked.AddDynamic(this, &UAstrawildTitleMenuWidget::HandleQuitClicked);

    auto AddMenuRow = [&MenuBox, this](UButton* Button)
    {
        if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(Button)))
        {
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            Slot->SetPadding(FMargin(0.0f, 7.0f));
        }
    };
    AddMenuRow(NewGameButton);
    AddMenuRow(ContinueButton);
    AddMenuRow(ControlsButton);
    AddMenuRow(QuitButton);
    MenuPanel->SetContent(MenuBox);

    // --- Controls panel (built lazily-shown; constructed up front) ---
    BuildControlsPanel();

    // --- Layout on the canvas ---
    if (UCanvasPanelSlot* ScrimSlot = Canvas->AddChildToCanvas(PanelBorder))
    {
        ScrimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        ScrimSlot->SetOffsets(FMargin(0.0f, 0.0f));
        ScrimSlot->SetAlignment(FVector2D::ZeroVector);
    }
    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-TitlePanelWidth * 0.5f, -260.0f));
        TitleSlot->SetSize(FVector2D(TitlePanelWidth, 60.0f));
    }
    if (UCanvasPanelSlot* SubtitleSlot = Canvas->AddChildToCanvas(SubtitleText))
    {
        SubtitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        SubtitleSlot->SetPosition(FVector2D(-TitlePanelWidth * 0.5f, -204.0f));
        SubtitleSlot->SetSize(FVector2D(TitlePanelWidth, 26.0f));
    }
    if (UCanvasPanelSlot* MenuSlot = Canvas->AddChildToCanvas(MenuPanel))
    {
        MenuSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        MenuSlot->SetPosition(FVector2D(-TitlePanelWidth * 0.5f, -150.0f));
        MenuSlot->SetSize(FVector2D(TitlePanelWidth, (TitleButtonHeight + 14.0f) * 4.0f + 52.0f));
    }
    if (ControlsPanel)
    {
        if (UCanvasPanelSlot* ControlsSlot = Canvas->AddChildToCanvas(ControlsPanel))
        {
            ControlsSlot->SetAnchors(FAnchors(0.5f, 0.5f));
            ControlsSlot->SetPosition(FVector2D(-260.0f, -220.0f));
            ControlsSlot->SetSize(FVector2D(520.0f, 440.0f));
        }
        ControlsPanel->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (UCanvasPanelSlot* FooterSlot = Canvas->AddChildToCanvas(FooterText))
    {
        FooterSlot->SetAnchors(FAnchors(0.5f, 1.0f));
        FooterSlot->SetPosition(FVector2D(-TitlePanelWidth * 0.5f, -46.0f));
        FooterSlot->SetSize(FVector2D(TitlePanelWidth, 20.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildTitleMenuWidget::BuildControlsPanel()
{
    ControlsPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TitleControlsPanel"));
    ControlsPanel->SetBrushColor(PanelBg);
    ControlsPanel->SetPadding(FMargin(26.0f, 22.0f, 26.0f, 18.0f));

    UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TitleControlsBox"));

    auto AddLine = [&Box, this](const FString& Action, const FString& Keys, const bool bHeaderLine = false)
    {
        UTextBlock* Line = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsLine"));
        if (bHeaderLine)
        {
            Line->SetColorAndOpacity(FSlateColor(TitleGold));
            Line->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 16));
        }
        else
        {
            Line->SetColorAndOpacity(FSlateColor(FLinearColor(0.88f, 0.93f, 0.96f, 1.0f)));
            Line->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 13));
        }
        Line->SetText(FText::FromString(bHeaderLine ? Action : FString::Printf(TEXT("  %-24s %s"), *Action, *Keys)));
        if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(Line)))
        {
            Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            Slot->SetPadding(FMargin(0.0f, bHeaderLine ? 10.0f : 2.0f));
        }
    };

    AddLine(TEXT("CONTROLS — KEYBOARD & MOUSE"), TEXT(""), true);
    AddLine(TEXT("Move / Look"), TEXT("WASD / Mouse"));
    AddLine(TEXT("Jump / Sprint"), TEXT("Space / Left Shift"));
    AddLine(TEXT("Interact / Talk / Gather"), TEXT("E"));
    AddLine(TEXT("Attack / Heavy Attack"), TEXT("Left Mouse / F"));
    AddLine(TEXT("Dodge / Block"), TEXT("Q / Right Mouse"));
    AddLine(TEXT("Command Echo / Feed Echo"), TEXT("C / R"));
    AddLine(TEXT("Build Mode / Rotate / Cycle"), TEXT("B / N / Mouse Wheel"));
    AddLine(TEXT("Dismantle Building"), TEXT("Z"));
    AddLine(TEXT("Consume / Equip Best"), TEXT("G / X"));
    AddLine(TEXT("Scan (hold) / Drone / Robot"), TEXT("V / H / J"));
    AddLine(TEXT("Inventory / Research"), TEXT("TAB / K"));
    AddLine(TEXT("Pause Menu"), TEXT("ESC"));
    AddLine(TEXT("Quick Save / Quick Load"), TEXT("F5 / F9"));
    AddLine(TEXT("GAMEPAD"), TEXT(""), true);
    AddLine(TEXT("A full gamepad mapping context is active"), TEXT(""));
    AddLine(TEXT("alongside keyboard & mouse (sticks + face"), TEXT(""));
    AddLine(TEXT("buttons mirror the bindings above)."), TEXT(""));

    ControlsBackButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ControlsBack"));
    ControlsBackButton->SetBackgroundColor(ButtonControls);
    UTextBlock* BackLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ControlsBackLabel"));
    BackLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    BackLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 14));
    BackLabel->SetText(FText::FromString(TEXT("Back")));
    ControlsBackButton->AddChild(BackLabel);
    ControlsBackButton->OnClicked.AddDynamic(this, &UAstrawildTitleMenuWidget::HandleBackFromControlsClicked);
    if (UVerticalBoxSlot* BackSlot = Cast<UVerticalBoxSlot>(Box->AddChildToVerticalBox(ControlsBackButton)))
    {
        BackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        BackSlot->SetPadding(FMargin(0.0f, 12.0f));
    }

    ControlsPanel->SetContent(Box);
}

void UAstrawildTitleMenuWidget::RefreshContinueAvailability()
{
    if (!ContinueButton)
    {
        return;
    }

    const UWorld* World = GetWorld();
    bool bAnySave = false;
    if (World && World->GetGameInstance())
    {
        if (const UAstrawildSaveSubsystem* SaveSubsystem = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
        {
            // LoadLatest resolves between the manual and autosave slots — the button
            // is live when either exists.
            bAnySave = SaveSubsystem->DoesSaveExist(TEXT("ASTRAWILD_Main"))
                || SaveSubsystem->DoesSaveExist(TEXT("ASTRAWILD_Auto"));
        }
    }

    ContinueButton->SetIsEnabled(bAnySave);
    if (UTextBlock* Label = Cast<UTextBlock>(ContinueButton->GetChildAt(0)))
    {
        Label->SetColorAndOpacity(FSlateColor(bAnySave
            ? FLinearColor::White
            : FLinearColor(0.45f, 0.48f, 0.5f, 0.55f)));
        Label->SetText(FText::FromString(bAnySave ? TEXT("Continue") : TEXT("Continue (no save)")));
    }
}

void UAstrawildTitleMenuWidget::HandleNewGameClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->DismissTitleScreen(false);
    }
}

void UAstrawildTitleMenuWidget::HandleContinueClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->DismissTitleScreen(true);
    }
}

void UAstrawildTitleMenuWidget::HandleControlsClicked()
{
    if (ControlsPanel && MenuPanel)
    {
        MenuPanel->SetVisibility(ESlateVisibility::Collapsed);
        if (TitleText)
        {
            TitleText->SetVisibility(ESlateVisibility::Collapsed);
        }
        if (SubtitleText)
        {
            SubtitleText->SetVisibility(ESlateVisibility::Collapsed);
        }
        ControlsPanel->SetVisibility(ESlateVisibility::Visible);
    }
}

void UAstrawildTitleMenuWidget::HandleBackFromControlsClicked()
{
    if (ControlsPanel && MenuPanel)
    {
        ControlsPanel->SetVisibility(ESlateVisibility::Collapsed);
        if (TitleText)
        {
            TitleText->SetVisibility(ESlateVisibility::Visible);
        }
        if (SubtitleText)
        {
            SubtitleText->SetVisibility(ESlateVisibility::Visible);
        }
        MenuPanel->SetVisibility(ESlateVisibility::Visible);
    }
}

void UAstrawildTitleMenuWidget::HandleQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
