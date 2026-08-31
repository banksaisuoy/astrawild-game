#include "AstrawildInventoryComponent.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"

UAstrawildInventoryComponent::UAstrawildInventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UAstrawildInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAstrawildInventoryComponent, Items);
    DOREPLIFETIME(UAstrawildInventoryComponent, EquippedItemId);
    DOREPLIFETIME(UAstrawildInventoryComponent, EquippedShieldItemId);
    DOREPLIFETIME(UAstrawildInventoryComponent, EquippedArmorItemId);
    // Final production run: the advanced slots replicate so client HUDs/inventory
    // screens mirror the server loadout.
    DOREPLIFETIME(UAstrawildInventoryComponent, EquippedHelmetItemId);
    DOREPLIFETIME(UAstrawildInventoryComponent, EquippedExosuitItemId);
    DOREPLIFETIME(UAstrawildInventoryComponent, EquippedScannerItemId);
}

void UAstrawildInventoryComponent::BeginPlay()
{
    Super::BeginPlay();
}

UAstrawildItemRegistrySubsystem* UAstrawildInventoryComponent::GetRegistry() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
}

bool UAstrawildInventoryComponent::IsValidQuantityRequest(const FName ItemId, const int32 Quantity) const
{
    return !ItemId.IsNone() && Quantity > 0;
}

float UAstrawildInventoryComponent::GetCurrentWeight() const
{
    float Weight = 0.0f;
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        for (const TPair<FName, int32>& Pair : Items)
        {
            if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(Pair.Key))
            {
                Weight += ItemDef->Weight * Pair.Value;
            }
        }
    }
    return Weight;
}

float UAstrawildInventoryComponent::GetWeightFraction() const
{
    const float EffectiveMax = GetEffectiveMaxWeight();
    if (EffectiveMax <= 0.0f)
    {
        return 0.0f;
    }
    return FMath::Clamp(GetCurrentWeight() / EffectiveMax, 0.0f, 1.0f);
}

bool UAstrawildInventoryComponent::CanAddItem(const FName ItemId, const int32 Quantity) const
{
    if (!IsValidQuantityRequest(ItemId, Quantity))
    {
        return false;
    }

    // Final production run: the exosuit carry-weight bonus raises the cap.
    const float EffectiveMax = GetEffectiveMaxWeight();
    if (EffectiveMax <= 0.0f)
    {
        return true;
    }

    float AddedWeight = static_cast<float>(Quantity);
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(ItemId))
        {
            AddedWeight = ItemDef->Weight * Quantity;
        }
    }
    return GetCurrentWeight() + AddedWeight <= EffectiveMax + KINDA_SMALL_NUMBER;
}

bool UAstrawildInventoryComponent::CanAddItemStacks(const TArray<FAstrawildItemStack>& Stacks) const
{
    // H-11: cumulative weight check for a whole stack set (craft outputs).
    // Each stack alone might fit while the set does not — the sum is what
    // actually has to land in the pack.
    float AddedWeight = 0.0f;
    int32 ValidUnits = 0;
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        for (const FAstrawildItemStack& Stack : Stacks)
        {
            if (Stack.Quantity <= 0)
            {
                continue;
            }
            ValidUnits += Stack.Quantity;
            if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(Stack.ItemId))
            {
                AddedWeight += ItemDef->Weight * Stack.Quantity;
            }
            else
            {
                AddedWeight += static_cast<float>(Stack.Quantity); // registry-less fallback (tests)
            }
        }
    }
    else
    {
        for (const FAstrawildItemStack& Stack : Stacks)
        {
            if (Stack.Quantity > 0)
            {
                ValidUnits += Stack.Quantity;
                AddedWeight += static_cast<float>(Stack.Quantity);
            }
        }
    }
    if (ValidUnits <= 0)
    {
        return true; // nothing to add
    }

    const float EffectiveMax = GetEffectiveMaxWeight();
    if (EffectiveMax <= 0.0f)
    {
        return true; // unlimited pack
    }
    return GetCurrentWeight() + AddedWeight <= EffectiveMax + KINDA_SMALL_NUMBER;
}

