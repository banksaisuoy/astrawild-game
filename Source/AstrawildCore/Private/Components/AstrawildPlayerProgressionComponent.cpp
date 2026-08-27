#include "Components/AstrawildPlayerProgressionComponent.h"

#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

UAstrawildPlayerProgressionComponent::UAstrawildPlayerProgressionComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

int32 UAstrawildPlayerProgressionComponent::AddExperience(const float ExperienceAmount)
{
    if (!HasAuthorityForProgression() || ExperienceAmount <= 0.0f)
    {
        return 0;
    }
    int32 levelsGained = 0;
    Experience += ExperienceAmount;
    while (Level < FMath::Max(1, MaxLevel))
    {
        const float threshold = ExperienceForFirstLevel * FMath::Pow(static_cast<float>(Level), 1.35f);
        if (Experience < threshold)
        {
            break;
        }
        Experience -= threshold;
        ++Level;
        ++UnspentStatPoints;
        ++levelsGained;
    }
    if (levelsGained > 0)
    {
        OnLevelChanged.Broadcast(Level, UnspentStatPoints);
    }
    return levelsGained;
}

bool UAstrawildPlayerProgressionComponent::AllocateStatPoint(const EAstrawildPlayerStat StatType)
{
    if (!HasAuthorityForProgression() || UnspentStatPoints <= 0)
    {
        return false;
    }
    AllocatedStatPoints.FindOrAdd(StatType)++;
    --UnspentStatPoints;
    OnLevelChanged.Broadcast(Level, UnspentStatPoints);
    return true;
}

bool UAstrawildPlayerProgressionComponent::UnlockPerk(const FGameplayTag PerkTag)
{
    if (!HasAuthorityForProgression() || !PerkTag.IsValid() || IsPerkUnlocked(PerkTag))
    {
        return false;
    }
    const FAstrawildPlayerPerkRow* Perk = FindPerk(PerkTag);
    if (!Perk || Level < Perk->Tier)
    {
        return false;
    }
    for (const FGameplayTag& prerequisite : Perk->PrerequisitePerkTags)
    {
        if (prerequisite.IsValid() && !IsPerkUnlocked(prerequisite))
        {
            return false;
        }
    }
    UnlockedPerkTags.Add(PerkTag);
    OnPerkUnlocked.Broadcast(PerkTag);
    return true;
}

bool UAstrawildPlayerProgressionComponent::IsPerkUnlocked(const FGameplayTag PerkTag) const
{
    return UnlockedPerkTags.Contains(PerkTag);
}

int32 UAstrawildPlayerProgressionComponent::GetAllocatedStatPoints(const EAstrawildPlayerStat StatType) const
{
    return AllocatedStatPoints.FindRef(StatType);
}

float UAstrawildPlayerProgressionComponent::GetPerkModifier(const FGameplayTag PerkTag, const FName ModifierName) const
{
    const FAstrawildPlayerPerkRow* Perk = FindPerk(PerkTag);
    if (!Perk || !IsPerkUnlocked(PerkTag))
    {
        return ModifierName == TEXT("SprintStaminaMultiplier") || ModifierName == TEXT("ReloadSpeedMultiplier") || ModifierName == TEXT("CriticalDamageMultiplier") || ModifierName == TEXT("FoodNutritionMultiplier") ? 1.0f : 0.0f;
    }
    if (ModifierName == TEXT("StatBonus")) return Perk->StatBonus;
    if (ModifierName == TEXT("SprintStaminaMultiplier")) return Perk->SprintStaminaMultiplier;
    if (ModifierName == TEXT("FoodNutritionMultiplier")) return Perk->FoodNutritionMultiplier;
    if (ModifierName == TEXT("RepairRefundMultiplier")) return Perk->RepairRefundMultiplier;
    if (ModifierName == TEXT("CaptureOddsBonus")) return Perk->CaptureOddsBonus;
    if (ModifierName == TEXT("ReloadSpeedMultiplier")) return Perk->ReloadSpeedMultiplier;
    if (ModifierName == TEXT("CriticalDamageMultiplier")) return Perk->CriticalDamageMultiplier;
    return 0.0f;
}

const FAstrawildPlayerPerkRow* UAstrawildPlayerProgressionComponent::FindPerk(const FGameplayTag PerkTag) const
{
    if (!PerkTable || !PerkTag.IsValid())
    {
        return nullptr;
    }
    TArray<FAstrawildPlayerPerkRow*> rows;
    PerkTable->GetAllRows<FAstrawildPlayerPerkRow>(TEXT("AstrawildPerkLookup"), rows);
    return rows.FindByPredicate([PerkTag](const FAstrawildPlayerPerkRow* row)
    {
        return row && row->PerkTag == PerkTag;
    });
}

bool UAstrawildPlayerProgressionComponent::HasAuthorityForProgression() const
{
    return !GetOwner() || GetOwner()->HasAuthority();
}
