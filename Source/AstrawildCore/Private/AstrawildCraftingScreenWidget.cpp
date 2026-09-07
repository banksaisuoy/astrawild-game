#include "AstrawildCraftingScreenWidget.h"

#include "AstrawildPlayerController.h"

#include "AstrawildCraftingComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
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
#include "GameFramework/Pawn.h"

namespace
{
    constexpr float CraftPanelWidth = 780.0f;
    constexpr float CraftPanelHeight = 640.0f;

    /** "Station_Workbench" -> "Workbench" (player-readable station label). */
    FString StationLabel(const FName StationId)
    {
        FString Label = StationId.ToString();
        if (Label.StartsWith(TEXT("Station_")))
        {
            Label = Label.RightChop(8);
        }
        return Label.IsEmpty() ? FString(TEXT("Station")) : Label;
    }
}

// ---------------------------------------------------------------------------
// FPP-1: recipe row
// ---------------------------------------------------------------------------

void UAstrawildCraftingRowWidget::InitializeRow(UAstrawildCraftingScreenWidget* ParentScreenPtr,
    const UAstrawildRecipeDefinition* Recipe, const bool bStationNearby, const bool bCraftableNow)
{
    ParentScreen = ParentScreenPtr;
    RowRecipeId = Recipe ? Recipe->RecipeId : NAME_None;

    if (Recipe)
    {
        bRowCraftable = bCraftableNow;
        bRowStationNearby = bStationNearby;
        RowDisplayName = Recipe->DisplayName.ToString();
        RowInputs = Recipe->Ingredients;
        RowOutputs = Recipe->Outputs;
        RowStationId = Recipe->RequiredStationId;
        RowCraftSeconds = Recipe->CraftDurationSeconds;
    }

    if (WidgetTree && WidgetTree->RootWidget && !RowText)
    {
        BuildRowTree();
    }
}

void UAstrawildCraftingRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRowTree();
}

void UAstrawildCraftingRowWidget::BuildRowTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!Registry || RowRecipeId.IsNone())
    {
        return;
    }

    // --- Row text: name, inputs (with have/need counts), station + time. ---
    FString Line = FString::Printf(TEXT("%s"), *RowDisplayName);

    if (RowInputs.Num() > 0)
    {
        FString InputLine;
        for (int32 i = 0; i < RowInputs.Num(); ++i)
        {
            if (i > 0)
            {
                InputLine += TEXT(" + ");
            }
            const FAstrawildItemStack& Input = RowInputs[i];
            const UAstrawildItemDefinition* Def = Registry->FindItem(Input.ItemId);
            const FString ItemLabel = Def ? Def->DisplayName.ToString() : Input.ItemId.ToString();
            // "2x Wood (you have 12)" — the shortage is visible BEFORE the click.
            InputLine += FString::Printf(TEXT("%dx %s"), FMath::Max(1, Input.Quantity), *ItemLabel);
        }
        Line += FString::Printf(TEXT("\nInputs: %s"), *InputLine);
    }

    if (RowOutputs.Num() > 0)
    {
        FString OutputLine;
        for (int32 i = 0; i < RowOutputs.Num(); ++i)
        {
            if (i > 0)
            {
                OutputLine += TEXT(" + ");
            }
            const FAstrawildItemStack& Output = RowOutputs[i];
            const UAstrawildItemDefinition* Def = Registry->FindItem(Output.ItemId);
            const FString ItemLabel = Def ? Def->DisplayName.ToString() : Output.ItemId.ToString();
            OutputLine += FString::Printf(TEXT("%dx %s"), FMath::Max(1, Output.Quantity), *ItemLabel);
        }
        Line += FString::Printf(TEXT("\nMakes: %s"), *OutputLine);
    }

    if (!RowStationId.IsNone())
    {
        Line += FString::Printf(TEXT("\nStation: %s%s"),
            *StationLabel(RowStationId),
            bRowStationNearby ? TEXT(" (in range)") : TEXT(" — stand closer to craft"));
    }
    if (RowCraftSeconds > 0.0f)
    {
        Line += FString::Printf(TEXT("\n%.1fs craft"), RowCraftSeconds);
    }

    RowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftRowText"));
    RowText->SetColorAndOpacity(FSlateColor(bRowCraftable
        ? FLinearColor(0.95f, 0.93f, 0.85f, 1.0f)
        : FLinearColor(0.62f, 0.64f, 0.68f, 1.0f)));
    RowText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    RowText->SetAutoWrapText(true);
    RowText->SetText(FText::FromString(Line));

    // --- Craft button: label carries the one-line reason when disabled. ---
    CraftButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CraftRowButton"));
    FString ButtonLabel;
    if (bRowCraftable)
    {
        CraftButton->SetBackgroundColor(FLinearColor(0.18f, 0.42f, 0.38f, 1.0f));
        ButtonLabel = TEXT("Craft");
    }
    else
    {
        CraftButton->SetBackgroundColor(FLinearColor(0.30f, 0.30f, 0.34f, 1.0f));
        ButtonLabel = !RowStationId.IsNone() && !bRowStationNearby ? TEXT("Station needed") : TEXT("Missing inputs");
    }
    UTextBlock* ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftRowButtonLabel"));
    ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ButtonText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    ButtonText->SetText(FText::FromString(ButtonLabel));
    CraftButton->AddChild(ButtonText);
    CraftButton->SetIsEnabled(bRowCraftable);
    if (bRowCraftable)
    {
        CraftButton->OnClicked.AddDynamic(this, &UAstrawildCraftingRowWidget::HandleCraftClicked);
    }

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CraftRow"));
    if (auto* TextSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(RowText)))
    {
        TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        TextSlot->SetVerticalAlignment(VAlign_Center);
    }
    if (auto* ButtonSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(CraftButton)))
    {
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
        ButtonSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
    }

    WidgetTree->RootWidget = Row;
}

