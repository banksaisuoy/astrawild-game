#include "AstrawildCraftingScreenWidget.h"

#include "AstrawildPlayerController.h"

#include "AstrawildCraftingComponent.h"
#include "AstrawildLog.h"
#include "GameFramework/Pawn.h"


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

    BindCraftingComponent();

    if (UAstrawildCraftingComponent* Component = CraftingComponent.Get())
    {
        BP_OnRecipesAvailable(Component->GetTechUnlockedRecipes());
    }
}

void UAstrawildCraftingScreenWidget::NativeDestruct()
{
    UnbindCraftingComponent();
    Super::NativeDestruct();
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
        BP_OnRecipesAvailable(Component->GetTechUnlockedRecipes());
    }
}

void UAstrawildCraftingScreenWidget::HandleCraftStarted(const FName RecipeId, const float DurationSeconds)
{
    BP_OnCraftStarted(RecipeId, DurationSeconds);
}

void UAstrawildCraftingScreenWidget::HandleCraftProgress(const FName RecipeId, const float ProgressFraction)
{
    BP_OnCraftProgress(RecipeId, ProgressFraction);
}

void UAstrawildCraftingScreenWidget::HandleCraftCompleted(const FName RecipeId, const bool bSuccess)
{
    BP_OnCraftCompleted(RecipeId, bSuccess);
}

void UAstrawildCraftingScreenWidget::HandleCraftCancelled(const FName RecipeId, const bool bRefunded)
{
    BP_OnCraftCancelled(RecipeId, bRefunded);
}
