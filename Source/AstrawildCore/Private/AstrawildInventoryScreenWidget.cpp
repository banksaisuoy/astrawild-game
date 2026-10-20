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
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    constexpr float InvPanelWidth = 620.0f;
    // CGC-4: the panel grew for the details line + the taller scrollable rows
    // region (the old fixed 400px stack slot silently clipped everything past
    // 13 stacks and was UNSCROLLABLE — audit 2-c finding).
    constexpr float InvPanelHeight = 780.0f;
    constexpr float InvRowHeight = 30.0f;
    // CGC-4: the discard double-click window (second click must land inside).
    constexpr double DiscardConfirmWindowSeconds = 3.0;
}

// ---------------------------------------------------------------------------
// CGC-4: pure static contracts (world-free — test 138 pins them)
// ---------------------------------------------------------------------------

FString UAstrawildInventoryScreenWidget::FormatItemDetails(const UAstrawildItemDefinition* ItemDef)
{
    // Fail-closed: a null/missing definition renders NOTHING (the caller's
    // details line clears — never a raw id or a lie).
    if (!ItemDef)
    {
        return FString();
    }

    FString Lines;
    if (!ItemDef->DisplayName.IsEmpty())
    {
        Lines = ItemDef->DisplayName.ToString();
    }
    if (!ItemDef->Description.IsEmpty())
    {
        if (!Lines.IsEmpty())
        {
            Lines += TEXT("\n");
        }
        Lines += ItemDef->Description.ToString();
    }

    // Stats line: ONLY the non-zero fields (the REAL ItemDefinition field
    // names — Food/Water/Heal/Weight/VendorPrice). A zero-stat material
    // renders no consumption/economy tokens at all (honest, not padded).
    FString Stats;
    auto AppendStat = [&Stats](const FString& Entry)
    {
        if (!Entry.IsEmpty())
        {
            if (!Stats.IsEmpty())
            {
                Stats += TEXT(" · ");
            }
            Stats += Entry;
        }
    };
    if (ItemDef->FoodValue > 0.0f)
    {
        AppendStat(FString::Printf(TEXT("Food +%.0f"), ItemDef->FoodValue));
    }
    if (ItemDef->WaterValue > 0.0f)
    {
        AppendStat(FString::Printf(TEXT("Water +%.0f"), ItemDef->WaterValue));
    }
    if (ItemDef->HealValue > 0.0f)
    {
        AppendStat(FString::Printf(TEXT("Heals +%.0f"), ItemDef->HealValue));
    }
    if (ItemDef->Weight > 0.0f)
    {
        AppendStat(FString::Printf(TEXT("%.1f kg each"), ItemDef->Weight));
    }
    if (ItemDef->VendorPrice > 0)
    {
        AppendStat(FString::Printf(TEXT("Sells for %d"), ItemDef->VendorPrice));
    }
    if (!Stats.IsEmpty())
    {
        if (!Lines.IsEmpty())
        {
            Lines += TEXT("\n");
        }
        Lines += Stats;
    }
    return Lines;
}

bool UAstrawildInventoryScreenWidget::SortStacksByName(TArray<FAstrawildItemStack>& Stacks,
    const UAstrawildItemRegistrySubsystem* Registry)
{
    // PRESENTATION sort only — callers pass a COPY of GetItemStacks(); the
    // underlying inventory array order is untouched (documented contract).
    // StableSort keeps equal-name rows in their original relative order.
    auto DisplayNameOf = [Registry](const FAstrawildItemStack& Stack) -> FString
    {
        const UAstrawildItemDefinition* Def = Registry ? Registry->FindItem(Stack.ItemId) : nullptr;
        return Def && !Def->DisplayName.IsEmpty()
            ? Def->DisplayName.ToString()
            : Stack.ItemId.ToString();
    };

    Stacks.StableSort([&DisplayNameOf](const FAstrawildItemStack& A, const FAstrawildItemStack& B)
    {
        // Case-insensitive: "a" sorts before "B".
        return DisplayNameOf(A).Compare(DisplayNameOf(B), ESearchCase::IgnoreCase) < 0;
    });

    return Registry != nullptr; // false = the id-string fallback was used.
}

// ---------------------------------------------------------------------------
// Row widget
// ---------------------------------------------------------------------------

void UAstrawildInventoryRowWidget::InitializeRow(UAstrawildInventoryScreenWidget* ParentScreenPtr, const FName ItemId)
{
    ParentScreen = ParentScreenPtr;
    RowItemId = ItemId;

    if (WidgetTree && WidgetTree->RootWidget && !NameText)
    {
        BuildRowTree();
    }
}

void UAstrawildInventoryRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRowTree();
}

bool UAstrawildInventoryRowWidget::IsEquippedItem() const
{
    // The honest equipped-read: any of the six replicated slot ids matching
    // this row means the item is in active use. (The smallest honest discard
    // behavior is to SKIP the button — dropping an equipped stack would leave
    // a dangling equipped id, and Unequip() clears ALL slots, which would be a
    // far more destructive side effect than the feature asks for.)
    APawn* OwningPawn = ParentScreen ? ParentScreen->GetOwningPlayerPawn() : nullptr;
    const AAstrawildPlayerCharacter* Player = OwningPawn ? Cast<AAstrawildPlayerCharacter>(OwningPawn) : nullptr;
    const UAstrawildInventoryComponent* Inventory = Player ? Player->InventoryComponent : nullptr;
    if (!Inventory)
    {
        return false;
    }
    return Inventory->EquippedItemId == RowItemId
        || Inventory->EquippedShieldItemId == RowItemId
        || Inventory->EquippedArmorItemId == RowItemId
        || Inventory->EquippedHelmetItemId == RowItemId
        || Inventory->EquippedExosuitItemId == RowItemId
        || Inventory->EquippedScannerItemId == RowItemId;
}

void UAstrawildInventoryRowWidget::BuildRowTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(RowItemId) : nullptr;
    if (!ItemDef || !ParentScreen)
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

    auto MakeRowButton = [this](const FName& Name, const TCHAR* Label, const FLinearColor& Color) -> UButton*
    {
        UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *Name.ToString());
        Button->SetBackgroundColor(Color);
        UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InvRowButtonLabel"));
        ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
        ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
        ButtonLabel->SetText(FText::FromString(Label));
        Button->AddChild(ButtonLabel);
        return Button;
    };

    ActionButton = MakeRowButton(TEXT("RowAction"), bConsumable ? TEXT("Use") : TEXT("Equip"), FLinearColor(0.16f, 0.38f, 0.45f, 1.0f));
    ActionButton->SetVisibility(bConsumable || bEquipment ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

    // CGC-4: the details button ("?") — every row gets one; it fills the
    // screen's details line from the item definition.
    DetailsButton = MakeRowButton(TEXT("RowDetails"), TEXT("?"), FLinearColor(0.3f, 0.34f, 0.42f, 1.0f));

    // CGC-4: the discard button — EVERY row except an equipped stack (the
    // overweight-junk trap escape: materials finally have an out).
    DiscardButton = MakeRowButton(TEXT("RowDiscard"), TEXT("Discard"), FLinearColor(0.5f, 0.24f, 0.18f, 1.0f));
    if (UTextBlock* ExistingLabel = Cast<UTextBlock>(DiscardButton->GetChildAt(0)))
    {
        DiscardButtonLabel = ExistingLabel;
    }
    DiscardButton->SetVisibility(IsEquippedItem() ? ESlateVisibility::Hidden : ESlateVisibility::Visible);

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
    if (auto* DetailsSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(DetailsButton)))
    {
        DetailsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        DetailsSlot->SetVerticalAlignment(VAlign_Center);
    }
    if (auto* DiscardSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(DiscardButton)))
    {
        DiscardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        DiscardSlot->SetVerticalAlignment(VAlign_Center);
    }

    if (bConsumable || bEquipment)
    {
        ActionButton->OnClicked.AddDynamic(this, &UAstrawildInventoryRowWidget::HandleActionClicked);
    }
    DetailsButton->OnClicked.AddDynamic(this, &UAstrawildInventoryRowWidget::HandleDetailsClicked);
    if (!IsEquippedItem())
    {
        DiscardButton->OnClicked.AddDynamic(this, &UAstrawildInventoryRowWidget::HandleDiscardClicked);
    }

    WidgetTree->RootWidget = Row;
}

void UAstrawildInventoryRowWidget::HandleActionClicked()
{
    APawn* OwningPawn = ParentScreen ? ParentScreen->GetOwningPlayerPawn() : nullptr;
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
            Player->ApplyFieldConsumableEffects(ItemDef); // DP-6: timed field verbs (status payload / capture focus).
        }
    }
    else if (ItemDef->Category == EAstrawildItemCategory::Equipment)
    {
        Player->InventoryComponent->EquipItem(RowItemId);
    }

    if (ParentScreen)
    {
        ParentScreen->RefreshInventory();
    }
}

void UAstrawildInventoryRowWidget::HandleDetailsClicked()
{
    // CGC-4: fill the screen's details line — fail-closed inside (a missing
    // definition clears the line, never invents text).
    if (ParentScreen)
    {
        ParentScreen->ShowItemDetails(RowItemId);
    }
}