bool UAstrawildInventoryComponent::AddItem(const FName ItemId, const int32 Quantity)
{
    if (!IsValidQuantityRequest(ItemId, Quantity))
    {
        return false;
    }

    // Weight gate (server authoritative; clients keep a loose copy for UI).
    if (GetOwnerRole() == ROLE_Authority && !CanAddItem(ItemId, Quantity))
    {
        UE_LOG(LogAstrawildEconomy, Verbose, TEXT("AddItem rejected (over weight): %s x%d"), *ItemId.ToString(), Quantity);
        return false;
    }

    int32& Count = Items.FindOrAdd(ItemId);
    Count += Quantity;
    OnInventoryChanged.Broadcast(ItemId, Count);
    BroadcastWeight();

    // Publish collection event for quests/journal (server only, directive §25).
    if (GetOwnerRole() == ROLE_Authority)
    {
        if (UWorld* World = GetWorld())
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_ItemCollected, GetOwner(), ItemId, Quantity, GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector);
            }
        }
    }
    return true;
}

bool UAstrawildInventoryComponent::AddItemSilent(const FName ItemId, const int32 Quantity)
{
    if (!IsValidQuantityRequest(ItemId, Quantity))
    {
        return false;
    }

    if (GetOwnerRole() == ROLE_Authority && !CanAddItem(ItemId, Quantity))
    {
        UE_LOG(LogAstrawildEconomy, Verbose, TEXT("AddItemSilent rejected (over weight): %s x%d"), *ItemId.ToString(), Quantity);
        return false;
    }

    int32& Count = Items.FindOrAdd(ItemId);
    Count += Quantity;
    OnInventoryChanged.Broadcast(ItemId, Count);
    BroadcastWeight();
    // Intentionally NOT publishing TAG_Astrawild_Event_ItemCollected — refunds
    // should not advance CollectItem quest objectives (Batch 2 — Item B).
    return true;
}

bool UAstrawildInventoryComponent::RemoveItem(const FName ItemId, const int32 Quantity)
{
    if (!HasItem(ItemId, Quantity))
    {
        return false;
    }

    int32& Count = Items.FindChecked(ItemId);
    Count -= Quantity;
    if (Count <= 0)
    {
        Items.Remove(ItemId);
    }
    OnInventoryChanged.Broadcast(ItemId, Count);
    BroadcastWeight();
    return true;
}

int32 UAstrawildInventoryComponent::GetQuantity(const FName ItemId) const
{
    const int32* Count = Items.Find(ItemId);
    return Count ? *Count : 0;
}

bool UAstrawildInventoryComponent::HasItem(const FName ItemId, const int32 Quantity) const
{
    return GetQuantity(ItemId) >= Quantity;
}

bool UAstrawildInventoryComponent::ConsumeItems(const TArray<FAstrawildItemStack>& RequiredItems)
{
    for (const FAstrawildItemStack& Required : RequiredItems)
    {
        if (!HasItem(Required.ItemId, Required.Quantity))
        {
            return false;
        }
    }

    for (const FAstrawildItemStack& Required : RequiredItems)
    {
        RemoveItem(Required.ItemId, Required.Quantity);
    }
    return true;
}

TArray<FAstrawildItemStack> UAstrawildInventoryComponent::GetItemStacks() const
{
    TArray<FAstrawildItemStack> Stacks;
    Stacks.Reserve(Items.Num());
    for (const TPair<FName, int32>& Pair : Items)
    {
        if (Pair.Value > 0)
        {
            FAstrawildItemStack Stack;
            Stack.ItemId = Pair.Key;
            Stack.Quantity = Pair.Value;
            Stacks.Add(Stack);
        }
    }
    return Stacks;
}

void UAstrawildInventoryComponent::SetItemStacks(const TArray<FAstrawildItemStack>& InStacks)
{
    Items.Reset();
    for (const FAstrawildItemStack& Stack : InStacks)
    {
        if (Stack.IsValid())
        {
            Items.Add(Stack.ItemId, Stack.Quantity);
        }
    }
    OnInventoryChanged.Broadcast(NAME_None, 0);
    BroadcastWeight();
}

