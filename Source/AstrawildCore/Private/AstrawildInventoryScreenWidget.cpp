#include "AstrawildInventoryScreenWidget.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildSurvivalComponent.h"
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
    constexpr float InvPanelWidth = 620.0f;
    constexpr float InvPanelHeight = 560.0f;
    constexpr float InvRowHeight = 30.0f;
}

// ---------------------------------------------------------------------------
// Row widget
// ---------------------------------------------------------------------------

void UAstrawildInventoryRowWidget::InitializeRow(UAstrawildInventoryScreenWidget* ParentScreenPtr, const FName ItemId)
{
    ParentScreen = ParentScreenPtr;
    RowItemId = ItemId;

    if (RootWidget && !NameText)
    {
        BuildRowTree();
    }
}

void UAstrawildInventoryRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRowTree();
}

void UAstrawildInventoryRowWidget::BuildRowTree()
{
    if (RootWidget)
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(RowItemId) : nullptr;
    if (!ItemDef || !ParentScreen.IsValid())
    {
        return;
    }

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InvRow"));

    auto MakeText = [this](const FName& Name, const FLinearColor& Color, const int32 FontSize) -> UTextBlock*
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name.ToString());
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), FontSize));
        return Text;
    };

    // "Name ×Qty" + weight on the right side of the fill.
    const APawn* OwningPawn = ParentScreen->GetOwningPlayerPawn();
    const AAstrawildPlayerCharacter* Player = OwningPawn ? Cast<AAstrawildPlayerCharacter>(OwningPawn) : nullptr;
    const int32 Quantity = Player && Player->InventoryComponent ? Player->InventoryComponent->GetQuantity(RowItemId) : 0;
    const FString NameLine = FString::Printf(TEXT("%s  ×%d"), *ItemDef->DisplayName.ToString(), Quantity);

    NameText = MakeText(TEXT("RowName"), FLinearColor(0.95f, 0.93f, 0.85f, 1.0f), 13);
    NameText->SetText(FText::FromString(NameLine));

    UTextBlock* WeightText = MakeText(TEXT("RowWeight"), FLinearColor(0.6f, 0.62f, 0.66f, 1.0f), 12);
    WeightText->SetText(FText::FromString(FString::Printf(TEXT("%.1f kg"), ItemDef->Weight * Quantity)));

    // Action: Use for consumables, Equip for equipment.
    const bool bConsumable = ItemDef->Category == EAstrawildItemCategory::Consumable;
    const bool bEquipment = ItemDef->Category == EAstrawildItemCategory::Equipment;

    ActionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RowAction"));
    ActionButton->SetBackgroundColor(FLinearColor(0.16f, 0.38f, 0.45f, 1.0f));
    ActionButton->SetVisibility(bConsumable || bEquipment ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
    UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RowActionLabel"));
    ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    ButtonLabel->SetText(FText::FromString(bConsumable ? TEXT("Use") : TEXT("Equip")));
    ActionButton->AddChild(ButtonLabel);

    if (auto* NameSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(NameText)))
    {
        NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        NameSlot->SetVerticalAlignment(VAlign_Center);
    }
    if (auto* WeightSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(WeightText)))
    {
        WeightSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        WeightSlot->SetVerticalAlignment(VAlign_Center);
        WeightSlot->SetHorizontalAlignment(HAlign_Right);
    }
    if (auto* ButtonSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(ActionButton)))
    {
        ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ButtonSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (bConsumable || bEquipment)
    {
        ActionButton->OnClicked.AddDynamic(this, &UAstrawildInventoryRowWidget::HandleActionClicked);
    }

    RootWidget = Row;
}

void UAstrawildInventoryRowWidget::HandleActionClicked()
{
    APawn* OwningPawn = ParentScreen.IsValid() ? ParentScreen->GetOwningPlayerPawn() : nullptr;
    AAstrawildPlayerCharacter* Player = OwningPawn ? Cast<AAstrawildPlayerCharacter>(OwningPawn) : nullptr;
    if (!Player || !Player->InventoryComponent)
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(RowItemId) : nullptr;
    if (!ItemDef)
    {
        return;
    }

    if (ItemDef->Category == EAstrawildItemCategory::Consumable)
    {
        if (Player->InventoryComponent->RemoveItem(RowItemId, 1))
        {
            if (Player->SurvivalComponent)
            {
                Player->SurvivalComponent->ApplyConsumption(ItemDef->FoodValue, ItemDef->WaterValue, ItemDef->HealValue);
            }
        }
    }
    else if (ItemDef->Category == EAstrawildItemCategory::Equipment)
    {
        Player->InventoryComponent->EquipItem(RowItemId);
    }

    if (ParentScreen.IsValid())
    {
        ParentScreen->RefreshInventory();
    }
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------

void UAstrawildInventoryScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshInventory();
}

AAstrawildPlayerCharacter* UAstrawildInventoryScreenWidget::GetPlayerCharacter() const
{
    const APawn* OwningPawn = GetOwningPlayerPawn();
    return OwningPawn ? Cast<AAstrawildPlayerCharacter>(OwningPawn) : nullptr;
}

