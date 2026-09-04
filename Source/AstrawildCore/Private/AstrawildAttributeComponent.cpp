#include "AstrawildAttributeComponent.h"

#include "AstrawildLog.h"
#include "GameFramework/Pawn.h"

UAstrawildAttributeComponent::UAstrawildAttributeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
    // DP-4: the loadout is born as three empty slots — all-None keeps the
    // smart-cast ladder on its legacy all-unlocked behavior.
    BoundSkills.Init(EAstrawildPlayerSkillId::None, SkillSlotCount);
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
    const int32 NearbyEnemies, const bool bEnemyInMelee, const bool bMoving,
    const bool bWeakenedPreyNear) const
{
    // DP-4: a player-chosen loadout narrows the ladder to the bound skills
    // (build identity); an all-empty loadout (fresh component / pre-DP-4
    // saves) considers every unlocked skill exactly as before — the
    // zero-regression default (see the header contract).
    const bool bHasLoadout = BoundSkills.ContainsByPredicate(
        [](const EAstrawildPlayerSkillId Skill) { return Skill != EAstrawildPlayerSkillId::None; });
    auto Ready = [this, bHasLoadout](const EAstrawildPlayerSkillId Skill)
    {
        return (!bHasLoadout || BoundSkills.Contains(Skill)) &&
            IsSkillUnlockedByAttributes(Skill, Might.Level, Vigor.Level, Agility.Level,
                Instinct.Level, Craft.Level) && GetSkillCooldownRemaining(Skill) <= 0.0f;
    };

    // Deterministic priority ladder — the smart-cast reads the battlefield.
    if (HealthFraction < 0.35f && Ready(EAstrawildPlayerSkillId::SecondWind))
    {
        return EAstrawildPlayerSkillId::SecondWind;
    }
    // FCR-1-b fix (M-b4): a WEAKENED hostile in capture reach outranks everything
    // below — the old condition (no enemies at all) made Hunter's Focus pickable
    // only when there was nothing to capture, exactly backwards for a pre-capture
    // buff (+25% capture window).
    if (bWeakenedPreyNear && Ready(EAstrawildPlayerSkillId::HuntersFocus))
    {
        return EAstrawildPlayerSkillId::HuntersFocus;
    }
    if (NearbyEnemies >= 3 && Ready(EAstrawildPlayerSkillId::Whirlwind))
    {
        return EAstrawildPlayerSkillId::Whirlwind;
    }
    if (bEnemyInMelee && Ready(EAstrawildPlayerSkillId::PowerStrike))
    {
        return EAstrawildPlayerSkillId::PowerStrike;
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
    // DP-4: the loadout rides on the FIRST row (Might) of the payload —
    // pre-DP-4 readers ignore the extra field, pre-DP-4 saves have none.
    if (!Data.IsEmpty() && BoundSkills.Num() == SkillSlotCount)
    {
        Data[0].BoundSkills = BoundSkills;
    }
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
            // FCR-1-b fix (L-b6): a corrupt/unknown attribute type IS a repair —
            // the row is dropped, so it must be counted (the old bare continue
            // undercounted the repair report).
            ++Repairs;
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

    // DP-4: the loadout rides on the first row carrying a non-empty
    // BoundSkills array (ToSaveData writes it on the Might row; first-seen
    // wins, like the attribute rows themselves). Sanitized into the 3-slot
    // shape: valid unlocked ids keep their slot, duplicates / overflow /
    // no-longer-unlocked entries drop and count as repairs (None is the
    // empty-slot encoding, not a repair). A payload without the field
    // (pre-DP-4 saves) resets the loadout to all-empty — the legacy
    // smart-cast contract.
    bool bLoadoutFound = false;
    for (const FAstrawildAttributeSaveData& Row : Data)
    {
        if (Row.BoundSkills.IsEmpty())
        {
            continue;
        }
        bLoadoutFound = true;
        TArray<EAstrawildPlayerSkillId> Sanitized;
        Sanitized.Init(EAstrawildPlayerSkillId::None, SkillSlotCount);
        TSet<EAstrawildPlayerSkillId> SeenSkills;
        for (int32 SlotIndex = 0; SlotIndex < Row.BoundSkills.Num(); ++SlotIndex)
        {
            const EAstrawildPlayerSkillId RowSkill = Row.BoundSkills[SlotIndex];
            if (RowSkill == EAstrawildPlayerSkillId::None)
            {
                continue;
            }
            if (SlotIndex >= SkillSlotCount || SeenSkills.Contains(RowSkill) ||
                !IsSkillUnlockedByAttributes(RowSkill, Might.Level, Vigor.Level, Agility.Level,
                    Instinct.Level, Craft.Level))
            {
                ++Repairs; // Duplicate, overflow, or no-longer-unlocked binding.
                continue;
            }
            SeenSkills.Add(RowSkill);
            Sanitized[SlotIndex] = RowSkill;
        }
        BoundSkills = Sanitized;
        break;
    }
    if (!bLoadoutFound)
    {
        BoundSkills.Init(EAstrawildPlayerSkillId::None, SkillSlotCount);
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

bool UAstrawildAttributeComponent::BindSkillToSlot(const int32 Slot, const EAstrawildPlayerSkillId Skill)
{
    // Server-authoritative when owned (ownerless test components pass
    // through) — the same rule as AddAttributeXP.
    if (GetOwner() && GetOwnerRole() != ROLE_Authority)
    {
        return false;
    }
    if (!BoundSkills.IsValidIndex(Slot))
    {
        return false; // Slot bounds: 0-2 only.
    }
    // The skill must be unlocked by the CURRENT milestones — this also
    // rejects None and any out-of-range id (they never pass a milestone).
    if (!IsSkillUnlockedByAttributes(Skill, Might.Level, Vigor.Level, Agility.Level,
        Instinct.Level, Craft.Level))
    {
        return false;
    }
    if (BoundSkills.Contains(Skill))
    {
        return false; // No duplicate bindings — clear the other slot first.
    }

    BoundSkills[Slot] = Skill; // Rebinding replaces the previous occupant.
    UE_LOG(LogAstrawild, Log, TEXT("Skill loadout: slot %d bound to %s (%s)."),
        Slot, *UEnum::GetValueAsString(Skill), *GetNameSafe(GetOwner()));
    return true;
}

void UAstrawildAttributeComponent::ClearSlot(const int32 Slot)
{
    // Server-authoritative when owned (mirrors BindSkillToSlot).
    if (GetOwner() && GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    if (BoundSkills.IsValidIndex(Slot))
    {
        BoundSkills[Slot] = EAstrawildPlayerSkillId::None;
    }
}

bool UAstrawildAttributeComponent::IsSkillBound(const EAstrawildPlayerSkillId Skill) const
{
    return Skill != EAstrawildPlayerSkillId::None && BoundSkills.Contains(Skill);
}

TArray<EAstrawildPlayerSkillId> UAstrawildAttributeComponent::GetBoundSkills() const
{
    return BoundSkills;
}
