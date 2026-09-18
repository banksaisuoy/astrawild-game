#include "AstrawildResearchScreenWidget.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildPlayerController.h"
#include "AstrawildResearchSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    constexpr float TechPanelWidth = 680.0f;
    constexpr float TechPanelHeight = 560.0f;
}

// ---------------------------------------------------------------------------
// Row widget
// ---------------------------------------------------------------------------

void UAstrawildResearchRowWidget::InitializeRow(UAstrawildResearchScreenWidget* ParentScreenPtr, const FName TechId)
{
    ParentScreen = ParentScreenPtr;
    RowTechId = TechId;

    if (WidgetTree && WidgetTree->RootWidget && !NameText)
    {
        BuildRowTree();
    }
}

void UAstrawildResearchRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRowTree();
}

void UAstrawildResearchRowWidget::BuildRowTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    UAstrawildResearchSubsystem* Research = (World && World->GetGameInstance())
        ? World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>() : nullptr;
    const UAstrawildTechnologyDefinition* TechDef = Registry ? Registry->FindTechnology(RowTechId) : nullptr;
    if (!TechDef || !Research || !ParentScreen)
    {
        return;
    }

    const bool bUnlocked = Research->IsTechUnlocked(RowTechId);
    const TArray<FName> Missing = Research->GetMissingPrerequisites(RowTechId);
    const bool bAvailable = !bUnlocked && Missing.IsEmpty() && Research->GetResearchPoints() >= TechDef->ResearchCost;

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TechRow"));

    auto MakeText = [this](const FName& Name, const FLinearColor& Color, const int32 FontSize) -> UTextBlock*
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name.ToString());
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), FontSize));
        return Text;
    };

    // Name + state.
    FString StateLine;
    FLinearColor StateColor(0.6f, 0.62f, 0.66f, 1.0f);
    if (bUnlocked)
    {
        StateLine = TEXT("✓ Unlocked");
        StateColor = FLinearColor(0.45f, 0.85f, 0.55f, 1.0f);
    }
    else if (Missing.Num() > 0)
    {
        StateLine = FString::Printf(TEXT("Needs: %s"), *Missing[0].ToString());
    }
    else if (Research->GetResearchPoints() < TechDef->ResearchCost)
    {
        StateLine = TEXT("Not enough RP");
        StateColor = FLinearColor(0.9f, 0.6f, 0.35f, 1.0f);
    }
    else
    {
        StateLine = TEXT("Ready");
        StateColor = FLinearColor(0.98f, 0.85f, 0.5f, 1.0f);
    }

    NameText = MakeText(TEXT("TechName"), FLinearColor(0.95f, 0.93f, 0.85f, 1.0f), 13);
    NameText->SetText(FText::FromString(FString::Printf(TEXT("%s\n%s"), *TechDef->DisplayName.ToString(), *StateLine)));
    NameText->SetColorAndOpacity(FSlateColor(bUnlocked
        ? FLinearColor(0.45f, 0.55f, 0.5f, 1.0f)
        : FLinearColor(0.95f, 0.93f, 0.85f, 1.0f)));

    UTextBlock* EraText = MakeText(TEXT("TechEra"), StateColor, 12);
    EraText->SetText(FText::FromString(FString::Printf(TEXT("%s  %d RP"),
        *UEnum::GetDisplayValueAsText(TechDef->Era).ToString(), TechDef->ResearchCost)));

    UnlockButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TechUnlock"));
    UnlockButton->SetBackgroundColor(FLinearColor(0.42f, 0.3f, 0.55f, 1.0f));
    UnlockButton->SetVisibility(bAvailable ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TechUnlockLabel"));
    ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    ButtonLabel->SetText(FText::FromString(TEXT("Unlock")));
    UnlockButton->AddChild(ButtonLabel);

    if (bAvailable)
    {
        UnlockButton->OnClicked.AddDynamic(this, &UAstrawildResearchRowWidget::HandleUnlockClicked);
    }

    if (auto* NameSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(NameText)))
    {
        NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        NameSlot->SetVerticalAlignment(VAlign_Center);
    }
    if (auto* EraSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(EraText)))
    {
        EraSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        EraSlot->SetVerticalAlignment(VAlign_Center);
        EraSlot->SetHorizontalAlignment(HAlign_Right);
    }
    if (auto* ButtonSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(UnlockButton)))
    {
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
    }

    WidgetTree->RootWidget = Row;
}