void UAstrawildInventoryComponent::ClearInventory()
{
    Items.Reset();
    OnInventoryChanged.Broadcast(NAME_None, 0);
    BroadcastWeight();
}

bool UAstrawildInventoryComponent::EquipItem(const FName ItemId)
{
    if (!HasItem(ItemId, 1))
    {
        return false;
    }

    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        const UAstrawildItemDefinition* ItemDef = Registry->FindItem(ItemId);
        if (ItemDef && ItemDef->Category == EAstrawildItemCategory::Equipment)
        {
            // Final production run (PHASE 12): explicit slot routing wins first —
            // advanced items (helmet/exosuit/scanner/ranged weapons) declare their
            // slot; Auto keeps the legacy stat-based routing below.
            switch (ItemDef->EquipmentSlot)
            {
            case EAstrawildEquipmentSlot::Helmet:
                {
                    const FName Previous = EquippedHelmetItemId;
                    EquippedHelmetItemId = ItemId;
                    if (Previous != ItemId)
                    {
                        OnSlotChanged.Broadcast(EAstrawildEquipmentSlot::Helmet, ItemId);
                    }
                    OnEquipmentChanged.Broadcast(EquippedItemId, EquippedShieldItemId);
                    return true;
                }
            case EAstrawildEquipmentSlot::Exosuit:
                {
                    const FName Previous = EquippedExosuitItemId;
                    EquippedExosuitItemId = ItemId;
                    if (Previous != ItemId)
                    {
                        OnSlotChanged.Broadcast(EAstrawildEquipmentSlot::Exosuit, ItemId);
                    }
                    OnEquipmentChanged.Broadcast(EquippedItemId, EquippedShieldItemId);
                    return true;
                }
            case EAstrawildEquipmentSlot::Scanner:
                {
                    const FName Previous = EquippedScannerItemId;
                    EquippedScannerItemId = ItemId;
                    if (Previous != ItemId)
                    {
                        OnSlotChanged.Broadcast(EAstrawildEquipmentSlot::Scanner, ItemId);
                    }
                    OnEquipmentChanged.Broadcast(EquippedItemId, EquippedShieldItemId);
                    return true;
                }
            case EAstrawildEquipmentSlot::Weapon:
            case EAstrawildEquipmentSlot::Shield:
            case EAstrawildEquipmentSlot::Torso:
            case EAstrawildEquipmentSlot::Auto:
            default:
                break;
            }

            // Wave 3 routing: attack items are weapons, mitigation items are shields.
            if (ItemDef->AttackPower > 0.0f)
            {
                EquippedItemId = ItemId;
            }
            else if (ItemDef->BlockMitigation > 0.0f)
            {
                EquippedShieldItemId = ItemId;
            }
            // Batch 3 — Item C: torso armor routes to its own slot (must come before
            // the statless legacy fallback below).
            else if (ItemDef->ArmorRating > 0.0f)
            {
                const FName Previous = EquippedArmorItemId;
                EquippedArmorItemId = ItemId;
                if (Previous != ItemId)
                {
                    OnArmorChanged.Broadcast(EquippedArmorItemId);
                }
            }
            else
            {
                // Statless equipment keeps the legacy weapon slot behaviour.
                EquippedItemId = ItemId;
            }
            OnEquipmentChanged.Broadcast(EquippedItemId, EquippedShieldItemId);
            return true;
        }
    }
    return false;
}

void UAstrawildInventoryComponent::Unequip()
{
    EquippedItemId = NAME_None;
    EquippedShieldItemId = NAME_None;
    // Batch 3 — Item C: clear the armor slot too.
    if (!EquippedArmorItemId.IsNone())
    {
        EquippedArmorItemId = NAME_None;
        OnArmorChanged.Broadcast(NAME_None);
    }
    // Final production run: clear the advanced slots as well.
    if (!EquippedHelmetItemId.IsNone())
    {
        EquippedHelmetItemId = NAME_None;
        OnSlotChanged.Broadcast(EAstrawildEquipmentSlot::Helmet, NAME_None);
    }
    if (!EquippedExosuitItemId.IsNone())
    {
        EquippedExosuitItemId = NAME_None;
        OnSlotChanged.Broadcast(EAstrawildEquipmentSlot::Exosuit, NAME_None);
    }
    if (!EquippedScannerItemId.IsNone())
    {
        EquippedScannerItemId = NAME_None;
        OnSlotChanged.Broadcast(EAstrawildEquipmentSlot::Scanner, NAME_None);
    }
    OnEquipmentChanged.Broadcast(EquippedItemId, EquippedShieldItemId);
}

