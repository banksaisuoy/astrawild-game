#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildSurvivalComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAstrawildPlayerDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildSurvivalStatsChanged, float, Health, float, Stamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildStatusEffectApplied, FName, StatusId);
// Batch 3 — Item A: fired when a status effect expires so speed can refresh.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildStatusEffectRemoved, FName, StatusId);
// Batch 4 — M-2a: fired once when stamina hits the floor while sprint-draining,
// so the player character can drop out of sprint speed.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAstrawildSprintExhausted);

/**
 * Player survival vitals — server-authoritative (directive §11/§28).
 * Hunger/thirst decay slowly enough to support exploration instead of menu-spam:
 * ~20 real minutes from full to empty at default rates.
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildSurvivalComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildSurvivalComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Survival")
    FAstrawildPlayerDied OnDied;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Survival")
    FAstrawildSurvivalStatsChanged OnStatsChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Survival")
    FAstrawildStatusEffectApplied OnStatusEffectApplied;

    /** Batch 3 — Item A: fired when a status effect expires (speed refresh trigger). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Survival")
    FAstrawildStatusEffectRemoved OnStatusEffectRemoved;

    /** Batch 4 — M-2a: stamina hit the floor while sprinting (drop out of sprint speed). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Survival")
    FAstrawildSprintExhausted OnSprintExhausted;

    // --- Tunables (migrate to data asset later) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Rates", meta=(ClampMin="0.0"))
    float HungerDecayPerSecond = 0.083f;

    /**
     * Batch 4 — L-2: previously 0.14/s (~12 minutes full-to-empty), contradicting the
     * documented ~20-minute window above. 100 / (20 × 60) ≈ 0.0833/s → ~20 minutes.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Rates", meta=(ClampMin="0.0"))
    float ThirstDecayPerSecond = 0.0833f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Rates", meta=(ClampMin="0.0"))
    float StaminaRegenPerSecond = 14.0f;

    /**
     * Batch 4 — M-2a: stamina drained per second while sprint-drain is active and
     * the owner is actually moving. 100 stamina / 7 per second ≈ 14 s of sprinting.
     * While draining, passive regen is suspended (regen 14/s would otherwise
     * out-pace the drain and make sprinting free).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Rates", meta=(ClampMin="0.0"))
    float SprintStaminaDrainPerSecond = 7.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Rates", meta=(ClampMin="0.0"))
    float StarvationHealthDamagePerSecond = 1.5f;

    /** Below this ambient temperature the player suffers cold damage. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Temperature")
    float ColdThresholdCelsius = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Temperature")
    float HeatThresholdCelsius = 36.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Temperature", meta=(ClampMin="0.0"))
    float ExposureHealthDamagePerSecond = 1.0f;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    const FAstrawildSurvivalStats& GetStats() const { return Stats; }

    /** Production V2 (Master Plan §6): party-aura restore hooks (server-side). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void RestoreStamina(float Amount);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void RestoreHealth(float Amount);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    float GetHealthFraction() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    float GetStaminaFraction() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    bool IsDead() const { return Stats.bIsDead; }

    /** Server-side damage entry point. Returns applied damage. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    float ApplyDamage(float DamageAmount);

    /** Consume food/drink/heal values from an item (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void ApplyConsumption(float FoodValue, float WaterValue, float HealValue);

    /** Spend stamina for actions (sprint/dodge/attacks). Server validated. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    bool TryConsumeStamina(float Amount);

    /**
     * Batch 4 — M-2a: mark sprint-drain as active/inactive (server-side state; the
     * drain itself only ticks when the owner is actually moving). Call from
     * StartSprint/StopSprint — including the exhaustion path.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void SetSprintDrainActive(bool bActive);

    /** Rest point: restore everything (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void FullRestore();

    /**
     * Audit H-1: restore a saved vitals snapshot (server). Previously LoadWorld called
     * FullRestore(), silently discarding the saved hunger/thirst/health every load.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void SetStatsForRestore(const FAstrawildSurvivalStats& InStats);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void AddStatusEffect(const FAstrawildStatusEffect& Effect);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    bool HasStatusEffect(FName StatusId) const;

    /**
     * Batch 3 — Item A: combined movement multiplier from every active speed-affecting
     * status (Chill 0.5, Shock 0.3, ...). 1.0 when nothing slows the player.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    float GetStatusSpeedMultiplier() const;

    /** Debug/cheat. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void SetGodMode(bool bEnabled);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    bool IsGodMode() const { return bGodMode; }

    /**
     * GDP-3 (public — the save load path calls it after importing attributes):
     * recompute max health from the owner's Vigor level (server-side).
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void RefreshVigorMaxHealth();

protected:
    virtual void BeginPlay() override;

    // --- GDP-3: attribute-driven vitals ---

    /** Vigor level-up feed (max health refresh). */
    UFUNCTION()
    void HandleAttributeLevelUp(EAstrawildAttributeType Attribute, int32 NewLevel);

    /** Pre-Vigor base max health (tunable; the level multiplier scales this). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Survival", meta=(ClampMin="1.0"))
    float BaseMaxHealth = 100.0f;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(ReplicatedUsing=OnRep_Stats)
    FAstrawildSurvivalStats Stats;

    UPROPERTY(Replicated)
    TArray<FAstrawildStatusEffect> StatusEffects;

    bool bGodMode = false;

    /** Batch 4 — M-2a: sprint-drain request flag (server-side only, not replicated). */
    bool bSprintDrainActive = false;

    UFUNCTION()
    void OnRep_Stats();

    void UpdateTemperature();
    void ApplyStatusTicks(float DeltaTime);
    void Die();
    /** Batch 4 — M-2a: drain only while the owner actually moves (holding sprint
     *  while standing still is free). */
    bool IsOwnerMoving() const;
    class UAstrawildWeatherSubsystem* GetWeatherSubsystem() const;

    /**
     * DP-7 (world depth): thermal offset of the ambient hazard of the zone the
     * owner currently stands in (pure zone-table lookup; 0 outside the world or
     * in hazard-free zones). Layered ON TOP of the global weather offset.
     */
    float GetZoneHazardTemperatureOffset() const;

    /**
     * DP-7: passive stamina-regen penalty of the owner's current zone (ash
     * lung; 0 for every non-respiratory hazard). Consumed by the regen branch
     * of the authority tick, clamped so the net regen never goes negative.
     */
    float GetZoneHazardStaminaRegenPenalty() const;
    // Final production run (PHASE 12): advanced-equipment integration helpers.
    float GetEquippedInsulation() const;

    /** Production V2: split thermal bands — cold-side insulation. */
    float GetEquippedColdInsulation() const;

    /** Production V2: split thermal bands — heat-side insulation. */
    float GetEquippedHeatInsulation() const;
    float GetExosuitStaminaRegenBonus() const;
};
