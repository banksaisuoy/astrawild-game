#include "AstrawildShopWidget.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

namespace
{
    constexpr float ShopPanelWidth = 660.0f;
    constexpr float ShopPanelHeight = 560.0f;
    constexpr float RowHeight = 30.0f;

    /** Result → player-facing status line. */
    FString VendorResultText(const EAstrawildVendorResult Result)
    {
        switch (Result)
        {
        case EAstrawildVendorResult::Success:          return TEXT("Deal!");
        case EAstrawildVendorResult::NotAVendor:       return TEXT("This NPC has no shop.");
        case EAstrawildVendorResult::NotAWare:         return TEXT("Not tradeable here.");
        case EAstrawildVendorResult::NotEnoughCurrency:return TEXT("Not enough Dawn Shards.");
        case EAstrawildVendorResult::TooHeavy:         return TEXT("Too heavy to carry.");
        case EAstrawildVendorResult::TooFarAway:       return TEXT("Step closer to trade.");
        case EAstrawildVendorResult::InvalidRequest:
        default:                                       return TEXT("Invalid request.");
        }
    }
}

// ---------------------------------------------------------------------------
// Row widget
// ---------------------------------------------------------------------------
void UAstrawildShopRowWidget::InitializeRow(UAstrawildShopWidget* ParentShop, AAstrawildNPCCharacter* InVendor, const FName ItemId, const bool bInBuyRow)
{
    ParentShop = ParentShop;
    Vendor = InVendor;
    RowItemId = ItemId;
    bBuyRow = bInBuyRow;

    // Rows may be configured before they enter the live tree — build lazily in
    // NativeConstruct; rebuild immediately if we are already live.
    if (RootWidget && !NameText)
    {
        BuildRowTree();
    }
}

void UAstrawildShopRowWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildRowTree();
}

void UAstrawildShopRowWidget::BuildRowTree()
{
    if (RootWidget)
    {
        return; // Already built (double construct guard).
    }

    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(RowItemId) : nullptr;
    if (!ItemDef || !Vendor || !ParentShop.IsValid())
    {
        return;
    }

    const UAstrawildNPCDefinition* NpcDef = Vendor->NpcDefinition;
    const FName CurrencyId = NpcDef ? NpcDef->CurrencyItemId : NAME_None;
    const UAstrawildItemDefinition* CurrencyDef = (Registry && !CurrencyId.IsNone()) ? Registry->FindItem(CurrencyId) : nullptr;
    const FString CurrencyName = CurrencyDef ? CurrencyDef->DisplayName.ToString() : TEXT("shards");

    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ShopRow"));

    auto MakeText = [this](const FString& Name, const FLinearColor& Color, const int32 FontSize) -> UTextBlock*
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), FontSize));
        return Text;
    };

    // Name (+ owned count on sell rows).
    FString NameLine = ItemDef->DisplayName.ToString();
    if (!bBuyRow)
    {
        const APawn* OwningPawn = ParentShop->GetOwningPlayerPawn();
        const AAstrawildPlayerCharacter* Player = OwningPawn ? Cast<AAstrawildPlayerCharacter>(OwningPawn) : nullptr;
        const int32 Owned = Player && Player->InventoryComponent ? Player->InventoryComponent->GetQuantity(RowItemId) : 0;
        NameLine = FString::Printf(TEXT("%s  ×%d"), *NameLine, Owned);
    }
    NameText = MakeText(TEXT("RowName"), FLinearColor(0.95f, 0.93f, 0.85f, 1.0f), 13);

    // Price: buy rows show VendorPrice; sell rows show half (floor 1).
    const int32 Price = bBuyRow
        ? ItemDef->VendorPrice
        : AAstrawildNPCCharacter::ComputeVendorSellValue(ItemDef->VendorPrice);
    PriceText = MakeText(TEXT("RowPrice"),
        bBuyRow ? FLinearColor(0.98f, 0.78f, 0.40f, 1.0f) : FLinearColor(0.45f, 0.88f, 0.80f, 1.0f), 13);

    ActionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RowAction"));
    ActionButton->SetBackgroundColor(bBuyRow
        ? FLinearColor(0.55f, 0.42f, 0.18f, 1.0f)
        : FLinearColor(0.16f, 0.45f, 0.42f, 1.0f));
    UTextBlock* ButtonLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RowActionLabel"));
    ButtonLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    ButtonLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 12));
    ActionButton->AddChild(ButtonLabel);

    // Fill the row and wire the click AFTER the widgets exist.
    auto* NameSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(NameText));
    NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    NameSlot->SetVerticalAlignment(VAlign_Center);
    auto* PriceSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(PriceText));
    PriceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    PriceSlot->SetPadding(FMargin(12.0f, 0.0f, 12.0f, 0.0f));
    PriceSlot->SetVerticalAlignment(VAlign_Center);
    auto* ButtonSlot = Cast<UHorizontalBoxSlot>(Row->AddChildToHorizontalBox(ActionButton));
    ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    ButtonSlot->SetVerticalAlignment(VAlign_Center);

    WidgetTree->RootWidget = Row;

    // Text content needs the slot layout done first (order: build → set text).
    NameText->SetText(FText::FromString(NameLine));
    PriceText->SetText(FText::FromString(FString::Printf(TEXT("%d %s"), Price, *CurrencyName)));
    ButtonLabel->SetText(FText::FromString(bBuyRow ? TEXT("Buy ×1") : TEXT("Sell ×1")));

    if (ActionButton)
    {
        ActionButton->OnClicked.AddDynamic(this, &UAstrawildShopRowWidget::HandleActionClicked);
    }
}