float UAstrawildInventoryComponent::GetEquippedWeaponAttackPower() const
{
    if (EquippedItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedItemId))
        {
            return ItemDef->AttackPower;
        }
    }
    return 0.0f;
}

float UAstrawildInventoryComponent::GetEquippedShieldMitigation() const
{
    if (EquippedShieldItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedShieldItemId))
        {
            return ItemDef->BlockMitigation;
        }
    }
    return 0.0f;
}

float UAstrawildInventoryComponent::GetEquippedArmorRating() const
{
    // Batch 3 — Item C: armor rating for the combat component's damage-reduction formula.
    if (EquippedArmorItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedArmorItemId))
        {
            return ItemDef->ArmorRating;
        }
    }
    return 0.0f;
}

EAstrawildElementType UAstrawildInventoryComponent::GetEquippedWeaponElement() const
{
    // Batch 3 — Item A: the equipped weapon's element overrides the combat tunable.
    if (EquippedItemId.IsNone())
    {
        return EAstrawildElementType::None;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedItemId))
        {
            return ItemDef->Element;
        }
    }
    return EAstrawildElementType::None;
}

void UAstrawildInventoryComponent::BroadcastWeight()
{
    OnWeightChanged.Broadcast(GetCurrentWeight());
}

// --- Final production run (PHASE 12): advanced-equipment queries ---

float UAstrawildInventoryComponent::GetEquippedHelmetArmorRating() const
{
    if (EquippedHelmetItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedHelmetItemId))
        {
            return ItemDef->ArmorRating;
        }
    }
    return 0.0f;
}

float UAstrawildInventoryComponent::GetTotalArmorRating() const
{
    // Torso + helmet feed the same diminishing-returns formula (rating adds up).
    return GetEquippedArmorRating() + GetEquippedHelmetArmorRating();
}

float UAstrawildInventoryComponent::GetEquippedInsulationRating() const
{
    float Total = 0.0f;
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (!EquippedHelmetItemId.IsNone())
        {
            if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedHelmetItemId))
            {
                Total += ItemDef->InsulationRating;
            }
        }
        if (!EquippedExosuitItemId.IsNone())
        {
            if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedExosuitItemId))
            {
                Total += ItemDef->InsulationRating;
            }
        }
    }
    return Total;
}

float UAstrawildInventoryComponent::GetEquippedStaminaRegenBonus() const
{
    if (EquippedExosuitItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedExosuitItemId))
        {
            return ItemDef->StaminaRegenBonus;
        }
    }
    return 0.0f;
}

float UAstrawildInventoryComponent::GetEquippedCarryWeightBonus() const
{
    if (EquippedExosuitItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedExosuitItemId))
        {
            return ItemDef->CarryWeightBonus;
        }
    }
    return 0.0f;
}

float UAstrawildInventoryComponent::GetEquippedMoveSpeedBonus() const
{
    if (EquippedExosuitItemId.IsNone())
    {
        return 0.0f;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedExosuitItemId))
        {
            return ItemDef->MoveSpeedBonus;
        }
    }
    return 0.0f;
}

bool UAstrawildInventoryComponent::IsRangedWeaponEquipped() const
{
    if (EquippedItemId.IsNone())
    {
        return false;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedItemId))
        {
            return ItemDef->bIsRangedWeapon;
        }
    }
    return false;
}

FName UAstrawildInventoryComponent::GetEquippedAmmoItemId() const
{
    if (EquippedItemId.IsNone())
    {
        return NAME_None;
    }
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedItemId))
        {
            return ItemDef->AmmoItemId;
        }
    }
    return NAME_None;
}

