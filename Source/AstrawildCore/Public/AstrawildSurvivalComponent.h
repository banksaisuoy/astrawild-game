#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildSurvivalComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAstrawildPlayerDied);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildSurvivalStatsChanged, float, Health, float, Stamina);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildStatusEffectApplied, FName, StatusId);

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

    // --- Tunables (migrate to data asset later) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Rates", meta=(ClampMin="0.0"))
    float HungerDecayPerSecond = 0.083f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Rates", meta=(ClampMin="0.0"))
    float ThirstDecayPerSecond = 0.14f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival|Rates", meta=(ClampMin="0.0"))
    float StaminaRegenPerSecond = 14.0f;

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

    /** Rest point: restore everything (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void FullRestore();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void AddStatusEffect(const FAstrawildStatusEffect& Effect);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    bool HasStatusEffect(FName StatusId) const;

    /** Debug/cheat. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void SetGodMode(bool bEnabled);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    bool IsGodMode() const { return bGodMode; }

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(ReplicatedUsing=OnRep_Stats)
    FAstrawildSurvivalStats Stats;

    UPROPERTY(Replicated)
    TArray<FAstrawildStatusEffect> StatusEffects;

    bool bGodMode = false;

    UFUNCTION()
    void OnRep_Stats();

    void UpdateTemperature();
    void ApplyStatusTicks(float DeltaTime);
    void Die();
    class UAstrawildWeatherSubsystem* GetWeatherSubsystem() const;
};