void UAstrawildInventoryScreenWidget::BuildWidgetTree()
{
    if (RootWidget)
    {
        return;
    }

    UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("InvRoot"));
    RootCanvas = Canvas;

    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InvTitle"));
    TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.92f, 0.75f, 1.0f)));
    TitleText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 20));
    TitleText->SetText(FText::FromString(TEXT("Pack")));

    WeightText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InvWeight"));
    WeightText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.85f, 0.8f, 1.0f)));
    WeightText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 14));

    LoadoutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InvLoadout"));
    LoadoutText->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.87f, 0.9f, 1.0f)));
    LoadoutText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));

    StackBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InvStacks"));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("InvClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.45f, 0.2f, 0.16f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InvCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    CloseLabel->SetText(FText::FromString(TEXT("Close [TAB]")));
    CloseButton->AddChild(CloseLabel);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildInventoryScreenWidget::HandleCloseClicked);

    // Layout: centered panel column.
    if (UCanvasPanelSlot* TitleSlot = Canvas->AddChildToCanvas(TitleText))
    {
        TitleSlot->SetAnchors(Anchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-InvPanelWidth * 0.5f, -InvPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(InvPanelWidth, 32.0f));
    }
    if (UCanvasPanelSlot* WeightSlot = Canvas->AddChildToCanvas(WeightText))
    {
        WeightSlot->SetAnchors(Anchors(0.5f, 0.5f));
        WeightSlot->SetPosition(FVector2D(-InvPanelWidth * 0.5f, -InvPanelHeight * 0.5f + 34.0f));
        WeightSlot->SetSize(FVector2D(InvPanelWidth, 24.0f));
    }
    if (UCanvasPanelSlot* LoadoutSlot = Canvas->AddChildToCanvas(LoadoutText))
    {
        LoadoutSlot->SetAnchors(Anchors(0.5f, 0.5f));
        LoadoutSlot->SetPosition(FVector2D(-InvPanelWidth * 0.5f, -InvPanelHeight * 0.5f + 62.0f));
        LoadoutSlot->SetSize(FVector2D(InvPanelWidth, 40.0f));
    }
    if (UCanvasPanelSlot* StacksSlot = Canvas->AddChildToCanvas(StackBox))
    {
        StacksSlot->SetAnchors(Anchors(0.5f, 0.5f));
        StacksSlot->SetPosition(FVector2D(-InvPanelWidth * 0.5f, -InvPanelHeight * 0.5f + 108.0f));
        StacksSlot->SetSize(FVector2D(InvPanelWidth, InvPanelHeight - 160.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(Anchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(InvPanelWidth * 0.5f - 130.0f, InvPanelHeight * 0.5f - 38.0f));
        CloseSlot->SetSize(FVector2D(130.0f, 32.0f));
    }

    RootWidget = Canvas;
}

void UAstrawildInventoryScreenWidget::RefreshInventory()
{
    if (!StackBox)
    {
        return;
    }

    AAstrawildPlayerCharacter* Player = GetPlayerCharacter();
    if (!Player || !Player->InventoryComponent)
    {
        return;
    }

    // Weight line.
    WeightText->SetText(FText::FromString(FString::Printf(TEXT("%.1f / %.0f kg"),
        Player->InventoryComponent->GetCurrentWeight(),
        Player->InventoryComponent->GetEffectiveMaxWeight())));

    // Loadout line (all six slots).
    auto SlotName = [](const FName Id) -> FString { return Id.IsNone() ? TEXT("—") : Id.ToString(); };
    LoadoutText->SetText(FText::FromString(FString::Printf(
        TEXT("Weapon: %s  ·  Shield: %s  ·  Armor: %s\nHelmet: %s  ·  Exosuit: %s  ·  Scanner: %s"),
        *SlotName(Player->InventoryComponent->EquippedItemId),
        *SlotName(Player->InventoryComponent->EquippedShieldItemId),
        *SlotName(Player->InventoryComponent->EquippedArmorItemId),
        *SlotName(Player->InventoryComponent->EquippedHelmetItemId),
        *SlotName(Player->InventoryComponent->EquippedExosuitItemId),
        *SlotName(Player->InventoryComponent->EquippedScannerItemId))));

    // Stacks.
    StackBox->ClearChildren();
    const TArray<FAstrawildItemStack> Stacks = Player->InventoryComponent->GetItemStacks();
    for (const FAstrawildItemStack& Stack : Stacks)
    {
        UAstrawildInventoryRowWidget* Row = WidgetTree->ConstructWidget<UAstrawildInventoryRowWidget>(
            UAstrawildInventoryRowWidget::StaticClass(), TEXT("InvRow"));
        Row->InitializeRow(this, Stack.ItemId);
        if (UVerticalBoxSlot* RowSlot = StackBox->AddChildToVerticalBox(Row))
        {
            RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            RowSlot->SetPadding(FMargin(4.0f, (InvRowHeight - 22.0f) * 0.5f, 4.0f, 0.0f));
        }
    }

    if (Stacks.IsEmpty())
    {
        UTextBlock* Empty = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InvEmpty"));
        Empty->SetColorAndOpacity(FSlateColor(FLinearColor(0.5f, 0.52f, 0.55f, 1.0f)));
        Empty->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 13));
        Empty->SetText(FText::FromString(TEXT("Pack is empty — go gather something.")));
        StackBox->AddChildToVerticalBox(Empty);
    }
}

void UAstrawildInventoryScreenWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->ToggleInventoryScreen();
    }
}
