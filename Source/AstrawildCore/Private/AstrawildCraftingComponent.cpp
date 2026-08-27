#include "AstrawildCraftingComponent.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildInventoryComponent.h"

UAstrawildCraftingComponent::UAstrawildCraftingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAstrawildCraftingComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!InventoryOwner.IsValid())
    {
        InventoryOwner = GetOwner();
    }
}

void UAstrawildCraftingComponent::SetInventoryOwner(AActor* InOwner)
{
    InventoryOwner = InOwner;
}

bool UAstrawildCraftingComponent::CanCraft(const UAstrawildRecipeDefinition* Recipe) const
{
    if (!IsValid(Recipe) || Recipe->RecipeId.IsNone() || !InventoryOwner.IsValid())
    {
        return false;
    }

    const UAstrawildInventoryComponent* Inventory = InventoryOwner->FindComponentByClass<UAstrawildInventoryComponent>();
    if (!IsValid(Inventory))
    {
        return false;
    }

    for (const FAstrawildItemStack& Ingredient : Recipe->Ingredients)
    {
        if (!Ingredient.IsValid() || !Inventory->HasItem(Ingredient.ItemId, Ingredient.Quantity))
        {
            return false;
        }
    }
    return true;
}

bool UAstrawildCraftingComponent::CraftRecipe(const UAstrawildRecipeDefinition* Recipe)
{
    if (!CanCraft(Recipe))
    {
        UE_LOG(LogAstrawild, Verbose, TEXT("CraftRecipe rejected: recipe is unavailable."));
        return false;
    }

    UAstrawildInventoryComponent* Inventory = InventoryOwner.IsValid()
        ? InventoryOwner->FindComponentByClass<UAstrawildInventoryComponent>()
        : nullptr;
    if (!IsValid(Inventory) || !Inventory->ConsumeItems(Recipe->Ingredients))
    {
        return false;
    }

    for (const FAstrawildItemStack& Output : Recipe->Outputs)
    {
        if (!Output.IsValid() || !Inventory->AddItem(Output.ItemId, Output.Quantity))
        {
            UE_LOG(LogAstrawild, Error, TEXT("CraftRecipe output failed for recipe %s."), *Recipe->RecipeId.ToString());
            return false;
        }
    }
    return true;
}
