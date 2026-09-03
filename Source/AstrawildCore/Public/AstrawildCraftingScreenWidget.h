#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildCraftingScreenWidget.generated.h"

class UAstrawildCraftingComponent;
class UAstrawildRecipeDefinition;

/**
 * UMG crafting screen contract (directive §15 — the documented "future UMG contract").
 * Blueprint designers subclass this widget and implement the BP_* events; the base
 * class owns all binding to the owning player's crafting component, so UMG assets
 * stay pure view code (no logic duplication). The pure-C++ HUD covers gameplay until
 * UMG assets are authored (Docs/ASTRAWILD_UI_ARCHITECTURE.md migration path).
 */
UCLASS(Abstract, Blueprintable)
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

    UFUNCTION()
    void HandleCraftStarted(FName RecipeId, float DurationSeconds);

    UFUNCTION()
    void HandleCraftProgress(FName RecipeId, float ProgressFraction);

    UFUNCTION()
    void HandleCraftCompleted(FName RecipeId, bool bSuccess);

    UFUNCTION()
    void HandleCraftCancelled(FName RecipeId, bool bRefunded);

    TWeakObjectPtr<UAstrawildCraftingComponent> CraftingComponent;
};