void UAstrawildInventoryRowWidget::HandleDiscardClicked()
{
    // CGC-4 (discard-with-confirm): first click arms ("Discard N?" — the
    // pause-menu slot-label relabel idiom), a second click inside the 3s
    // window drops the WHOLE stack via RemoveItem(ItemId, StackCount). Any
    // refresh rebuilds the rows and resets every arm (stale confirms die with
    // the row they belonged to).
    APawn* OwningPawn = ParentScreen ? ParentScreen->GetOwningPlayerPawn() : nullptr;
    AAstrawildPlayerCharacter* Player = OwningPawn ? Cast<AAstrawildPlayerCharacter>(OwningPawn) : nullptr;
    if (!Player || !Player->InventoryComponent)
    {
        return;
    }

    // Destructive mutation: authority only (the QuickSave guard class — a
    // pure LAN client's local pack mirror must not desync; SaveWorld-style
    // fail-closed honesty for the trap escape).
    if (Player->GetLocalRole() != ROLE_Authority)
    {
        if (AAstrawildPlayerController* PC = ParentScreen->GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->Notify(FText::FromString(TEXT("Discard refused — the host authority owns the pack.")));
        }
        return;
    }

    const double NowSeconds = FPlatformTime::Seconds();
    const bool bArmedRecently = DiscardArmedAtSeconds > 0.0
        && (NowSeconds - DiscardArmedAtSeconds) <= DiscardConfirmWindowSeconds;
    if (!bArmedRecently)
    {
        DiscardArmedAtSeconds = NowSeconds;
        if (DiscardButtonLabel)
        {
            DiscardButtonLabel->SetText(FText::FromString(FString::Printf(TEXT("Discard %d?"),
                Player->InventoryComponent->GetQuantity(RowItemId))));
        }
        return;
    }

    // Confirmed inside the window: drop the whole stack (live quantity, not
    // the possibly-stale armed label).
    const int32 StackCount = Player->InventoryComponent->GetQuantity(RowItemId);
    DiscardArmedAtSeconds = 0.0;
    if (StackCount > 0 && Player->InventoryComponent->RemoveItem(RowItemId, StackCount))
    {
        if (AAstrawildPlayerController* PC = ParentScreen->GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->Notify(FText::FromString(FString::Printf(TEXT("Discarded ×%d."), StackCount)));
        }
    }

    if (ParentScreen)
    {
        ParentScreen->RefreshInventory(); // rebuild resets every row's arm state
    }
}

// ---------------------------------------------------------------------------
// Screen widget
// ---------------------------------------------------------------------------


UAstrawildInventoryScreenWidget::UAstrawildInventoryScreenWidget()
{
    // Final-audit F-05: focusable so TAB/ESC reach NativeOnKeyDown in UIOnly mode.
    bIsFocusable = true;
}

FReply UAstrawildInventoryScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // Final-audit F-05: the screen advertises "Close [TAB]" — make it true. ESC also
    // closes (the pause/dialogue convention).
    if (InKeyEvent.GetKey() == EKeys::Tab || InKeyEvent.GetKey() == EKeys::Escape)
    {
        if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
        {
            PC->ToggleInventoryScreen();
            return FReply::Handled();
        }
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAstrawildInventoryScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
    RefreshInventory();
}

AAstrawildPlayerCharacter* UAstrawildInventoryScreenWidget::GetPlayerCharacter() const
{
    APawn* OwningPawn = const_cast<UAstrawildInventoryScreenWidget*>(this)->GetOwningPlayerPawn();
    return OwningPawn ? Cast<AAstrawildPlayerCharacter>(OwningPawn) : nullptr;
}