void UAstrawildShopRowWidget::HandleActionClicked()
{
    AAstrawildNPCCharacter* VendorPtr = Vendor.Get();
    UAstrawildShopWidget* Shop = ParentShop.Get();
    if (!VendorPtr || !Shop)
    {
        return;
    }

    APawn* Pawn = Shop->GetOwningPlayerPawn();
    if (!Pawn)
    {
        return;
    }

    const EAstrawildVendorResult Result = bBuyRow
        ? VendorPtr->TryPurchase(Pawn, RowItemId, 1)
        : VendorPtr->TrySell(Pawn, RowItemId, 1);

    Shop->HandleRowTransaction(FText::FromString(FString::Printf(TEXT("%s — %s"),
        *VendorResultText(Result),
        bBuyRow ? TEXT("bought") : TEXT("sold"))));
}

// ---------------------------------------------------------------------------
// Shop screen
// ---------------------------------------------------------------------------
void UAstrawildShopWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BuildWidgetTree();
}

void UAstrawildShopWidget::InitializeShop(AAstrawildNPCCharacter* InVendor)
{
    Vendor = InVendor;
    if (!RootCanvas)
    {
        BuildWidgetTree();
    }
    RefreshShop();
}

void UAstrawildShopWidget::BuildWidgetTree()
{
    if (RootCanvas)
    {
        return; // Already built.
    }

    RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ShopRootCanvas"));
    WidgetTree->RootWidget = RootCanvas;

    // Dim the world behind the shop (semi-transparent black backdrop).
    UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShopBackdrop"));
    Backdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.55f));
    UCanvasPanelSlot* BackdropSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(Backdrop));
    BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
    BackdropSlot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 0.0f));

    // Panel card.
    UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ShopPanel"));
    Panel->SetBrushColor(FLinearColor(0.07f, 0.08f, 0.10f, 0.96f));
    UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(RootCanvas->AddChildToCanvas(Panel));
    PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
    PanelSlot->SetPosition(FVector2D(-ShopPanelWidth * 0.5f, -ShopPanelHeight * 0.5f));
    PanelSlot->SetSize(FVector2D(ShopPanelWidth, ShopPanelHeight));

    UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ShopLayout"));
    Panel->SetContent(Layout);

    auto MakeText = [this](const FString& Name, const FLinearColor& Color, const int32 FontSize) -> UTextBlock*
    {
        UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *Name);
        Text->SetColorAndOpacity(FSlateColor(Color));
        Text->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf"), FontSize));
        return Text;
    };

    auto AddVertical = [Layout](UWidget* Widget, const float Padding) -> UVerticalBoxSlot*
    {
        auto* Slot = Cast<UVerticalBoxSlot>(Layout->AddChildToVerticalBox(Widget));
        Slot->SetPadding(FMargin(24.0f, Padding, 24.0f, Padding));
        return Slot;
    };

    // Header.
    TitleText = MakeText(TEXT("ShopTitle"), FLinearColor(0.98f, 0.85f, 0.55f, 1.0f), 20);
    AddVertical(TitleText, 20.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    BalanceText = MakeText(TEXT("ShopBalance"), FLinearColor(0.55f, 0.90f, 0.85f, 1.0f), 14);
    AddVertical(BalanceText, 4.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    UTextBlock* BuyHeader = MakeText(TEXT("ShopBuyHeader"), FLinearColor(0.80f, 0.80f, 0.72f, 1.0f), 13);
    AddVertical(BuyHeader, 12.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    BuyBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ShopBuyBox"));
    AddVertical(BuyBox, 4.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    UTextBlock* SellHeader = MakeText(TEXT("ShopSellHeader"), FLinearColor(0.80f, 0.80f, 0.72f, 1.0f), 13);
    AddVertical(SellHeader, 10.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    SellBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ShopSellBox"));
    AddVertical(SellBox, 4.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

    StatusText = MakeText(TEXT("ShopStatus"), FLinearColor(0.95f, 0.95f, 0.85f, 1.0f), 13);
    AddVertical(StatusText, 8.0f)->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));

    CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ShopClose"));
    CloseButton->SetBackgroundColor(FLinearColor(0.35f, 0.28f, 0.14f, 1.0f));
    UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ShopCloseLabel"));
    CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    CloseLabel->SetFont(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 13));
    CloseLabel->SetText(FText::FromString(TEXT("Close [mouse]")));
    CloseButton->AddChild(CloseLabel);
    auto* CloseSlot = AddVertical(CloseButton, 8.0f);
    CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    CloseSlot->SetHorizontalAlignment(HAlign_Right);
    CloseButton->OnClicked.AddDynamic(this, &UAstrawildShopWidget::HandleCloseClicked);
}

void UAstrawildShopWidget::RefreshShop()
{
    AAstrawildNPCCharacter* VendorPtr = Vendor.Get();
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!VendorPtr || !Registry || !BuyBox || !SellBox)
    {
        return;
    }

    const UAstrawildNPCDefinition* NpcDef = VendorPtr->NpcDefinition;
    if (!NpcDef || NpcDef->ShopLootTableId.IsNone() || NpcDef->CurrencyItemId.IsNone())
    {
        return;
    }

    AAstrawildPlayerCharacter* Player = GetOwningPlayerPawn()
        ? Cast<AAstrawildPlayerCharacter>(GetOwningPlayerPawn())
        : nullptr;

    // Header + balance.
    if (TitleText)
    {
        TitleText->SetText(FText::FromString(FString::Printf(TEXT("%s's Wares"), *NpcDef->DisplayName.ToString())));
    }
    if (BalanceText && Player && Player->InventoryComponent)
    {
        const int32 Balance = Player->InventoryComponent->GetQuantity(NpcDef->CurrencyItemId);
        if (const UAstrawildItemDefinition* CurrencyDef = Registry->FindItem(NpcDef->CurrencyItemId))
        {
            BalanceText->SetText(FText::FromString(FString::Printf(TEXT("Balance: %d %s"), Balance, *CurrencyDef->DisplayName.ToString())));
        }
    }

    // Buy list — the vendor's shop table.
    BuyBox->ClearChildren();
    if (const UAstrawildLootTableDefinition* Shop = Registry->FindLootTable(NpcDef->ShopLootTableId))
    {
        for (const FAstrawildItemStack& Ware : Shop->GuaranteedDrops)
        {
            const UAstrawildItemDefinition* Def = Registry->FindItem(Ware.ItemId);
            if (!Def || Def->VendorPrice <= 0)
            {
                continue;
            }
            UAstrawildShopRowWidget* Row = WidgetTree->ConstructWidget<UAstrawildShopRowWidget>(UAstrawildShopRowWidget::StaticClass());
            Row->InitializeRow(this, VendorPtr, Ware.ItemId, true);
            auto* Slot = Cast<UScrollBoxSlot>(BuyBox->AddChild(Row));
            Slot->SetSize(FSlateChildSize(FVector2D(ShopPanelWidth - 48.0f, RowHeight)));
        }
    }

    // Sell list — priced items the player actually carries.
    SellBox->ClearChildren();
    if (Player && Player->InventoryComponent)
    {
        for (const FAstrawildItemStack& Stack : Player->InventoryComponent->GetItemStacks())
        {
            const UAstrawildItemDefinition* Def = Registry->FindItem(Stack.ItemId);
            if (!Def || Def->VendorPrice <= 0 || Stack.ItemId == NpcDef->CurrencyItemId || Stack.Quantity < 1)
            {
                continue;
            }
            UAstrawildShopRowWidget* Row = WidgetTree->ConstructWidget<UAstrawildShopRowWidget>(UAstrawildShopRowWidget::StaticClass());
            Row->InitializeRow(this, VendorPtr, Stack.ItemId, false);
            auto* Slot = Cast<UScrollBoxSlot>(SellBox->AddChild(Row));
            Slot->SetSize(FSlateChildSize(FVector2D(ShopPanelWidth - 48.0f, RowHeight)));
        }
    }

    if (StatusText)
    {
        StatusText->SetText(FText::FromString(TEXT("Buy at the listed price · sell at half price (floor 1).")));
    }
}

void UAstrawildShopWidget::HandleRowTransaction(const FText& StatusMessage)
{
    if (StatusText)
    {
        StatusText->SetText(StatusMessage);
    }
    RefreshShop();
}

void UAstrawildShopWidget::HandleCloseClicked()
{
    if (AAstrawildPlayerController* PC = GetOwningPlayer<AAstrawildPlayerController>())
    {
        PC->CloseShop();
    }
}
