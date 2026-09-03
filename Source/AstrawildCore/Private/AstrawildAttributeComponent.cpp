#include "AstrawildAttributeComponent.h"

#include "AstrawildLog.h"
#include "GameFramework/Pawn.h"

UAstrawildAttributeComponent::UAstrawildAttributeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

// Replication note: raw stats are read server-side by the owning player's
// systems and by the pawn-local HUD copy (single-player/listen-server first).
// The dedicated-server co-op pass (CV-6, P3 by design) will add condensed
// snapshot replication — the save schema and query surface already fit it.

FAstrawildAttributeStat* UAstrawildAttributeComponent::GetStat(const EAstrawildAttributeType Attribute)
{
    switch (Attribute)
    {
    case EAstrawildAttributeType::Might:    return &Might;
    case EAstrawildAttributeType::Vigor:    return &Vigor;
    case EAstrawildAttributeType::Agility:  return &Agility;
    case EAstrawildAttributeType::Instinct: return &Instinct;
    case EAstrawildAttributeType::Craft:    return &Craft;
    default: return nullptr;
    }
}

const FAstrawildAttributeStat* UAstrawildAttributeComponent::GetStat(const EAstrawildAttributeType Attribute) const
{
    return const_cast<UAstrawildAttributeComponent*>(this)->GetStat(Attribute);
}