void UAstrawildInventoryScreenWidget::BuildWidgetTree()
{
    if (WidgetTree && WidgetTree->RootWidget)
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

    // CGC-4: the Sort header button — a PRESENTATION toggle over a copy of
    // the stacks (the inventory array order itself is never touched).
    SortButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("InvSort"));
    SortButton->SetBackgroundColor(FLinearColor(0.28f, 0.36f, 0.46f, 1.0f));
    SortButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InvSortLabel"));
    SortButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    SortButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    SortButton->AddChild(SortButtonLabel);
    SortButton->OnClicked.AddDynamic(this, &UAstrawildInventoryScreenWidget::HandleSortClicked);

    // CGC-4: the stacks container now lives inside a ScrollBox (the
    // JournalScreen construction idiom, copied exactly — ConstructWidget +
    // AddChild wrap) with an always-visible scrollbar: >13 stacks overflow is
    // scrollable instead of silently clipped.
    StacksScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("InvStacksScroll"));
    StacksScroll->SetScrollbarVisibility(EScrollBoxScrollbarVisibility::AlwaysVisible);
    StackBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InvStacks"));
    StacksScroll->AddChild(StackBox);

    // CGC-4: the item details line (filled by a row's "?" button).
    DetailsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InvDetails"));
    DetailsText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.9f, 0.82f, 1.0f)));
    DetailsText->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), 12));
    DetailsText->SetAutoWrapText(true);

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
        TitleSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        TitleSlot->SetPosition(FVector2D(-InvPanelWidth * 0.5f, -InvPanelHeight * 0.5f));
        TitleSlot->SetSize(FVector2D(InvPanelWidth * 0.5f, 32.0f));
    }
    if (UCanvasPanelSlot* SortSlot = Canvas->AddChildToCanvas(SortButton))
    {
        SortSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        SortSlot->SetPosition(FVector2D(6.0f, -InvPanelHeight * 0.5f));
        SortSlot->SetSize(FVector2D(160.0f, 30.0f));
    }
    if (UCanvasPanelSlot* WeightSlot = Canvas->AddChildToCanvas(WeightText))
    {
        WeightSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        WeightSlot->SetPosition(FVector2D(-InvPanelWidth * 0.5f, -InvPanelHeight * 0.5f + 34.0f));
        WeightSlot->SetSize(FVector2D(InvPanelWidth, 24.0f));
    }
    if (UCanvasPanelSlot* LoadoutSlot = Canvas->AddChildToCanvas(LoadoutText))
    {
        LoadoutSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        LoadoutSlot->SetPosition(FVector2D(-InvPanelWidth * 0.5f, -InvPanelHeight * 0.5f + 62.0f));
        LoadoutSlot->SetSize(FVector2D(InvPanelWidth, 40.0f));
    }
    if (UCanvasPanelSlot* StacksSlot = Canvas->AddChildToCanvas(StacksScroll))
    {
        StacksSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        StacksSlot->SetPosition(FVector2D(-InvPanelWidth * 0.5f, -InvPanelHeight * 0.5f + 108.0f));
        // CGC-4: the rows region is now the tall SCROLL VIEWPORT (520px),
        // not the old fixed 400px clip.
        StacksSlot->SetSize(FVector2D(InvPanelWidth, 520.0f));
    }
    if (UCanvasPanelSlot* DetailsSlot = Canvas->AddChildToCanvas(DetailsText))
    {
        DetailsSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        DetailsSlot->SetPosition(FVector2D(-InvPanelWidth * 0.5f, InvPanelHeight * 0.5f - 150.0f));
        DetailsSlot->SetSize(FVector2D(InvPanelWidth, 100.0f));
    }
    if (UCanvasPanelSlot* CloseSlot = Canvas->AddChildToCanvas(CloseButton))
    {
        CloseSlot->SetAnchors(FAnchors(0.5f, 0.5f));
        CloseSlot->SetPosition(FVector2D(InvPanelWidth * 0.5f - 130.0f, InvPanelHeight * 0.5f - 38.0f));
        CloseSlot->SetSize(FVector2D(130.0f, 32.0f));
    }

    WidgetTree->RootWidget = Canvas;

    RefreshSortButtonLabel();
}

void UAstrawildInventoryScreenWidget::ShowItemDetails(const FName ItemId)
{
    if (!DetailsText)
    {
        return;
    }
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(ItemId) : nullptr;
    DetailsText->SetText(FText::FromString(FormatItemDetails(ItemDef)));
}

void UAstrawildInventoryScreenWidget::HandleSortClicked()
{
    // CGC-4: toggle the by-name presentation sort. Default OFF preserves the
    // inventory's insertion order (crafting refunds and gather arrivals stay
    // in the order they happened); one click reads A→Z, another restores.
    bSortByName = !bSortByName;
    RefreshSortButtonLabel();
    RefreshInventory();
}

void UAstrawildInventoryScreenWidget::RefreshSortButtonLabel()
{
    if (SortButtonLabel)
    {
        SortButtonLabel->SetText(FText::FromString(bSortByName
            ? TEXT("Sort: A→Z (on)")
            : TEXT("Sort: A→Z (off)")));
    }
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

    // Stacks — a COPY (the sort contract: the inventory array order itself is
    // never touched), optionally sorted by display name for presentation.
    TArray<FAstrawildItemStack> Stacks = Player->InventoryComponent->GetItemStacks();
    if (bSortByName)
    {
        UWorld* World = GetWorld();
        UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
        SortStacksByName(Stacks, Registry);
    }

    StackBox->ClearChildren();
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
