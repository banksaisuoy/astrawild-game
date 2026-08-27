// Copyright Epic Games, Inc. All Rights Reserved.

#include "Components/AstrawildCraftingComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildTechnologyComponent.h"
#include "Components/AstrawildQuestComponent.h"
#include "Data/AstrawildCraftingData.h"
#include "Engine/DataTable.h"
#include "AstrawildLogChannels.h"

UAstrawildCraftingComponent::UAstrawildCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAstrawildCraftingComponent::BeginPlay()
{
	Super::BeginPlay();
	GetInventory();
	if (RecipeTable)
	{
		RegisterRecipesFromDataTable(RecipeTable);
	}
	if (KnownRecipes.Num() == 0)
	{
		RegisterDefaultRecipes();
	}
}

UAstrawildInventoryComponent* UAstrawildCraftingComponent::GetInventory()
{
	if (!CachedInventory.IsValid() && GetOwner())
	{
		CachedInventory = GetOwner()->FindComponentByClass<UAstrawildInventoryComponent>();
	}
	return CachedInventory.Get();
}

UAstrawildTechnologyComponent* UAstrawildCraftingComponent::GetTechnology()
{
	if (!CachedTechnology.IsValid() && GetOwner())
	{
		CachedTechnology = GetOwner()->FindComponentByClass<UAstrawildTechnologyComponent>();
	}
	return CachedTechnology.Get();
}

bool UAstrawildCraftingComponent::CanCraft(const FAstrawildRecipe& Recipe, EAstrawildBuildingType CurrentStation) const
{
	// Check station requirement
	if (Recipe.RequiredStation != EAstrawildBuildingType::None && Recipe.RequiredStation != CurrentStation)
	{
		return false;
	}

	if (Recipe.RequiredTechnologyTag.IsValid())
	{
		UAstrawildTechnologyComponent* Tech = const_cast<UAstrawildCraftingComponent*>(this)->GetTechnology();
		if (!Tech || !Tech->IsTechnologyUnlocked(Recipe.RequiredTechnologyTag))
		{
			return false;
		}
	}

	UAstrawildInventoryComponent* Inv = const_cast<UAstrawildCraftingComponent*>(this)->GetInventory();
	if (!Inv)
	{
		return false;
	}

	// Verify all ingredients
	for (const FAstrawildRecipeIngredient& Ingredient : Recipe.Ingredients)
	{
		if (!Inv->HasItem(Ingredient.ItemTag, Ingredient.Quantity))
		{
			return false;
		}
	}

	return true;
}

bool UAstrawildCraftingComponent::CraftRecipe(const FAstrawildRecipe& Recipe, EAstrawildBuildingType CurrentStation)
{
	if (!CanCraft(Recipe, CurrentStation))
	{
		UE_LOG(LogAstrawild, Warning, TEXT("Failed to craft recipe %s: Missing ingredients or incorrect station."), *Recipe.RecipeTag.ToString());
		OnCraftFailed.Broadcast(Recipe, TEXT("Missing required ingredients or station."));
		return false;
	}

	UAstrawildInventoryComponent* Inv = GetInventory();
	if (!Inv)
	{
		return false;
	}

	// Deduct ingredients, retaining a rollback list if the output cannot be inserted.
	TArray<FAstrawildRecipeIngredient> RemovedIngredients;
	for (const FAstrawildRecipeIngredient& Ingredient : Recipe.Ingredients)
	{
		if (Inv->RemoveItem(Ingredient.ItemTag, Ingredient.Quantity))
		{
			RemovedIngredients.Add(Ingredient);
		}
		else
		{
			for (const FAstrawildRecipeIngredient& Removed : RemovedIngredients)
			{
				Inv->AddItem(Removed.ItemTag, Removed.Quantity);
			}
			OnCraftFailed.Broadcast(Recipe, TEXT("Ingredient state changed before crafting completed."));
			return false;
		}
	}

	// Add output item
	const bool bAdded = Inv->AddItem(Recipe.OutputItemTag, Recipe.OutputQuantity);
	if (bAdded)
	{
					UE_LOG(LogAstrawild, Log, TEXT("Successfully crafted %s (Qty: %d)"), *Recipe.OutputItemTag.ToString(), Recipe.OutputQuantity);
			if (UAstrawildQuestComponent* Quest = GetOwner() ? GetOwner()->FindComponentByClass<UAstrawildQuestComponent>() : nullptr)
			{
				Quest->AddProgressForTarget(EAstrawildQuestObjectiveType::Craft, Recipe.OutputItemTag, Recipe.OutputQuantity);
			}
			OnCraftSuccess.Broadcast(Recipe);

		return true;
	}

	for (const FAstrawildRecipeIngredient& Removed : RemovedIngredients)
	{
		Inv->AddItem(Removed.ItemTag, Removed.Quantity);
	}
	OnCraftFailed.Broadcast(Recipe, TEXT("Inventory is full; ingredients were refunded."));
	return false;
}

