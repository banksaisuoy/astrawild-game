#include "UI/AstrawildGameplayWidgets.h"

#include "Characters/AstrawildCharacter.h"
#include "Components/AstrawildCraftingComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Blueprint/WidgetTree.h"

void UAstrawildInventorySlotWidget::SetSlotData(const FGameplayTag& InItemTag, const int32 InQuantity, const int32 InSlotIndex)
{
    ItemTag = InItemTag;
    Quantity = InQuantity;
    SlotIndex = InSlotIndex;
}

void UAstrawildInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BindInventory();
    RefreshInventory();
}

void UAstrawildInventoryWidget::NativeDestruct()
{
    if (BoundInventory.IsValid())
    {
        BoundInventory->OnInventoryUpdated.RemoveDynamic(this, &UAstrawildInventoryWidget::HandleInventoryUpdated);
        BoundInventory->OnItemAdded.RemoveDynamic(this, &UAstrawildInventoryWidget::HandleItemAdded);
        BoundInventory->OnItemRemoved.RemoveDynamic(this, &UAstrawildInventoryWidget::HandleItemRemoved);
    }
    BoundInventory.Reset();
    Super::NativeDestruct();
}

void UAstrawildInventoryWidget::BindInventory()
{
    APawn* Pawn = GetOwningPlayerPawn();
    AAstrawildCharacter* Character = Cast<AAstrawildCharacter>(Pawn);
    BoundInventory.Reset();
    if (Character && Character->Inventory)
    {
        BoundInventory = Character->Inventory.Get();
    }

    if (BoundInventory.IsValid())
    {
        BoundInventory->OnInventoryUpdated.AddDynamic(this, &UAstrawildInventoryWidget::HandleInventoryUpdated);
        BoundInventory->OnItemAdded.AddDynamic(this, &UAstrawildInventoryWidget::HandleItemAdded);
        BoundInventory->OnItemRemoved.AddDynamic(this, &UAstrawildInventoryWidget::HandleItemRemoved);
        SlotCount = BoundInventory->MaxSlots;
    }
}

void UAstrawildInventoryWidget::RefreshInventory()
{
    if (!InventoryGrid || !SlotWidgetClass || !BoundInventory.IsValid())
    {
        return;
    }

    InventoryGrid->ClearChildren();
    const TArray<FAstrawildItemSlot>& Slots = BoundInventory->GetSlots();
    SlotCount = FMath::Max(SlotCount, Slots.Num());

    for (int32 Index = 0; Index < SlotCount; ++Index)
    {
        UAstrawildInventorySlotWidget* SlotWidget = CreateWidget<UAstrawildInventorySlotWidget>(this, SlotWidgetClass);
        if (!SlotWidget)
        {
            continue;
        }

        if (Slots.IsValidIndex(Index))
        {
            SlotWidget->SetSlotData(Slots[Index].ItemTag, Slots[Index].Quantity, Index);
        }
        else
        {
            SlotWidget->SetSlotData(FGameplayTag(), 0, Index);
        }

        const int32 Row = Index / FMath::Max(1, GridColumns);
        const int32 Column = Index % FMath::Max(1, GridColumns);
        InventoryGrid->AddChildToUniformGrid(SlotWidget, Row, Column);
    }
}

void UAstrawildInventoryWidget::HandleInventoryUpdated()
{
    RefreshInventory();
}

void UAstrawildInventoryWidget::HandleItemAdded(const FGameplayTag& ItemTag, const int32 Quantity)
{
    RefreshInventory();
}

void UAstrawildInventoryWidget::HandleItemRemoved(const FGameplayTag& ItemTag, const int32 Quantity)
{
    RefreshInventory();
}

void UAstrawildCraftingWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BindCrafting();
    RefreshRecipes();
}

void UAstrawildCraftingWidget::NativeDestruct()
{
    if (BoundCrafting.IsValid())
    {
        BoundCrafting->OnCraftSuccess.RemoveDynamic(this, &UAstrawildCraftingWidget::HandleCraftSuccess);
        BoundCrafting->OnCraftFailed.RemoveDynamic(this, &UAstrawildCraftingWidget::HandleCraftFailed);
    }
    BoundCrafting.Reset();
    Super::NativeDestruct();
}

void UAstrawildCraftingWidget::BindCrafting()
{
    APawn* Pawn = GetOwningPlayerPawn();
    AAstrawildCharacter* Character = Cast<AAstrawildCharacter>(Pawn);
    BoundCrafting.Reset();
    if (Character && Character->Crafting)
    {
        BoundCrafting = Character->Crafting.Get();
    }

    if (BoundCrafting.IsValid())
    {
        BoundCrafting->OnCraftSuccess.AddDynamic(this, &UAstrawildCraftingWidget::HandleCraftSuccess);
        BoundCrafting->OnCraftFailed.AddDynamic(this, &UAstrawildCraftingWidget::HandleCraftFailed);
    }
}

void UAstrawildCraftingWidget::RefreshRecipes()
{
    VisibleRecipes.Reset();
    if (BoundCrafting.IsValid())
    {
        VisibleRecipes = BoundCrafting->KnownRecipes;
    }

    if (VisibleRecipes.Num() == 0)
    {
        SelectedRecipeIndex = INDEX_NONE;
    }
    else if (!VisibleRecipes.IsValidIndex(SelectedRecipeIndex))
    {
        SelectedRecipeIndex = 0;
    }
    UpdateSelectedRecipeState();
}

void UAstrawildCraftingWidget::SelectRecipe(const int32 RecipeIndex)
{
    if (VisibleRecipes.IsValidIndex(RecipeIndex))
    {
        SelectedRecipeIndex = RecipeIndex;
        UpdateSelectedRecipeState();
    }
}

bool UAstrawildCraftingWidget::CraftSelectedRecipe()
{
    if (!BoundCrafting.IsValid() || !VisibleRecipes.IsValidIndex(SelectedRecipeIndex))
    {
        return false;
    }

    return BoundCrafting->CraftRecipe(VisibleRecipes[SelectedRecipeIndex]);
}

void UAstrawildCraftingWidget::HandleCraftSuccess(const FAstrawildRecipe& Recipe)
{
    RefreshRecipes();
}

void UAstrawildCraftingWidget::HandleCraftFailed(const FAstrawildRecipe& Recipe, const FString& Reason)
{
    UpdateSelectedRecipeState();
}

void UAstrawildCraftingWidget::UpdateSelectedRecipeState()
{
    bCanCraftSelectedRecipe = BoundCrafting.IsValid() && VisibleRecipes.IsValidIndex(SelectedRecipeIndex)
        && BoundCrafting->CanCraft(VisibleRecipes[SelectedRecipeIndex]);
}
