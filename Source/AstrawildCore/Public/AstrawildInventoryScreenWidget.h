#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildInventoryScreenWidget.generated.h"

class AAstrawildPlayerCharacter;
class UAstrawildInventoryScreenWidget;
class UButton;
class UCanvasPanel;
class UTextBlock;
class UVerticalBox;

/**
 * Final production run (PHASE 4 gap): the inventory screen. Pure-C++ UMG (same
 * zero-asset doctrine as the HUD/shop): lists every stack with weight, shows the
 * full 6-slot loadout, and offers Use (consumables) / Equip (equipment) per row.
 * Opened with TAB — AAstrawildPlayerController::ToggleInventoryScreen.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildInventoryRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeRow(UAstrawildInventoryScreenWidget* ParentScreen, FName ItemId);

protected:
    virtual void NativeConstruct() override;

private:
    UFUNCTION()
    void HandleActionClicked();

    void BuildRowTree();

    UPROPERTY()
    TObjectPtr<UAstrawildInventoryScreenWidget> ParentScreen;

    FName RowItemId = NAME_None;

    UPROPERTY()
    TObjectPtr<UButton> ActionButton;

    UPROPERTY()
    TObjectPtr<UTextBlock> NameText;
};

/**
 * The inventory screen panel:
 *
 *   ┌ Pack ──────────── 46.2 / 120 kg ─────── [Close] ┐
 *   │ Weapon: Dawnwood Club  · Shield: —  · Armor: —  │
 *   │ Helmet: —  · Exosuit: —  · Scanner: —           │
 *   │ ── Stacks ─────────────────────────────────────  │
 *   │  Berry ×12        1.2 kg    [Eat]                │
 *   │  FiberWeave Vest  4.0 kg    [Equip]              │
 *   └──────────────────────────────────────────────────┘
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildInventoryScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Rebuild the stack listing + loadout readout (call after any change). */
    void RefreshInventory();

protected:
    virtual void NativeConstruct() override;

private:
    friend class UAstrawildInventoryRowWidget;

    void BuildWidgetTree();
    AAstrawildPlayerCharacter* GetPlayerCharacter() const;

    UFUNCTION()
    void HandleCloseClicked();

    UPROPERTY()
    TObjectPtr<UCanvasPanel> RootCanvas;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> WeightText;

    UPROPERTY()
    TObjectPtr<UTextBlock> LoadoutText;

    UPROPERTY()
    TObjectPtr<UVerticalBox> StackBox;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