void UAstrawildAttributeComponent::AddAttributeXP(const EAstrawildAttributeType Attribute, const float Amount)
{
    if (Amount <= 0.0f)
    {
        return; // No negative/zero XP ever.
    }
    // Server-authoritative when owned (networked play); ownerless components
    // (automation tests, transient tools) pass straight through.
    if (GetOwner() && GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    FAstrawildAttributeStat* Stat = GetStat(Attribute);
    if (!Stat)
    {
        return;
    }
    if (Stat->Level >= MaxAttributeLevel)
    {
        return; // Cap reached — XP no longer accumulates.
    }

    Stat->XP += Amount;
    while (Stat->Level < MaxAttributeLevel && Stat->XP >= BaseXPPerLevel * Stat->Level)
    {
        Stat->XP -= BaseXPPerLevel * Stat->Level;
        Stat->Level++;
        OnAttributeLevelUp.Broadcast(Attribute, Stat->Level);
        UE_LOG(LogAstrawild, Log, TEXT("Attribute level up: %d -> %d (%s)."),
            static_cast<int32>(Attribute), Stat->Level, *GetNameSafe(GetOwner()));
    }
    if (Stat->Level >= MaxAttributeLevel)
    {
        Stat->XP = 0.0f; // Cap: tidy the bar instead of leaving overflow residue.
    }
}

int32 UAstrawildAttributeComponent::GetLevel(const EAstrawildAttributeType Attribute) const
{
    const FAstrawildAttributeStat* Stat = GetStat(Attribute);
    return Stat ? Stat->Level : 1;
}

float UAstrawildAttributeComponent::GetXP(const EAstrawildAttributeType Attribute) const
{
    const FAstrawildAttributeStat* Stat = GetStat(Attribute);
    return Stat ? Stat->XP : 0.0f;
}

float UAstrawildAttributeComponent::GetXPToNextLevel(const EAstrawildAttributeType Attribute) const
{
    const FAstrawildAttributeStat* Stat = GetStat(Attribute);
    if (!Stat || Stat->Level >= MaxAttributeLevel)
    {
        return 0.0f;
    }
    return FMath::Max(1.0f, BaseXPPerLevel * Stat->Level - Stat->XP);
}

float UAstrawildAttributeComponent::GetMeleeDamageMultiplier() const
{
    return 1.0f + 0.04f * (Might.Level - 1);
}

float UAstrawildAttributeComponent::GetMaxHealthMultiplier() const
{
    return 1.0f + 0.05f * (Vigor.Level - 1);
}

float UAstrawildAttributeComponent::GetStaminaRegenMultiplier() const
{
    return 1.0f + 0.04f * (Agility.Level - 1);
}

float UAstrawildAttributeComponent::GetMoveSpeedMultiplier() const
{
    return 1.0f + 0.02f * (Agility.Level - 1);
}

float UAstrawildAttributeComponent::GetCaptureChanceBonus() const
{
    return 0.015f * (Instinct.Level - 1);
}

float UAstrawildAttributeComponent::GetCraftSpeedMultiplier() const
{
    return 1.0f + 0.04f * (Craft.Level - 1);
}

float UAstrawildAttributeComponent::GetMasterworkRefundChance() const
{
    return IsSkillUnlockedByAttributes(EAstrawildPlayerSkillId::Masterwork,
        Might.Level, Vigor.Level, Agility.Level, Instinct.Level, Craft.Level) ? 0.15f : 0.0f;
}

bool UAstrawildAttributeComponent::IsSkillUnlockedByAttributes(const EAstrawildPlayerSkillId Skill,
    const int32 MightLevel, const int32 VigorLevel, const int32 AgilityLevel,
    const int32 InstinctLevel, const int32 CraftLevel)
{
    switch (Skill)
    {
    case EAstrawildPlayerSkillId::PowerStrike:  return MightLevel >= 3;
    case EAstrawildPlayerSkillId::Whirlwind:    return MightLevel >= 6;
    case EAstrawildPlayerSkillId::Dash:         return AgilityLevel >= 3;
    case EAstrawildPlayerSkillId::SecondWind:   return VigorLevel >= 4;
    case EAstrawildPlayerSkillId::HuntersFocus: return InstinctLevel >= 4;
    case EAstrawildPlayerSkillId::Masterwork:   return CraftLevel >= 5;
    case EAstrawildPlayerSkillId::Overcharge:   return InstinctLevel >= 7;
    case EAstrawildPlayerSkillId::None:
    default: return false;
    }
}

float UAstrawildAttributeComponent::GetSkillCooldown(const EAstrawildPlayerSkillId Skill)
{
    switch (Skill)
    {
    case EAstrawildPlayerSkillId::PowerStrike:  return 8.0f;
    case EAstrawildPlayerSkillId::Whirlwind:    return 14.0f;
    case EAstrawildPlayerSkillId::Dash:         return 6.0f;
    case EAstrawildPlayerSkillId::SecondWind:   return 30.0f;
    case EAstrawildPlayerSkillId::HuntersFocus: return 25.0f;
    case EAstrawildPlayerSkillId::Masterwork:   return 0.0f;   // Pure passive.
    case EAstrawildPlayerSkillId::Overcharge:   return 18.0f;
    case EAstrawildPlayerSkillId::None:
    default: return 0.0f;
    }
}

float UAstrawildAttributeComponent::GetSkillCooldownRemaining(const EAstrawildPlayerSkillId Skill) const
{
    const float* Remaining = SkillCooldowns.Find(Skill);
    return Remaining ? FMath::Max(0.0f, *Remaining) : 0.0f;
}

TArray<EAstrawildPlayerSkillId> UAstrawildAttributeComponent::GetUnlockedSkills() const
{
    TArray<EAstrawildPlayerSkillId> Unlocked;
    for (int32 i = 1; i <= static_cast<int32>(EAstrawildPlayerSkillId::Overcharge); ++i)
    {
        const EAstrawildPlayerSkillId Skill = static_cast<EAstrawildPlayerSkillId>(i);
        if (IsSkillUnlockedByAttributes(Skill, Might.Level, Vigor.Level, Agility.Level, Instinct.Level, Craft.Level))
        {
            Unlocked.Add(Skill);
        }
    }
    return Unlocked;
}

EAstrawildPlayerSkillId UAstrawildAttributeComponent::PickBestReadySkill(const float HealthFraction,
    const int32 NearbyEnemies, const bool bEnemyInMelee, const bool bMoving) const
{
    auto Ready = [this](const EAstrawildPlayerSkillId Skill)
    {
        return IsSkillUnlockedByAttributes(Skill, Might.Level, Vigor.Level, Agility.Level,
            Instinct.Level, Craft.Level) && GetSkillCooldownRemaining(Skill) <= 0.0f;
    };

    // Deterministic priority ladder — the smart-cast reads the battlefield.
    if (HealthFraction < 0.35f && Ready(EAstrawildPlayerSkillId::SecondWind))
    {
        return EAstrawildPlayerSkillId::SecondWind;
    }
    if (NearbyEnemies >= 3 && Ready(EAstrawildPlayerSkillId::Whirlwind))
    {
        return EAstrawildPlayerSkillId::Whirlwind;
    }
    if (bEnemyInMelee && Ready(EAstrawildPlayerSkillId::PowerStrike))
    {
        return EAstrawildPlayerSkillId::PowerStrike;
    }
    if (Ready(EAstrawildPlayerSkillId::HuntersFocus) && !bEnemyInMelee && NearbyEnemies == 0)
    {
        return EAstrawildPlayerSkillId::HuntersFocus;
    }
    if (bMoving && Ready(EAstrawildPlayerSkillId::Dash))
    {
        return EAstrawildPlayerSkillId::Dash;
    }
    if (Ready(EAstrawildPlayerSkillId::Overcharge) && bEnemyInMelee)
    {
        return EAstrawildPlayerSkillId::Overcharge;
    }
    return EAstrawildPlayerSkillId::None;
}

void UAstrawildAttributeComponent::TickCooldowns(const float DeltaSeconds)
{
    if (SkillCooldowns.IsEmpty())
    {
        return;
    }
    for (TPair<EAstrawildPlayerSkillId, float>& Pair : SkillCooldowns)
    {
        Pair.Value = FMath::Max(0.0f, Pair.Value - DeltaSeconds);
    }
    for (auto It = SkillCooldowns.CreateIterator(); It; ++It)
    {
        if (It->Value <= 0.0f)
        {
            It.RemoveCurrent();
        }
    }
}

TArray<FAstrawildAttributeSaveData> UAstrawildAttributeComponent::ToSaveData() const
{
    TArray<FAstrawildAttributeSaveData> Data;
    auto Push = [&Data](const EAstrawildAttributeType Type, const FAstrawildAttributeStat& Stat)
    {
        FAstrawildAttributeSaveData Row;
        Row.Type = Type;
        Row.Level = FMath::Clamp(Stat.Level, 1, MaxAttributeLevel);
        Row.XP = FMath::Max(0.0f, Stat.XP);
        Data.Add(Row);
    };
    Push(EAstrawildAttributeType::Might, Might);
    Push(EAstrawildAttributeType::Vigor, Vigor);
    Push(EAstrawildAttributeType::Agility, Agility);
    Push(EAstrawildAttributeType::Instinct, Instinct);
    Push(EAstrawildAttributeType::Craft, Craft);
    return Data;
}

int32 UAstrawildAttributeComponent::ImportFromSaveData(const TArray<FAstrawildAttributeSaveData>& Data)
{
    int32 Repairs = 0;
    TSet<EAstrawildAttributeType> Seen;

    for (const FAstrawildAttributeSaveData& Row : Data)
    {
        if (Seen.Contains(Row.Type))
        {
            Repairs++; // Duplicate row — first-seen-wins (same rule as research import).
            continue;
        }
        Seen.Add(Row.Type);

        FAstrawildAttributeStat* Stat = GetStat(Row.Type);
        if (!Stat)
        {
            continue;
        }
        const int32 ClampedLevel = FMath::Clamp(Row.Level, 1, MaxAttributeLevel);
        const float ClampedXP = FMath::Clamp(Row.XP, 0.0f, BaseXPPerLevel * ClampedLevel);
        if (ClampedLevel != Row.Level || ClampedXP != Row.XP)
        {
            Repairs++;
        }
        Stat->Level = ClampedLevel;
        Stat->XP = ClampedXP;
    }

    if (Repairs > 0)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Attribute import repaired %d rows (clamped/deduped)."), Repairs);
    }
    return Repairs;
}

void UAstrawildAttributeComponent::StartSkillCooldown(const EAstrawildPlayerSkillId Skill)
{
    const float Cooldown = GetSkillCooldown(Skill);
    if (Cooldown > 0.0f)
    {
        SkillCooldowns.Add(Skill, Cooldown);
    }
}