void UAstrawildCraftingRowWidget::HandleCraftClicked()
{
    if (ParentScreen)
    {
        ParentScreen->RequestCraft(RowRecipeId);
        // The craft-started handler refreshes the whole list (ingredient counts
        // + the "Crafting..." state); no local mutation to keep both honest.
    }
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

UAstrawildCraftingScreenWidget::UAstrawildCraftingScreenWidget()
{
    // Final-audit F-05: focusable so ESC reaches NativeOnKeyDown in UIOnly mode.
    bIsFocusable = true;
}

FReply UAstrawildCraftingScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // Final-audit F-05: ESC closes the crafting screen (station E toggles it too).
    if (InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleCraftingScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildCraftingScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BuildNativeUi();
    BindCraftingComponent();

    if (UAstrawildCraftingComponent* Component = CraftingComponent.Get())
    {
        PopulateRecipeList(Component->GetTechUnlockedRecipes());
        BP_OnRecipesAvailable(Component->GetTechUnlockedRecipes()); // BP subclass contract kept.
    }
    RefreshStatusLine();
}

void UAstrawildCraftingScreenWidget::NativeDestruct()
{
    UnbindCraftingComponent();
    Super::NativeDestruct();
}

void UAstrawildCraftingScreenWidget::BuildNativeUi()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CraftRoot"));

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.9f, 0.95f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("Crafting")));

    StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftStatus"));
    StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.85f, 0.5f, 1.0f)));
    StatusText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14));
    StatusText->SetAutoWrapText(true);

    RecipeList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CraftRecipeList"));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CraftClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [E/ESC]")));
    CloseButton->AddChild(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildCraftingScreenWidget::HandleCloseClicked);

    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-CraftPanelWidth * 0.5f, -CraftPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(CraftPanelWidth, 32.0f));
    }
    if (UCanvasPanelSlot* StatusSlot = Canvas->AddChildToCanvas(StatusText))
    {
        StatusSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        StatusSlot->SetPosition(FVector2D(-CraftPanelWidth * 0.5f, -CraftPanelHeight * 0.5f + 36.0f));
        StatusSlot->SetSize(FVector2D(CraftPanelWidth, 24.0f));
    }
    if (UCanvasPanelSlot* ListSlot = Canvas->AddChildToCanvas(RecipeList))
    {
        ListSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        ListSlot->SetPosition(FVector2D(-CraftPanelWidth * 0.5f, -CraftPanelHeight * 0.5f + 68.0f));
        ListSlot->SetSize(FVector2D(CraftPanelWidth, CraftPanelHeight - 120.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(CraftPanelWidth * 0.5f - 140.0f, CraftPanelHeight * 0.5f - 38.0f));
        CloseSlot->SetSize(FVector2D(140.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;
}

void UAstrawildCraftingScreenWidget::PopulateRecipeList(const TArray<UAstrawildRecipeDefinition*>& Recipes)
{
    if (!RecipeList)
    {
        return;
    }

    UAstrawildCraftingComponent* Component = CraftingComponent.Get();
    RecipeList->ClearChildren();

    if (!Component)
    {
        UTextBlock* EmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftEmpty"));
        EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.64f, 0.68f, 1.0f)));
        EmptyText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 13));
        EmptyText->SetAutoWrapText(true);
        EmptyText->SetText(FText::FromString(
            TEXT("Crafting binds to your survivor — approach a Workbench or Campfire and press E.")));
        RecipeList->AddChild(EmptyText);
        return;
    }

    const TArray<FName> NearbyStations = Component->GetNearbyStationIds();

    int32 RowIndex = 0;
    for (const UAstrawildRecipeDefinition* Recipe : Recipes)
    {
        if (!Recipe)
        {
            continue;
        }
        const bool bStationNearby = Recipe->RequiredStationId.IsNone() || NearbyStations.Contains(Recipe->RequiredStationId);
        const bool bCraftable = !Component->IsCrafting() &&
            Component->CanCraftIgnoringStation(Recipe) && bStationNearby;

        UAstrawildCraftingRowWidget* RowWidget = WidgetTree->ConstructWidget<UAstrawildCraftingRowWidget>(
            UAstrawildCraftingRowWidget::StaticClass(), *FString::Printf(TEXT("CraftRow%d"), RowIndex++));
        RowWidget->InitializeRow(this, Recipe, bStationNearby, bCraftable);
        if (UScrollBoxSlot* RowSlot = RecipeList->AddChild(RowWidget))
        {
            RowSlot->SetPadding(FMargin(6.0f, 5.0f, 6.0f, 2.0f));
        }
    }
}

