#include "AstrawildDurabilityComponent.h"

#include "AstrawildDataAssets.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UAstrawildDurabilityComponent::UAstrawildDurabilityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

UAstrawildInventoryComponent* UAstrawildDurabilityComponent::GetOwnerInventory() const
{
    const AActor* Owner = GetOwner();
    if (!IsValid(Owner))
    {
        return nullptr;
    }
    return Owner->FindComponentByClass<UAstrawildInventoryComponent>();
}

UAstrawildItemRegistrySubsystem* UAstrawildDurabilityComponent::GetRegistry() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    return World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
}

TArray<FName> UAstrawildDurabilityComponent::GetTrackedEquippedItemIds() const
{
    TArray<FName> Equipped;

    const UAstrawildInventoryComponent* Inventory = GetOwnerInventory();
    if (!Inventory)
    {
        return Equipped;
    }

    // Every slot the player can wear — weapon, shield, torso, helmet, exosuit.
    // (Scanner excluded: observation tools carry no combat wear.)
    Equipped.Add(Inventory->EquippedItemId);
    Equipped.Add(Inventory->EquippedShieldItemId);
    Equipped.Add(Inventory->EquippedArmorItemId);
    Equipped.Add(Inventory->EquippedHelmetItemId);
    Equipped.Add(Inventory->EquippedExosuitItemId);

    return Equipped;
}

float UAstrawildDurabilityComponent::WearItem(FName ItemId, float Amount)
{
    if (ItemId.IsNone() || Amount <= 0.0f)
    {
        return 0.0f;
    }

    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry)
    {
        return 0.0f;
    }

    const UAstrawildItemDefinition* Item = Registry->FindItem(ItemId);
    if (!Item || Item->DurabilityMax <= 0.0f)
    {
        // Untracked items never wear.
        return 0.0f;
    }

    float* Pool = DurabilityPools.Find(ItemId);
    if (!Pool)
    {
        // First wear initializes the full pool, then applies the amount.
        Pool = &DurabilityPools.Add(ItemId, Item->DurabilityMax);
    }

    const float Before = *Pool;
    *Pool = FMath::Clamp(*Pool - Amount, 0.0f, Item->DurabilityMax);

    if (Before > 0.0f && *Pool <= 0.0f)
    {
        // Crossing into broken — broadcast for the HUD warning + audio sting.
        UE_LOG(LogAstrawild, Log, TEXT("Durability: %s broke (%.0f pool)"), *ItemId.ToString(), Item->DurabilityMax);
        OnItemBroke.Broadcast(ItemId, Item->DurabilityMax);
    }

    return *Pool;
}

void UAstrawildDurabilityComponent::ApplyWeaponWear()
{
    const UAstrawildInventoryComponent* Inventory = GetOwnerInventory();
    if (!Inventory)
    {
        return;
    }
    CachedEquippedWeaponId = Inventory->EquippedItemId;
    WearItem(Inventory->EquippedItemId, 1.0f);
}

void UAstrawildDurabilityComponent::ApplyToolWear()
{
    // The equipped weapon doubles as the harvest tool (pick/axe/sickle family).
    ApplyWeaponWear();
}

void UAstrawildDurabilityComponent::ApplyArmorWear()
{
    // FCR-1-c fix (L-c16): armor wear covers PROTECTIVE slots only — the tracked
    // list includes the WEAPON, which takes hits at its own wear site; taking a
    // hit used to double-wear the weapon in the same fight.
    for (const FName ItemId : GetTrackedEquippedItemIds())
    {
        const UAstrawildInventoryComponent* Inventory = GetOwnerInventory();
        if (Inventory && ItemId == Inventory->EquippedItemId)
        {
            continue; // weapon wears on CONNECTED HITS, not on damage taken
        }
        // Shield wear follows the same rule (it is worn protective gear).
        WearItem(ItemId, 1.0f);
    }
}

bool UAstrawildDurabilityComponent::IsItemBroken(FName ItemId) const
{
    if (ItemId.IsNone())
    {
        return false;
    }

    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry)
    {
        return false;
    }

    const UAstrawildItemDefinition* Item = Registry->FindItem(ItemId);
    if (!Item || Item->DurabilityMax <= 0.0f)
    {
        return false;
    }

    const float* Pool = DurabilityPools.Find(ItemId);
    return Pool && *Pool <= 0.0f;
}

float UAstrawildDurabilityComponent::GetDurability(FName ItemId) const
{
    const float* Pool = DurabilityPools.Find(ItemId);
    return Pool ? *Pool : 0.0f;
}

float UAstrawildDurabilityComponent::GetDurabilityFraction(FName ItemId) const
{
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry)
    {
        return 1.0f;
    }

    const UAstrawildItemDefinition* Item = Registry->FindItem(ItemId);
    if (!Item || Item->DurabilityMax <= 0.0f)
    {
        return 1.0f;
    }

    return FMath::Clamp(GetDurability(ItemId) / Item->DurabilityMax, 0.0f, 1.0f);
}

float UAstrawildDurabilityComponent::GetEquippedWeaponDamageMultiplier() const
{
    const UAstrawildInventoryComponent* Inventory = GetOwnerInventory();
    if (!Inventory || Inventory->EquippedItemId.IsNone())
    {
        return 1.0f;
    }

    if (!IsItemBroken(Inventory->EquippedItemId))
    {
        return 1.0f;
    }

    // Broken weapons still swing — they just hit at 40% (directive Phase 12).
    return BrokenWeaponDamageMultiplier;
}

