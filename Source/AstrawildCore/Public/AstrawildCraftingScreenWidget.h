#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildCraftingScreenWidget.generated.h"

class UAstrawildCraftingComponent;
class UAstrawildRecipeDefinition;
class UAstrawildCraftingScreenWidget;
class UButton;
class UScrollBox;
class UTextBlock;

/**
 * FPP-1 (presentation pass): one recipe row of the native crafting screen.
 * Mirrors the roster/journal row pattern (pure-C++ Slate, no WBP needed) so
 * the crafting path is player-usable in the zero-asset build.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildCraftingRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeRow(UAstrawildCraftingScreenWidget* ParentScreenPtr,
        const UAstrawildRecipeDefinition* Recipe, bool bStationNearby, bool bCraftableNow);

protected:
    virtual void NativeConstruct() override;

    UFUNCTION()
    void HandleCraftClicked();

private:
    void BuildRowTree();

    UPROPERTY()
    TObjectPtr<UAstrawildCraftingScreenWidget> ParentScreen;

    UPROPERTY()
    TObjectPtr<UTextBlock> RowText;

    UPROPERTY()
    TObjectPtr<UButton> CraftButton;

    FName RowRecipeId = NAME_None;
    FString RowDisplayName;
    TArray<FAstrawildItemStack> RowInputs;
    TArray<FAstrawildItemStack> RowOutputs;
    FName RowStationId = NAME_None;
    float RowCraftSeconds = 0.0f;
    bool bRowCraftable = false;
    bool bRowStationNearby = false;
};

/**
 * UMG crafting screen (directive §15 — the documented "future UMG contract").
 *
 * FPP-1 (presentation pass): the class is now CONCRETE and self-sufficient —
 * the previous state was UCLASS(Abstract) with no authored WBP subclass in
 * Content, so the PlayerController's CreateWidget fallback returned nullptr
 * and the player could not open the crafting screen at all. The base class
 * builds a native pure-C++ recipe list (title, status line, scroll rows with
 * per-recipe [Craft] buttons, close button) and keeps ALL the binding to the
 * owning player's crafting component; UMG assets stay pure view code and can
 * still subclass this widget to restyle (the BP_* events keep firing).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildCraftingScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Final-audit F-05: focusable so ESC actually closes the screen in UIOnly input mode. */
    UAstrawildCraftingScreenWidget();

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

    /** Bound crafting component from the owning player pawn (may be null early). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting|UI")
    UAstrawildCraftingComponent* GetCraftingComponent() const { return CraftingComponent.Get(); }

    /** Ask the server to start a craft by recipe id (routes through Server RPC). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Crafting|UI")
    bool RequestCraft(FName RecipeId);

    /** Ask the server to cancel the active craft (server refunds ingredients). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Crafting|UI")
    bool RequestCancelCraft();

    /** Rebuild the recipe list now (call after inventory/tech changes). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Crafting|UI")
    void RefreshRecipes();

    /** Fired on construct and on RefreshRecipes — the tech-unlocked recipe listing. */
    UFUNCTION(BlueprintImplementableEvent, Category="ASTRAWILD|Crafting|UI")
    void BP_OnRecipesAvailable(const TArray<UAstrawildRecipeDefinition*>& Recipes);

    /** Fired when a timed craft begins. */
    UFUNCTION(BlueprintImplementableEvent, Category="ASTRAWILD|Crafting|UI")
    void BP_OnCraftStarted(FName RecipeId, float DurationSeconds);

    /** Progress tick for the active craft (0..1). */
    UFUNCTION(BlueprintImplementableEvent, Category="ASTRAWILD|Crafting|UI")
    void BP_OnCraftProgress(FName RecipeId, float ProgressFraction);

    /** Fired when the active craft completes. */
    UFUNCTION(BlueprintImplementableEvent, Category="ASTRAWILD|Crafting|UI")
    void BP_OnCraftCompleted(FName RecipeId, bool bSuccess);

    /** Fired when the active craft is cancelled (bRefunded when ingredients returned). */
    UFUNCTION(BlueprintImplementableEvent, Category="ASTRAWILD|Crafting|UI")
    void BP_OnCraftCancelled(FName RecipeId, bool bRefunded);

private:
    /** Resolve + bind the crafting component (idempotent). */
    void BindCraftingComponent();

    void UnbindCraftingComponent();

    /** FPP-1: build the native Slate tree (title + status + list + close). */
    void BuildNativeUi();

    /** FPP-1: fill RecipeList from the tech-unlocked recipes. */
    void PopulateRecipeList(const TArray<UAstrawildRecipeDefinition*>& Recipes);

    /** FPP-1: the live status line (active craft progress / idle hint). */
    void RefreshStatusLine();

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleCraftStarted(FName RecipeId, float DurationSeconds);

    UFUNCTION()
    void HandleCraftProgress(FName RecipeId, float ProgressFraction);

    UFUNCTION()
    void HandleCraftCompleted(FName RecipeId, bool bSuccess);

    UFUNCTION()
    void HandleCraftCancelled(FName RecipeId, bool bRefunded);

    TWeakObjectPtr<UAstrawildCraftingComponent> CraftingComponent;

    UPROPERTY()
    TObjectPtr<UTextBlock> TitleText;

    UPROPERTY()
    TObjectPtr<UTextBlock> StatusText;

    UPROPERTY()
    TObjectPtr<UScrollBox> RecipeList;

    UPROPERTY()
    TObjectPtr<UButton> CloseButton;
};
