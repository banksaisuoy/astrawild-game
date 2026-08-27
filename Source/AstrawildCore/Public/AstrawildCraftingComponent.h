#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildCraftingComponent.generated.h"

class UAstrawildRecipeDefinition;

UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCraftingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildCraftingComponent();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Crafting")
    bool CanCraft(const UAstrawildRecipeDefinition* Recipe) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Crafting")
    bool CraftRecipe(const UAstrawildRecipeDefinition* Recipe);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Crafting")
    void SetInventoryOwner(AActor* InOwner);

private:
    TWeakObjectPtr<AActor> InventoryOwner;
};
