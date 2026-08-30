#include "AstrawildPauseMenuWidget.h"

#include "AstrawildCore.h"
#include "AstrawildPlayerController.h"
#include "AstrawildSaveSubsystem.h"
#include "Blueprint/WidgetTree.h"
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
    constexpr float PausePanelWidth = 340.0f;
    constexpr float PauseButtonHeight = 44.0f;
}

void UAstrawildPauseMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
}

void UAstrawildPauseMenuWidget::BuildWidgetTree()
{
    if (RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("PauseRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.92f, 0.75f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 22));
    TitleText->SetText(FText::FromString(TEXT("ASTRAWILD — Paused")));

    MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PauseMenu"));

    auto MakeMenuButton = [this](const FName& Name, const FString& Label, const FLinearColor& Color) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name.ToString());
        Button->SetBackgroundColor(Color);
        UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PauseButtonLabel"));
        ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 14));
        ButtonLabel->SetText(FText::FromString(Label));
        Button->AddChild(ButtonLabel);
        return Button;
    };

    ResumeButton = MakeMenuButton(TEXT("PauseResume"), TEXT("Resume"), FLinearColor(0.18f, 0.42f, 0.38f, 1.0f));
    ResumeButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleResumeClicked);

    SaveButton = MakeMenuButton(TEXT("PauseSave"), TEXT("Save Now"), FLinearColor(0.2f, 0.3f, 0.5f, 1.0f));
    SaveButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleSaveClicked);

    QuitButton = MakeMenuButton(TEXT("PauseQuit"), TEXT("Quit To Desktop"), FLinearColor(0.5f, 0.2f, 0.16f, 1.0f));
    QuitButton->OnClicked.AddDynamic(this, &UAstrawildPauseMenuWidget::HandleQuitClicked);

    if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(ResumeButton)))
    {
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        Slot->SetPadding(FMargin(0.0f, 6.0f));
    }
    if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(SaveButton)))
    {
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        Slot->SetPadding(FMargin(0.0f, 6.0f));
    }
    if (UVerticalBoxSlot* Slot = Cast<UVerticalBoxSlot>(MenuBox->AddChildToVerticalBox(QuitButton)))
    {
        Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        Slot->SetPadding(FMargin(0.0f, 6.0f));
    }

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(Anchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-PausePanelWidth * 0.5f, -140.0f));
        TitleSlot->SetSize(FVector2D(PausePanelWidth, 36.0f));
    }
    if (UCanvasPanelSlot* MenuSlot = Canvas->AddChildToCanvas(MenuBox))
    {
        MenuSlot->SetAnchors(Anchors(0.5f, 0.5f));
        MenuSlot->SetPosition(FVector2D(-PausePanelWidth * 0.5f, -90.0f));
        MenuSlot->SetSize(FVector2D(PausePanelWidth, (PauseButtonHeight + 12.0f) * 3.0f));
    }

    RootWidget = Canvas;
}

void UAstrawildPauseMenuWidget::HandleResumeClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->TogglePauseMenu();
    }
}

void UAstrawildPauseMenuWidget::HandleSaveClicked()
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
        PC->Notify(FText::FromString(TEXT("Saved.")));
    }
}

void UAstrawildPauseMenuWidget::HandleQuitClicked()
{
    // PIE ends the session; packaged builds exit to desktop (loop stage QUIT).
    UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