int32 UAstrawildCraftingComponent::RegisterRecipesFromDataTable(UDataTable* InRecipeTable)
{
	if (!InRecipeTable)
	{
		return 0;
	}

	TArray<FAstrawildCraftingRecipeRow*> Rows;
	InRecipeTable->GetAllRows<FAstrawildCraftingRecipeRow>(TEXT("RecipeImport"), Rows);
	int32 AddedCount = 0;
	for (const FAstrawildCraftingRecipeRow* Row : Rows)
	{
		if (!Row || !Row->RecipeTag.IsValid() || !Row->OutputItemTag.IsValid() || Row->OutputQuantity <= 0)
		{
			continue;
		}
		FAstrawildRecipe ExistingRecipe;
		if (FindKnownRecipe(Row->RecipeTag, ExistingRecipe))
		{
			continue;
		}

		FAstrawildRecipe Recipe;
		Recipe.RecipeTag = Row->RecipeTag;
		Recipe.DisplayName = Row->DisplayName;
		Recipe.Description = Row->Description;
		Recipe.OutputItemTag = Row->OutputItemTag;
		Recipe.OutputQuantity = Row->OutputQuantity;
		Recipe.CraftTimeSeconds = FMath::Max(0.1f, Row->CraftTimeSeconds);
		Recipe.RequiredStation = Row->RequiredStation;
		Recipe.RequiredTechnologyTag = Row->RequiredTechnologyTag;

		const int32 IngredientCount = FMath::Min(Row->IngredientTags.Num(), Row->IngredientQuantities.Num());
		for (int32 Index = 0; Index < IngredientCount; ++Index)
		{
			if (Row->IngredientTags[Index].IsValid() && Row->IngredientQuantities[Index] > 0)
			{
				FAstrawildRecipeIngredient& Ingredient = Recipe.Ingredients.AddDefaulted_GetRef();
				Ingredient.ItemTag = Row->IngredientTags[Index];
				Ingredient.Quantity = Row->IngredientQuantities[Index];
			}
		}
		KnownRecipes.Add(Recipe);
		++AddedCount;
	}
	return AddedCount;
}

bool UAstrawildCraftingComponent::FindKnownRecipe(const FGameplayTag& RecipeTag, FAstrawildRecipe& OutRecipe) const
{
	if (!RecipeTag.IsValid())
	{
		return false;
	}
	for (const FAstrawildRecipe& KnownRecipe : KnownRecipes)
	{
		if (KnownRecipe.RecipeTag == RecipeTag)
		{
			OutRecipe = KnownRecipe;
			return true;
		}
	}
	return false;
}

