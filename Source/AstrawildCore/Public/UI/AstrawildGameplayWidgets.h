#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AstrawildTypes.h"
#include "AstrawildGameplayWidgets.generated.h"

class UAstrawildInventoryComponent;
class UAstrawildCraftingComponent;
class UUniformGridPanel;

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildInventorySlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Inventory")
    void SetSlotData(const FGameplayTag& InItemTag, int32 InQuantity, int32 InSlotIndex);

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Inventory")
    FGameplayTag ItemTag;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Inventory")
    int32 Quantity = 0;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Inventory")
    int32 SlotIndex = INDEX_NONE;
};

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildInventoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta=(BindWidgetOptional))
    TObjectPtr<UUniformGridPanel> InventoryGrid;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|UI|Inventory")
    TSubclassOf<UAstrawildInventorySlotWidget> SlotWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|UI|Inventory", meta=(ClampMin="1"))
    int32 GridColumns = 5;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Inventory")
    int32 SlotCount = 30;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Inventory")
    void RefreshInventory();

protected:
    UFUNCTION()
    void HandleInventoryUpdated();

    UFUNCTION()
    void HandleItemAdded(const FGameplayTag& ItemTag, int32 Quantity);

    UFUNCTION()
    void HandleItemRemoved(const FGameplayTag& ItemTag, int32 Quantity);

private:
    TWeakObjectPtr<UAstrawildInventoryComponent> BoundInventory;
    void BindInventory();
};

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildCraftingWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Crafting")
    TArray<FAstrawildRecipe> VisibleRecipes;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Crafting")
    int32 SelectedRecipeIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Crafting")
    bool bCanCraftSelectedRecipe = false;

    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Crafting")
    void RefreshRecipes();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Crafting")
    void SelectRecipe(int32 RecipeIndex);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Crafting")
    bool CraftSelectedRecipe();

protected:
    UFUNCTION()
    void HandleCraftSuccess(const FAstrawildRecipe& Recipe);

    UFUNCTION()
    void HandleCraftFailed(const FAstrawildRecipe& Recipe, const FString& Reason);

private:
    TWeakObjectPtr<UAstrawildCraftingComponent> BoundCrafting;
    void BindCrafting();
    void UpdateSelectedRecipeState();
};
