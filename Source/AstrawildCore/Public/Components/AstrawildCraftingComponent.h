// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildCraftingComponent.generated.h"

class UAstrawildInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftSuccessSignature, const FAstrawildRecipe&, Recipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCraftFailedSignature, const FAstrawildRecipe&, Recipe, const FString&, Reason);

UCLASS(ClassGroup = (Astrawild), meta = (BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAstrawildCraftingComponent();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	TArray<FAstrawildRecipe> KnownRecipes;

	UPROPERTY(BlueprintAssignable, Category = "Crafting|Events")
	FOnCraftSuccessSignature OnCraftSuccess;

	UPROPERTY(BlueprintAssignable, Category = "Crafting|Events")
	FOnCraftFailedSignature OnCraftFailed;

public:
	UFUNCTION(BlueprintPure, Category = "Crafting")
	bool CanCraft(const FAstrawildRecipe& Recipe, EAstrawildBuildingType CurrentStation = EAstrawildBuildingType::None) const;

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	bool CraftRecipe(const FAstrawildRecipe& Recipe, EAstrawildBuildingType CurrentStation = EAstrawildBuildingType::None);

	UFUNCTION(BlueprintCallable, Category = "Crafting")
	void RegisterDefaultRecipes();

private:
	TWeakObjectPtr<UAstrawildInventoryComponent> CachedInventory;
	UAstrawildInventoryComponent* GetInventory();
};