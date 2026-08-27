# P2 — UMG Inventory and Crafting Contract

## Runtime implementation

The release branch now includes `UAstrawildInventoryWidget`, `UAstrawildInventorySlotWidget`, and `UAstrawildCraftingWidget`. These classes bind to the Player's existing Inventory and Crafting components, refresh from their delegates, and expose data to Blueprint. They do not replace inventory transaction rules or crafting validation in C++.

## Required Widget Blueprints

| Widget | Parent class | Expected path | Required bindings |
|---|---|---|---|
| `WBP_MainHUD` | `UAstrawildHUD` or existing HUD integration | `Content/Astrawild/UI/WBP_MainHUD` | health/stamina, interaction prompt, save toast, companion strip |
| `WBP_InventoryGrid` | `UAstrawildInventoryWidget` | `Content/Astrawild/UI/WBP_InventoryGrid` | `InventoryGrid` as UniformGridPanel, `SlotWidgetClass`, five columns |
| `WBP_InventorySlot` | `UAstrawildInventorySlotWidget` | `Content/Astrawild/UI/WBP_InventorySlot` | item icon, quantity text, selected/empty states, slot index |
| `WBP_CraftingMenu` | `UAstrawildCraftingWidget` | `Content/Astrawild/UI/WBP_CraftingMenu` | recipe list, requirements, craft button calling `CraftSelectedRecipe` |
| `WBP_CraftingRecipeRow` | `UUserWidget` | `Content/Astrawild/UI/WBP_CraftingRecipeRow` | recipe name, output, ingredients, unavailable tint |

## Inventory grid behavior

The widget creates one slot per `MaxSlots` and lays them out using `GridColumns = 5`, producing six rows for the default 30-slot inventory. A slot is empty when its `ItemTag` is invalid or `Quantity <= 0`. Drag/drop, split, and swap requests must call the existing Inventory component methods and must not mutate `Slots` directly.

## Crafting behavior

`VisibleRecipes` is copied from the Player's `KnownRecipes`. Selecting a row calls `SelectRecipe`; the button calls `CraftSelectedRecipe`. The `bCanCraftSelectedRecipe` flag should drive button enabled state. Success and failure delegates should update a toast/status region, while ingredient validation remains in `UAstrawildCraftingComponent::CanCraft`.

## UI quality gate

Use readable contrast, controller-safe focus order, keyboard navigation, and a low-resolution layout test. Verify that opening and closing inventory does not pause or duplicate the widget, that inventory updates refresh without rebuilding the whole HUD, and that craft failure explains the missing requirement. Record the actual `.uasset` paths and one screenshot of each widget in `Docs/BUILD_STATUS.md`.
