# ASTRAWILD — Crafting System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary sources:** `AstrawildCraftingComponent.h/.cpp`, `AstrawildCraftingStationActor.h/.cpp`,
`AstrawildContentLibrary.cpp::BuildRecipes()`

Timed, tech-gated crafting with station proximity. One craft at a time per component. The player-facing
surface today is the **station interact** (deliberate vertical-slice stopgap); the real UMG crafting screen
contract is in §6.

---

## 1. Gates (checked in order)

`CanCraft(recipe)`:

1. **Ingredient availability** — every `Ingredients[i]` must satisfy `Inventory->HasItem(id, qty)`.
2. **Technology gate** — if `Recipe->RequiredTechId != NAME_None`, `Research->IsTechUnlocked` must pass.
3. **Station proximity** — if `Recipe->RequiredStationId != NAME_None`, a crafting station actor with
   matching `StationId` must exist within its `UseRadius` (**500 cm** default) of the player.
   `CanCraftIgnoringStation` performs checks 1–2 only (for UI listing).

---

## 2. The 5 CODE_DEFAULT Recipes

From `AstrawildContentLibrary.cpp::BuildRecipes()`:

| Recipe | Id | Ingredients | Output | Duration | Tech gate | Station |
|---|---|---|---|---|---|---|
| Echo Resonator | `Recipe_Resonator` | Stone ×2 + Fiber ×1 | Resonator ×1 | 3.0 s | — | — (anywhere) |
| Sunfiber Bandage | `Recipe_Bandage` | Fiber ×2 | Bandage ×1 | 2.0 s | — | — (anywhere) |
| Dawnwood Plank | `Recipe_WoodPlank` | Wood ×2 | Plank ×1 | 2.0 s | — | `Station_Workbench` |
| Seared Meat | `Recipe_CookedMeat` | Raw Meat ×1 | Cooked Meat ×1 | 5.0 s | `Tech_Cooking` | `Station_Campfire` |
| Dew Flask | `Recipe_WaterFlask` | Fiber ×2 + Crystal Shard ×1 | Flask ×1 | 4.0 s | — | `Station_Workbench` |

---

## 3. Craft Execution (server)

```
CraftRecipe(Recipe)                        [server; requires authority + CanCraft + not already crafting]
  ├─ Inventory->ConsumeItems(Recipe->Ingredients)      (all-or-nothing)
  ├─ if CraftDurationSeconds ≤ 0  → INSTANT:
  │     ├─ add outputs to inventory
  │     ├─ OnCraftCompleted(recipeId, true)
  │     └─ EventBus: Event.RecipeCrafted (quest progress)
  └─ else → TIMED QUEUE:
        ActiveRecipeId    = Recipe->RecipeId
        CraftTimeTotal    = Recipe->CraftDurationSeconds
        CraftTimeRemaining= CraftTimeTotal
        PendingOutputs    = Recipe->Outputs
```

- **One at a time**: `IsCrafting()` (`ActiveRecipeId != NAME_None`) blocks new crafts until completion.
  There is no multi-slot queue — v1 decision.
- Component tick (server, while crafting): `CraftTimeRemaining -= dt`; broadcasts
  `OnCraftProgress(recipeId, 1 − remaining/total)` every tick; at 0 → `CompleteActiveCraft()` adds
  `PendingOutputs` to the inventory, broadcasts `OnCraftCompleted` and publishes `Event.RecipeCrafted`.
- Failure modes return `false` and consume nothing (ingredient check precedes consumption).

---

## 4. Station Interact Behavior (vertical-slice stopgap)

`AAstrawildCraftingStationActor` (interactable, E key):

- Prompt: `"Craft at Station_Workbench [E]"`.
- **Interact crafts the FIRST currently-craftable recipe that requires this station** (iteration over the
  registry map): for each recipe with `RequiredStationId == StationId`, if `CanCraft` passes →
  `CraftRecipe`. First success returns; nothing craftable → verbose log only.
- Two stations are spawned by the WorldBootstrapper at camp: `Station_Workbench` (0, +900) and
  `Station_Campfire` (0, −900), both with `UseRadius = 500 cm`.

This is deliberately documented as a **no-UI stopgap** so the loop (gather → station → crafted item) is
playable before any UMG screen exists. It is not the final UX.

---

## 5. Tech Gating Summary

Only `Recipe_CookedMeat` is tech-gated today (via `Tech_Cooking`, 5 RP). Recipe → tech binding lives on
the recipe definition; the Research subsystem doc describes the unlock path. Stations themselves are not
tech-gated (the buildings that *represent* them are separate — see Building doc).

---

## 6. Future UMG Crafting Screen Contract

The real crafting UI (PLANNED — see Roadmap) must use this existing API surface and nothing else:

| Need | API |
|---|---|
| List recipes | `Registry->GetAllRecipes()` |
| Per-recipe availability (list view, ignore distance) | `Crafting->CanCraftIgnoringStation(recipe)` |
| Per-recipe availability (craft button) | `Crafting->CanCraft(recipe)` |
| Start craft | `Crafting->CraftRecipe(recipe)` / `CraftByRecipeId(id)` |
| Active craft + progress | `Crafting->IsCrafting()`, `GetActiveRecipeId()`, `OnCraftProgress` delegate |
| Completion feedback | `OnCraftCompleted` delegate |
| Ingredient tooltips | `Registry->FindItem(id)` → definition (weight, category, values) |

No additional gameplay hooks are required — the screen is a pure presentation layer over the component.

---

## 7. Not Implemented (honest)

| Feature | Status |
|---|---|
| UMG crafting screen | NOT IMPLEMENTED (station interact stopgap) |
| Multi-slot craft queue | NOT IMPLEMENTED (single active craft by design v1) |
| Craft cancellation / refund mid-craft | NOT IMPLEMENTED (materials are consumed at start) |
| Station visuals | PLACEHOLDER engine cylinders |
| Recipe output to a container (chest) | NOT IMPLEMENTED (outputs go to player inventory) |
