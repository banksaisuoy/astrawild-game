#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildTypes.h" // FAstrawildItemStack (the SortStacksByName parameter).
#include "AstrawildInventoryScreenWidget.generated.h"

class AAstrawildPlayerCharacter;
class UAstrawildInventoryScreenWidget;
class UAstrawildItemDefinition;
class UAstrawildItemRegistrySubsystem;
class UButton;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

/**
 * Final production run (PHASE 4 gap): the inventory screen. Pure-C++ UMG (same
 * zero-asset doctrine as the HUD/shop): lists every stack with weight, shows the
 * full 6-slot loadout, and offers Use (consumables) / Equip (equipment) per row.
 * Opened with TAB — AAstrawildPlayerController::ToggleInventoryScreen.
 *
 * CGC-4 (inventory QoL pack — audit 2-c fixes):
 *  - the stacks list lives inside a UScrollBox (the JournalScreen construction
 *    idiom) with an always-visible scrollbar — the fixed 400px slot silently
 *    clipped everything past 13 stacks and was UNSCROLLABLE;
 *  - a per-row "?" button reads the item's Definition into a details line at
 *    the panel bottom (Description + the REAL non-zero stats —
 *    UAstrawildInventoryScreenWidget::FormatItemDetails);
 *  - a per-row "Discard" button (EVERY row, materials included — the
 *    overweight-junk trap escape): first click arms ("Discard N?"), a second
 *    click within 3 seconds discards the WHOLE STACK; the row rebuild resets
 *    the arm. Equipped items skip the button (the smallest honest behavior —
 *    see UAstrawildInventoryRowWidget::BuildRowTree);
 *  - a "Sort" header button orders the rows by item display name
 *    (case-insensitive, stable) — a pure PRESENTATION sort over a copy of
 *    GetItemStacks(); the underlying inventory array order is untouched.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildInventoryRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeRow(UAstrawildInventoryScreenWidget* ParentScreen, FName ItemId);

protected:
    virtual void NativeConstruct() override;

    // CGC-4 latent-compile-bug fix: this ROW class used to declare (a) a
    // constructor named after the SCREEN class and (b) a NativeOnKeyDown
    // override with no definition — both impossible to compile/link (the
    // file has never been through the engine). The row does no key handling
    // (the screen owns TAB/ESC); both bogus declarations are gone.

private:
    UFUNCTION()
    void HandleActionClicked();

    /** CGC-4: fill the screen's details line from this row's item definition. */
    UFUNCTION()
    void HandleDetailsClicked();

    /** CGC-4: discard-with-confirm — arm, then (within the window) drop the whole stack. */
    UFUNCTION()
    void HandleDiscardClicked();

    void BuildRowTree();

    /** CGC-4: true when the row's item sits in any of the six equipment slots. */
    bool IsEquippedItem() const;

    UPROPERTY()
    TObjectPtr<UAstrawildInventoryScreenWidget> ParentScreen;

    FName RowItemId = NAME_None;

    UPROPERTY()
    TObjectPtr<UButton> ActionButton;

    /** CGC-4: the details ("?") button. */
    UPROPERTY()
    TObjectPtr<UButton> DetailsButton;

    /** CGC-4: the discard button (hidden for equipped items). */
    UPROPERTY()
    TObjectPtr<UButton> DiscardButton;

    /** CGC-4: the discard button's label (relabelled on arm — the pause-menu slot-label idiom). */
    UPROPERTY()
    TObjectPtr<UTextBlock> DiscardButtonLabel;

    UPROPERTY()
    TObjectPtr<UTextBlock> NameText;

    /** CGC-4: discard confirmation state — armed timestamp (0 = disarmed). */
    double DiscardArmedAtSeconds = 0.0;
};

/**
 * The inventory screen panel:
 *
 *   ┌ Pack ──────────── 46.2 / 120 kg ──── [Sort] ┐
 *   │ Weapon: Dawnwood Club  · Shield: —  · Armor: —  │
 *   │ Helmet: —  · Exosuit: —  · Scanner: —           │
 *   │ ┌ ═ scrollable rows ════════════════════════ ╥ ╡
 *   │ │  Berry ×12        1.2 kg    [Use] [?] [Discard] │
 *   │ │  FiberWeave Vest  4.0 kg  [Equip] [?] [Discard] │
 *   │ └ ───────────────────────────────────────── ╨ ┘
 *   │ Details: Raw Echo Meat — Food +8 · 0.7 kg │
 *   │                                    [Close] │
 *   └──────────────────────────────────────────────┘
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildInventoryScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Rebuild the stack listing + loadout readout (call after any change). */
    void RefreshInventory();

    /** Final-audit F-05: focusable so the advertised TAB/ESC close actually fires in UIOnly input mode. */
    UAstrawildInventoryScreenWidget();

    // --- CGC-4: pure static contracts (world-free, test 138 pins them) ---

    /**
     * The details-line text for an item definition: DisplayName, the
     * Description, then one " · "-joined stats line carrying ONLY the non-zero
     * fields among Food/Water/Heal (as "Food +N" etc.), Weight ("N kg",
     * rendered only when the per-unit weight is non-zero) and VendorPrice
     * ("Sells for N", tradeable items only). A null definition returns an
     * EMPTY string (fail-closed — the caller clears the details line).
     */
    static FString FormatItemDetails(const UAstrawildItemDefinition* ItemDef);

    /**
     * Sort a stack list by item DISPLAY NAME, case-insensitive and STABLE
     * (TArray::StableSort — equal names keep their relative order). This is
     * the PRESENTATION sort the screen applies to a COPY of GetItemStacks();
     * the underlying inventory array order is never touched. A missing
     * registry (world-free caller) falls back to sorting by item id string —
     * fail-closed, never a crash.
     */
    static bool SortStacksByName(TArray<FAstrawildItemStack>& Stacks, const UAstrawildItemRegistrySubsystem* Registry);

protected:
    virtual void NativeConstruct() override;

private:
    friend class UAstrawildInventoryRowWidget;

    void BuildWidgetTree();
    AAstrawildPlayerCharacter* GetPlayerCharacter() const;

    /** CGC-4 (friend-called from a row): show this item's details line. */
    void ShowItemDetails(FName ItemId);

    UFUNCTION()
    void HandleCloseClicked();

    /** CGC-4: toggle the by-name row sort (a presentation copy; inventory order untouched). */
    UFUNCTION()
    void HandleSortClicked();

    /** CGC-4: refresh the sort button label from the toggle state. */
    void RefreshSortButtonLabel();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> WeightText;

    UPROPERTY()
    TObjectPtr<UTextBlock> LoadoutText;

    /** CGC-4: the rows now live inside a ScrollBox (JournalScreen idiom). */
    UPROPERTY()
    TObjectPtr<UScrollBox> StacksScroll;

    UPROPERTY()
    TObjectPtr<UVerticalBox> StackBox;

    /** CGC-4: the item details line (bottom of the panel). */
    UPROPERTY()
    TObjectPtr<UTextBlock> DetailsText;

    /** CGC-4: the by-name sort toggle. */
    UPROPERTY()
    TObjectPtr<UButton> SortButton;

    /** CGC-4: label inside the sort button (reflects the toggle state). */
    UPROPERTY()
    TObjectPtr<UTextBlock> SortButtonLabel;

    /** CGC-4: true while rows render alphabetically (default: insertion order). */
    bool bSortByName = false;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