void UAstrawildCraftingScreenWidget::RefreshStatusLine()
{
    if (!StatusText)
    {
        return;
    }

    const UAstrawildCraftingComponent* Component = CraftingComponent.Get();
    if (Component && Component->IsCrafting())
    {
        StatusText->SetText(FText::FromString(FString::Printf(
            TEXT("Crafting... %.0fs left (%.0f%%)"),
            Component->GetCraftTimeRemaining(),
            Component->GetCraftingProgress() * 100.0f)));
    }
    else
    {
        StatusText->SetText(FText::FromString(
            TEXT("Pick a recipe — one craft at a time; timed crafts can be cancelled for a full refund.")));
    }
}

void UAstrawildCraftingScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleCraftingScreen();
    }
}

void UAstrawildCraftingScreenWidget::BindCraftingComponent()
{
    if (UAstrawildCraftingComponent* Existing = CraftingComponent.Get())
    {
        return; // Already bound.
    }

    const APawn* OwningPawn = GetOwningPlayerPawn();
    if (!OwningPawn)
    {
        return;
    }

    UAstrawildCraftingComponent* Component = OwningPawn->FindComponentByClass<UAstrawildCraftingComponent>();
    if (!Component)
    {
        return;
    }

    Component->OnCraftStarted.AddDynamic(this, &UAstrawildCraftingScreenWidget::HandleCraftStarted);
    Component->OnCraftProgress.AddDynamic(this, &UAstrawildCraftingScreenWidget::HandleCraftProgress);
    Component->OnCraftCompleted.AddDynamic(this, &UAstrawildCraftingScreenWidget::HandleCraftCompleted);
    Component->OnCraftCancelled.AddDynamic(this, &UAstrawildCraftingScreenWidget::HandleCraftCancelled);
    CraftingComponent = Component;

    UE_LOG(LogAstrawildEconomy, Log, TEXT("Crafting screen bound to pawn crafting component."));
}

void UAstrawildCraftingScreenWidget::UnbindCraftingComponent()
{
    if (UAstrawildCraftingComponent* Component = CraftingComponent.Get())
    {
        Component->OnCraftStarted.RemoveAll(this);
        Component->OnCraftProgress.RemoveAll(this);
        Component->OnCraftCompleted.RemoveAll(this);
        Component->OnCraftCancelled.RemoveAll(this);
    }
    CraftingComponent = nullptr;
}

bool UAstrawildCraftingScreenWidget::RequestCraft(const FName RecipeId)
{
    UAstrawildCraftingComponent* Component = CraftingComponent.Get();
    if (!Component)
    {
        BindCraftingComponent();
        Component = CraftingComponent.Get();
    }

    if (!Component)
    {
        return false;
    }

    Component->ServerRequestCraft(RecipeId);
    return true;
}

bool UAstrawildCraftingScreenWidget::RequestCancelCraft()
{
    UAstrawildCraftingComponent* Component = CraftingComponent.Get();
    if (!Component)
    {
        return false;
    }

    Component->ServerRequestCancelCraft();
    return true;
}

void UAstrawildCraftingScreenWidget::RefreshRecipes()
{
    if (UAstrawildCraftingComponent* Component = CraftingComponent.Get())
    {
        PopulateRecipeList(Component->GetTechUnlockedRecipes());
        BP_OnRecipesAvailable(Component->GetTechUnlockedRecipes()); // BP subclass contract kept.
    }
    RefreshStatusLine();
}

void UAstrawildCraftingScreenWidget::HandleCraftStarted(const FName RecipeId, const float DurationSeconds)
{
    RefreshRecipes();
    BP_OnCraftStarted(RecipeId, DurationSeconds);
}

void UAstrawildCraftingScreenWidget::HandleCraftProgress(const FName RecipeId, const float ProgressFraction)
{
    RefreshStatusLine();
    BP_OnCraftProgress(RecipeId, ProgressFraction);
}

void UAstrawildCraftingScreenWidget::HandleCraftCompleted(const FName RecipeId, const bool bSuccess)
{
    // FPP-1: the completion moment is visible on the screen (status + fresh
    // ingredient counts) — the same event drives the HUD toast in the
    // component itself so the player sees it even with the screen closed.
    RefreshRecipes();
    BP_OnCraftCompleted(RecipeId, bSuccess);
}

void UAstrawildCraftingScreenWidget::HandleCraftCancelled(const FName RecipeId, const bool bRefunded)
{
    RefreshRecipes();
    BP_OnCraftCancelled(RecipeId, bRefunded);
}