void UAstrawildResearchRowWidget::HandleUnlockClicked()
{
    UWorld* World = GetWorld();
    if (!World || !World->GetGameInstance())
    {
        return;
    }

    if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
    {
        Research->TryUnlockTech(RowTechId);
    }

    if (ParentScreen)
    {
        ParentScreen->RefreshResearch();
    }
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------


UAstrawildResearchScreenWidget::UAstrawildResearchScreenWidget()
{
    // Final-audit F-05: focusable so K/ESC reach NativeOnKeyDown in UIOnly mode.
    bIsFocusable = true;
}

FReply UAstrawildResearchScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // Final-audit F-05: the screen advertises "Close [K]" — make it true.
    if (InKeyEvent.GetKey() == EKeys::K || InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleResearchScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildResearchScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshResearch();
}

void UAstrawildResearchScreenWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ResearchRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResearchTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.72f, 0.98f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("Research")));

    PointsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResearchPoints"));
    PointsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.85f, 0.5f, 1.0f)));
    PointsText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14));

    TechBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TechList"));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ResearchClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResearchCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [K]")));
    CloseButton->AddChild(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildResearchScreenWidget::HandleCloseClicked);

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-TechPanelWidth * 0.5f, -TechPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(TechPanelWidth, 32.0f));
    }
    if (UCanvasPanelSlot* PointsSlot = Canvas->AddChildToCanvas(PointsText))
    {
        PointsSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        PointsSlot->SetPosition(FVector2D(-TechPanelWidth * 0.5f, -TechPanelHeight * 0.5f + 36.0f));
        PointsSlot->SetSize(FVector2D(TechPanelWidth, 24.0f));
    }
    if (UCanvasPanelSlot* ListSlot = Canvas->AddChildToCanvas(TechBox))
    {
        ListSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        ListSlot->SetPosition(FVector2D(-TechPanelWidth * 0.5f, -TechPanelHeight * 0.5f + 68.0f));
        ListSlot->SetSize(FVector2D(TechPanelWidth, TechPanelHeight - 120.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(TechPanelWidth * 0.5f - 130.0f, TechPanelHeight * 0.5f - 38.0f));
        CloseSlot->SetSize(FVector2D(130.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildResearchScreenWidget::RefreshResearch()
{
    if (!TechBox)
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    UAstrawildResearchSubsystem* Research = (World && World->GetGameInstance())
        ? World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>() : nullptr;
    if (!Registry || !Research)
    {
        return;
    }

    PointsText->SetText(FText::FromString(FString::Printf(TEXT("%d Research Points"), Research->GetResearchPoints())));

    TechBox->ClearChildren();
    const TArray<UAstrawildTechnologyDefinition*> Techs = Registry->GetAllTechnologies();
    for (const UAstrawildTechnologyDefinition* Tech : Techs)
    {
        if (!Tech)
        {
            continue;
        }

        UAstrawildResearchRowWidget* Row = WidgetTree->ConstructWidget<UAstrawildResearchRowWidget>(
            UAstrawildResearchRowWidget::StaticClass(), TEXT("TechRow"));
        Row->InitializeRow(this, Tech->TechId);
        if (UVerticalBoxSlot* RowSlot = TechBox->AddChildToVerticalBox(Row))
        {
            RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            RowSlot->SetPadding(FMargin(4.0f, 3.0f, 4.0f, 0.0f));
        }
    }
}

void UAstrawildResearchScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleResearchScreen();
    }
}