float UAstrawildInventoryComponent::GetEffectiveMaxWeight() const
{
    // Production V2 (Master Plan §6): Pack Instinct party passive — a healthy
    // captured Echo with the CarryBoost aura nearby adds carry capacity (+20kg).
    float Weight = MaxWeight + GetEquippedCarryWeightBonus();
    if (AAstrawildEchoCharacter::HasPlayerPartyPassive(GetWorld(), GetOwner(), EAstrawildEchoPassive::CarryBoost, 1500.0f))
    {
        Weight += 20.0f;
    }
    return Weight;
}

// --- Production V2 (additive): weapon profiles, split insulation, scanner tiers ---

UAstrawildWeaponDefinition* UAstrawildInventoryComponent::GetEquippedWeaponDefinition() const
{
    if (EquippedItemId.IsNone())
    {
        return nullptr;
    }
    const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(EquippedItemId) : nullptr)
    {
        return ItemDef->WeaponDefinitionId.IsNone()
            ? nullptr
            : Registry->FindWeapon(ItemDef->WeaponDefinitionId);
    }
    return nullptr;
}

float UAstrawildInventoryComponent::GetEquippedColdInsulationRating() const
{
    // Split-band insulation (Master Plan §9): dedicated cold/heat fields win;
    // the legacy InsulationRating still counts on BOTH sides so every existing
    // armor piece keeps its documented behaviour.
    float Total = 0.0f;
    const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry)
    {
        return 0.0f;
    }
    const auto AddPiece = [&Total, Registry](const FName ItemId)
    {
        if (ItemId.IsNone())
        {
            return;
        }
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(ItemId))
        {
            Total += ItemDef->ColdInsulationRating > 0.0f ? ItemDef->ColdInsulationRating : ItemDef->InsulationRating;
        }
    };
    AddPiece(EquippedHelmetItemId);
    AddPiece(EquippedExosuitItemId);
    AddPiece(EquippedArmorItemId);
    return Total;
}

float UAstrawildInventoryComponent::GetEquippedHeatInsulationRating() const
{
    float Total = 0.0f;
    const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (!Registry)
    {
        return 0.0f;
    }
    const auto AddPiece = [&Total, Registry](const FName ItemId)
    {
        if (ItemId.IsNone())
        {
            return;
        }
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(ItemId))
        {
            Total += ItemDef->HeatInsulationRating > 0.0f ? ItemDef->HeatInsulationRating : ItemDef->InsulationRating;
        }
    };
    AddPiece(EquippedHelmetItemId);
    AddPiece(EquippedExosuitItemId);
    AddPiece(EquippedArmorItemId);
    return Total;
}

float UAstrawildInventoryComponent::GetEquippedScannerRangeMultiplier() const
{
    if (EquippedScannerItemId.IsNone())
    {
        return 1.0f;
    }
    const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    if (const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(EquippedScannerItemId) : nullptr)
    {
        return FMath::Clamp(ItemDef->ScannerRangeMultiplier, 1.0f, 4.0f);
    }
    return 1.0f;
}

bool UAstrawildInventoryComponent::HasHiddenResourceDetection() const
{
    if (EquippedScannerItemId.IsNone())
    {
        return false;
    }
    const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(EquippedScannerItemId) : nullptr;
    return ItemDef && ItemDef->bHiddenResourceDetection;
}

bool UAstrawildInventoryComponent::HasAncientSignalTracking() const
{
    if (EquippedScannerItemId.IsNone())
    {
        return false;
    }
    const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(EquippedScannerItemId) : nullptr;
    return ItemDef && ItemDef->bAncientSignalTracking;
}

EAstrawildRarity UAstrawildInventoryComponent::GetEquippedWeaponRarity() const
{
    if (const UAstrawildWeaponDefinition* WeaponDef = GetEquippedWeaponDefinition())
    {
        return WeaponDef->Rarity;
    }
    if (!EquippedItemId.IsNone())
    {
        if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
        {
            if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(EquippedItemId))
            {
                return ItemDef->Rarity;
            }
        }
    }
    return EAstrawildRarity::Common;
}