void UAstrawildCraftingComponent::RegisterDefaultRecipes()
{
	// Recipe 1: Primal Stone Axe
	{
		FAstrawildRecipe AxeRecipe;
		AxeRecipe.RecipeTag = FGameplayTag::RequestGameplayTag(FName("Recipe.Tool.StoneAxe"), false);
		AxeRecipe.DisplayName = FText::FromString(TEXT("Primal Stone Axe"));
		AxeRecipe.OutputItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Tool.StoneAxe"), false);
		AxeRecipe.OutputQuantity = 1;
		AxeRecipe.CraftTimeSeconds = 2.0f;
		AxeRecipe.RequiredStation = EAstrawildBuildingType::None;

		FAstrawildRecipeIngredient IngWood;
		IngWood.ItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false);
		IngWood.Quantity = 5;
		AxeRecipe.Ingredients.Add(IngWood);

		FAstrawildRecipeIngredient IngStone;
		IngStone.ItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false);
		IngStone.Quantity = 3;
		AxeRecipe.Ingredients.Add(IngStone);

		KnownRecipes.Add(AxeRecipe);
	}

	// Recipe 2: Primal Stone Pick
	{
		FAstrawildRecipe PickRecipe;
		PickRecipe.RecipeTag = FGameplayTag::RequestGameplayTag(FName("Recipe.Tool.StonePick"), false);
		PickRecipe.DisplayName = FText::FromString(TEXT("Primal Stone Pick"));
		PickRecipe.OutputItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Tool.StonePick"), false);
		PickRecipe.OutputQuantity = 1;
		PickRecipe.CraftTimeSeconds = 2.0f;
		PickRecipe.RequiredStation = EAstrawildBuildingType::None;

		FAstrawildRecipeIngredient IngWood;
		IngWood.ItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false);
		IngWood.Quantity = 5;
		PickRecipe.Ingredients.Add(IngWood);

		FAstrawildRecipeIngredient IngStone;
		IngStone.ItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false);
		IngStone.Quantity = 3;
		PickRecipe.Ingredients.Add(IngStone);

		KnownRecipes.Add(PickRecipe);
	}

	// Recipe 3: Astra Resonator T1
	{
		FAstrawildRecipe ResonatorRecipe;
		ResonatorRecipe.RecipeTag = FGameplayTag::RequestGameplayTag(FName("Recipe.Tool.ResonatorT1"), false);
		ResonatorRecipe.DisplayName = FText::FromString(TEXT("Astra Resonator T1"));
		ResonatorRecipe.OutputItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Tool.AstraResonatorBasic"), false);
		ResonatorRecipe.OutputQuantity = 1;
		ResonatorRecipe.CraftTimeSeconds = 3.0f;
		ResonatorRecipe.RequiredStation = EAstrawildBuildingType::CraftingBench;

		FAstrawildRecipeIngredient IngShard;
		IngShard.ItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.AstraShard"), false);
		IngShard.Quantity = 1;
		ResonatorRecipe.Ingredients.Add(IngShard);

		FAstrawildRecipeIngredient IngStone;
		IngStone.ItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false);
		IngStone.Quantity = 2;
		ResonatorRecipe.Ingredients.Add(IngStone);

		FAstrawildRecipeIngredient IngWood;
		IngWood.ItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false);
		IngWood.Quantity = 3;
		ResonatorRecipe.Ingredients.Add(IngWood);

		KnownRecipes.Add(ResonatorRecipe);
	}

	// Recipe 4: Astra Resonator T2 (Enhanced Resonance)
	{
		FAstrawildRecipe ResonatorT2Recipe;
		ResonatorT2Recipe.RecipeTag = FGameplayTag::RequestGameplayTag(FName("Recipe.Tool.ResonatorT2"), false);
		ResonatorT2Recipe.DisplayName = FText::FromString(TEXT("Astra Resonator T2 (Enhanced)"));
		ResonatorT2Recipe.OutputItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Tool.AstraResonatorEnhanced"), false);
		ResonatorT2Recipe.OutputQuantity = 1;
		ResonatorT2Recipe.CraftTimeSeconds = 4.0f;
		ResonatorT2Recipe.RequiredStation = EAstrawildBuildingType::CraftingBench;

		FAstrawildRecipeIngredient IngShard;
		IngShard.ItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.AstraShard"), false);
		IngShard.Quantity = 3;
		ResonatorT2Recipe.Ingredients.Add(IngShard);

		FAstrawildRecipeIngredient IngStone;
		IngStone.ItemTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false);
		IngStone.Quantity = 5;
		ResonatorT2Recipe.Ingredients.Add(IngStone);

		KnownRecipes.Add(ResonatorT2Recipe);
	}
}