float UAstrawildDurabilityComponent::GetHarvestYieldMultiplier(FName ResourceItemId) const
{
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry)
    {
        return 1.0f;
    }

    const UAstrawildItemDefinition* Resource = Registry->FindItem(ResourceItemId);
    if (!Resource || Resource->HarvestCategory.IsNone())
    {
        return 1.0f;
    }

    const UAstrawildInventoryComponent* Inventory = GetOwnerInventory();
    if (!Inventory || Inventory->EquippedItemId.IsNone())
    {
        return 1.0f;
    }

    const UAstrawildItemDefinition* Tool = Registry->FindItem(Inventory->EquippedItemId);
    if (!Tool || Tool->HarvestBonusCategory.IsNone())
    {
        return 1.0f;
    }

    // Specialization: category match + healthy tool grants the multiplier.
    if (Tool->HarvestBonusCategory == Resource->HarvestCategory && !IsItemBroken(Tool->ItemId))
    {
        return FMath::Clamp(Tool->HarvestMultiplier, 1.0f, 10.0f);
    }

    return 1.0f;
}

const UAstrawildRecipeDefinition* UAstrawildDurabilityComponent::FindRecipeForOutput(FName ItemId) const
{
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry || ItemId.IsNone())
    {
        return nullptr;
    }

    for (const UAstrawildRecipeDefinition* Recipe : Registry->GetAllRecipes())
    {
        if (!Recipe)
        {
            continue;
        }
        for (const FAstrawildItemStack& Output : Recipe->Outputs)
        {
            if (Output.ItemId == ItemId)
            {
                return Recipe;
            }
        }
    }
    return nullptr;
}

FName UAstrawildDurabilityComponent::RepairItem(FName ItemId, bool bAtRepairBench)
{
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    UAstrawildInventoryComponent* Inventory = GetOwnerInventory();
    if (!Registry || !Inventory || ItemId.IsNone())
    {
        return NAME_None;
    }

    const UAstrawildItemDefinition* Item = Registry->FindItem(ItemId);
    if (!Item || Item->DurabilityMax <= 0.0f)
    {
        return NAME_None;
    }

    if (GetDurability(ItemId) >= Item->DurabilityMax)
    {
        // Already pristine — nothing to do, nothing to charge.
        return NAME_None;
    }

    // Bench path: 40% (ceil) of the item's craft inputs.
    if (bAtRepairBench)
    {
        const UAstrawildRecipeDefinition* Recipe = FindRecipeForOutput(ItemId);
        if (Recipe && !Recipe->Ingredients.IsEmpty())
        {
            TArray<FAstrawildItemStack> Cost;
            bool bCanAfford = true;
            for (const FAstrawildItemStack& Input : Recipe->Ingredients)
            {
                FAstrawildItemStack Required;
                Required.ItemId = Input.ItemId;
                Required.Quantity = FMath::Max(1, FMath::CeilToInt(Input.Quantity * BenchRepairCostFraction));
                Cost.Add(Required);
                if (!Inventory->HasItem(Required.ItemId, Required.Quantity))
                {
                    bCanAfford = false;
                }
            }

            if (bCanAfford && Inventory->ConsumeItems(Cost))
            {
                DurabilityPools.Add(ItemId, Item->DurabilityMax);
                UE_LOG(LogAstrawild, Log, TEXT("Durability: %s repaired at bench for %d ingredient lines"),
                    *ItemId.ToString(), Cost.Num());
                return TEXT("RepairBenchMaterials");
            }
        }
    }

    // Field path: one repair kit restores any single item to full.
    // FCR-1-c fix (L-c15): at a bench, the kit is NOT silently consumed — the
    // bench ran out of affordable materials and the player never consented to
    // spending a scarce field kit. Standing at a bench with no materials now
    // simply fails (the player can gather or use a kit deliberately in the field).
    static const FName RepairKitId = TEXT("Item_FieldRepairKit");
    if (!bAtRepairBench && Inventory->HasItem(RepairKitId, 1) && Inventory->RemoveItem(RepairKitId, 1))
    {
        DurabilityPools.Add(ItemId, Item->DurabilityMax);
        UE_LOG(LogAstrawild, Log, TEXT("Durability: %s repaired with a field kit"), *ItemId.ToString());
        return RepairKitId;
    }

    return NAME_None;
}

TMap<FName, float> UAstrawildDurabilityComponent::ExportForSave() const
{
    TMap<FName, float> Out;
    for (const TPair<FName, float>& Pair : DurabilityPools)
    {
        // Never persist pristine/full pools — keeps legacy saves additive-clean.
        if (Pair.Value > 0.0f)
        {
            Out.Add(Pair.Key, Pair.Value);
        }
    }
    return Out;
}

void UAstrawildDurabilityComponent::ImportFromSave(const TMap<FName, float>& InDurability)
{
    DurabilityPools.Reset();

    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry)
    {
        return;
    }

    for (const TPair<FName, float>& Pair : InDurability)
    {
        const UAstrawildItemDefinition* Item = Registry->FindItem(Pair.Key);
        if (!Item || Item->DurabilityMax <= 0.0f)
        {
            // Sanitize: unknown/untracked ids from edited saves are dropped.
            continue;
        }
        DurabilityPools.Add(Pair.Key, FMath::Clamp(Pair.Value, 0.0f, Item->DurabilityMax));
    }
}
