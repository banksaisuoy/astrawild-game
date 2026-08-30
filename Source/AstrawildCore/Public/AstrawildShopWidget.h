#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildShopWidget.generated.h"

class AAstrawildNPCCharacter;
class UAstrawildShopWidget;
class UButton;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;

/**
 * Batch 5 — Item C: one self-contained shop row. Pure-C++ UMG (no asset
 * dependency — same doctrine as UAstrawildHudWidget): each row builds its own
 * [name · price · action-button] tree and routes the click through the vendor's
 * server-authoritative TryPurchase/TrySell, then tells the parent screen to
 * refresh (balance + sell list change after every transaction).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildShopRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Configure the row after construction. bBuyRow=true → buy (×1), false → sell (×1). */
    void InitializeRow(UAstrawildShopWidget* ParentShop, AAstrawildNPCCharacter* Vendor, FName ItemId, bool bBuyRow);

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleActionClicked();

    void BuildRowTree();

    UPROPERTY()
    TObjectPtr<UAstrawildShopWidget> ParentShop;

    UPROPERTY()
    TObjectPtr<AAstrawildNPCCharacter> Vendor;

    /** The item this row trades (ware when buying, inventory item when selling). */
    FName RowItemId = NAME_None;

    /** Buy rows call TryPurchase; sell rows call TrySell. */
    bool bBuyRow = true;

    UPROPERTY()
    TObjectPtr<UButton> ActionButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> NameText;

    UPROPERTY()
    TObjectPtr<UTextBlock> PriceText;
};

/**
 * Batch 5 — Item C: the vendor shop screen (closes the "cheat-only shop" gap
 * from Batch 4). Pure-C++ UMG built entirely in NativeConstruct:
 *
 *   ┌ Trader Tam's Wares ────────────── [Close] ┐
 *   │ Balance: 12 Dawn Shard                    │
 *   │ ── Buy ──────────────────────────────────  │
 *   │  Echo Resonator        6 ⟐  [Buy ×1]       │
 *   │  Dawnbloom Salve       4 ⟐  [Buy ×1]       │
 *   │ ── Sell (half price, floor 1) ───────────  │
 *   │  Frostbloom   ×4  → 1 ⟐  [Sell ×1]         │
 *   └────────────────────────────────────────────┘
 *
 * Wares come from the vendor's ShopLootTableId loot table (prices = VendorPrice
 * in the vendor's currency); the sell list enumerates the player's priced
 * inventory items. All transactions route through AAstrawildNPCCharacter::
 * TryPurchase/TrySell — server-authoritative, range-checked, no partials. The
 * screen is opened by AAstrawildPlayerController::OpenShop when a vendor NPC
 * is interacted with, and input mode returns to game-only on close.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildShopWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Bind the vendor and (re)build the stock/sell listings. Safe to re-call. */
    void InitializeShop(AAstrawildNPCCharacter* InVendor);

    /** Called by rows after a transaction — refresh balance + sell list. */
    void HandleRowTransaction(const FText& StatusMessage);

    /** The vendor this screen currently trades with (may be null early). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|NPC|Vendor|UI")
    AAstrawildNPCCharacter* GetVendor() const { return Vendor.Get(); }

protected:
    virtual void NativeConstruct() override;

private:
    friend class UAstrawildShopRowWidget;

    void BuildWidgetTree();
    void RefreshShop();

    UFUNCTION()
    void HandleCloseClicked();

    UPROPERTY()
    TObjectPtr<AAstrawildNPCCharacter> Vendor;

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> BalanceText;

    UPROPERTY()
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY()
    TObjectPtr<UScrollBox> BuyBox;

    UPROPERTY()
    TObjectPtr<UScrollBox> SellBox;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
