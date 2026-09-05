#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildAttributeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildAttributeLevelUp,
    EAstrawildAttributeType, Attribute, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildPlayerSkillExecuted,
    EAstrawildPlayerSkillId, Skill, bool, bSuccess);

/**
 * GDP-3 — player growth: five attributes fed by playing, seven active skills
 * unlocked by attribute milestones.
 *
 * XP sources (wired at the existing gameplay sites — see the cpp notes):
 *   Might    — landing melee hits, taking real risks (heavy hits taken)
 *   Vigor    — surviving damage, exploring new zones
 *   Agility  — dodges, sprint distance milestones
 *   Instinct — captures, feeding, scanning observations
 *   Craft    — crafting items, building, harvesting nodes
 *
 * Everything is server-authoritative and additive to save schema v5
 * (FAstrawildAttributeSaveData array — absent in old saves = fresh 1/0 states).
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildAttributeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildAttributeComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Attribute")
    FAstrawildAttributeLevelUp OnAttributeLevelUp;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Attribute")
    FAstrawildPlayerSkillExecuted OnPlayerSkillExecuted;

    // ------------------------------------------------------------------
    // XP + levels
    // ------------------------------------------------------------------

    /** Server-side XP grant (clamped, leveled, broadcast). Client calls are ignored. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Attribute")
    void AddAttributeXP(EAstrawildAttributeType Attribute, float Amount);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute")
    int32 GetLevel(EAstrawildAttributeType Attribute) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute")
    float GetXP(EAstrawildAttributeType Attribute) const;

    /** XP needed for the next level (100 * current level; 0 at cap). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute")
    float GetXPToNextLevel(EAstrawildAttributeType Attribute) const;

    // ------------------------------------------------------------------
    // Passive bonus queries (consumed by the existing systems)
    // ------------------------------------------------------------------

    /** Melee damage multiplier (1 + 4% per Might level above 1). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Bonus")
    float GetMeleeDamageMultiplier() const;

    /** Max health multiplier (1 + 5% per Vigor level above 1). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Bonus")
    float GetMaxHealthMultiplier() const;

    /** Stamina regen multiplier (1 + 4% per Agility level above 1). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Bonus")
    float GetStaminaRegenMultiplier() const;

    /** Walk/sprint speed multiplier (1 + 2% per Agility level above 1). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Bonus")
    float GetMoveSpeedMultiplier() const;

    /** Flat capture-chance bonus (0.015 per Instinct level above 1). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Bonus")
    float GetCaptureChanceBonus() const;

    /** Crafting speed multiplier (1 + 4% per Craft level above 1). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Bonus")
    float GetCraftSpeedMultiplier() const;

    /** Masterwork: fractional chance to refund craft inputs (15% when unlocked). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Bonus")
    float GetMasterworkRefundChance() const;

    // ------------------------------------------------------------------
    // Active skills
    // ------------------------------------------------------------------

    /** Skills currently unlocked (level milestones reached). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Skill")
    TArray<EAstrawildPlayerSkillId> GetUnlockedSkills() const;

    /** Static unlock rule — public for tests. */
    static bool IsSkillUnlockedByAttributes(EAstrawildPlayerSkillId Skill,
        int32 MightLevel, int32 VigorLevel, int32 AgilityLevel, int32 InstinctLevel, int32 CraftLevel);

    /** Static cooldown table (seconds) — public for tests. */
    static float GetSkillCooldown(EAstrawildPlayerSkillId Skill);

    /** Cooldown remaining for a skill (0 = ready). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Skill")
    float GetSkillCooldownRemaining(EAstrawildPlayerSkillId Skill) const;

    /** Server-side: puts a skill on cooldown (its static table value). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Attribute|Skill")
    void StartSkillCooldown(EAstrawildPlayerSkillId Skill);

    /**
     * Deterministic "best skill for right now" — the single Y-key cast:
     * SecondWind when hurt, Whirlwind when swarmed, PowerStrike in melee,
     * HuntersFocus before a capture window, Dash as mobility filler.
     *
     * DP-4 CONTRACT: when the player bound at least one loadout slot the
     * ladder considers ONLY the bound skills (build identity); an all-empty
     * loadout (fresh component / pre-DP-4 saves) considers every unlocked
     * skill exactly as before DP-4 — the zero-regression default.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Skill")
    EAstrawildPlayerSkillId PickBestReadySkill(float HealthFraction, int32 NearbyEnemies,
        bool bEnemyInMelee, bool bMoving, bool bWeakenedPreyNear = false) const;

    // ------------------------------------------------------------------
    // DP-4: player skill loadout (build identity)
    // ------------------------------------------------------------------

    /**
     * Bind a skill to loadout slot 0-2 (a non-empty loadout narrows the
     * smart-cast ladder to the bound skills). Validates: slot bounds, the
     * skill is unlocked by the CURRENT attribute milestones, and no duplicate
     * binding (a skill already occupying ANY slot is rejected — clear that
     * slot first). Rebinding a slot replaces its previous occupant. Masterwork
     * is a pure passive (never on the ladder), so binding it only reserves a
     * slot. Server-authoritative when owned; ownerless components (tests)
     * pass through — the same rule as AddAttributeXP.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Attribute|Skill")
    bool BindSkillToSlot(int32 Slot, EAstrawildPlayerSkillId Skill);

    /** Empty a loadout slot (0-2; out-of-bounds is a safe no-op). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Attribute|Skill")
    void ClearSlot(int32 Slot);

    /** True when the skill occupies any loadout slot. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Skill")
    bool IsSkillBound(EAstrawildPlayerSkillId Skill) const;

    /**
     * The 3-slot loadout (index = slot; None = empty slot). All-None means
     * no loadout — the smart-cast ladder then considers every unlocked skill
     * exactly as before DP-4 (fresh components, pre-DP-4 saves).
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Skill")
    TArray<EAstrawildPlayerSkillId> GetBoundSkills() const;

    // ------------------------------------------------------------------
    // Save/load (schema v5 additive)
    // ------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Attribute|Save")
    TArray<FAstrawildAttributeSaveData> ToSaveData() const;

    /** Import with sanitize (clamps + drops duplicates); returns repaired count. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Attribute|Save")
    int32 ImportFromSaveData(const TArray<FAstrawildAttributeSaveData>& Data);

    /** Server-side skill cooldown tick (called by the owning character's Tick). */
    void TickCooldowns(float DeltaSeconds);

protected:
    static constexpr int32 MaxAttributeLevel = 10;
    static constexpr float BaseXPPerLevel = 100.0f;

    /** DP-4: loadout slot count — the fixed 3-slot build-identity surface. */
    static constexpr int32 SkillSlotCount = 3;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Attribute")
    FAstrawildAttributeStat Might;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Attribute")
    FAstrawildAttributeStat Vigor;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Attribute")
    FAstrawildAttributeStat Agility;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Attribute")
    FAstrawildAttributeStat Instinct;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Attribute")
    FAstrawildAttributeStat Craft;

    /** Live skill cooldowns (seconds remaining) — transient combat state. */
    TMap<EAstrawildPlayerSkillId, float> SkillCooldowns;

    /**
     * DP-4: the player-chosen loadout — fixed 3 entries, None = empty slot.
     * Serialized with the attribute payload (ToSaveData/ImportFromSaveData).
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Attribute")
    TArray<EAstrawildPlayerSkillId> BoundSkills;

private:
    FAstrawildAttributeStat* GetStat(EAstrawildAttributeType Attribute);
    const FAstrawildAttributeStat* GetStat(EAstrawildAttributeType Attribute) const;
};
