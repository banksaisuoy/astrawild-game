#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoBossCharacter.generated.h"

class UStaticMeshComponent;
class UAstrawildEchoDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildBossPhaseChanged, int32, NewPhase, float, HealthFractionAtTransition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildBossDefeated, class AAstrawildEchoBossCharacter*, Boss);

/**
 * Boss encounter architecture (directive §24): phases with distinct behavior,
 * enrage timer, summon adds, arena awareness — never just extra HP.
 *
 * Boss design (First Dawn's dungeon warden — original creature, no Palworld/ARK DNA):
 *   Phase 1 (100-66%): measured melee pressure — telegraphed charges.
 *   Phase 2 (66-33%): summons 2 gloomfang adds + faster attacks.
 *   Phase 3 (33-0%):  enrage — attack speed doubles, damage +40%.
 *
 * Batch 6 (dungeon/boss hardening — STEP 22 extension):
 *   - Stats derive from the boss species definition (BossDefinitionId was cosmetic
 *     before: HP/damage are now BaseStats × boss scales; weakness/element apply).
 *   - Elemental damage path: player attacks resolve weakness ×1.5 / same-element
 *     resist ×0.75 (same vocabulary as the Echo pipeline) and can afflict the
 *     boss with status effects (bosses shed them at BossStatusDurationMultiplier).
 *   - Defeat publishes Event.HostileDefeated with DefeatEventTargetId so quests
 *     credit the kill (previously OnBossDefeated had no subscribers at all).
 *
 * Server-authoritative: phases resolve on the server; UI reads replicated state.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildEchoBossCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAstrawildEchoBossCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Boss")
    FAstrawildBossPhaseChanged OnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Boss")
    FAstrawildBossDefeated OnBossDefeated;

    // --- Stats (scale above base species — phase design carries difficulty, not raw HP) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="1.0"))
    float MaxHealth = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="0.0"))
    float BaseDamage = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="0.1"))
    float AttackCooldownSeconds = 2.0f;

    /** Seconds until enrage regardless of health (directive §24 enrage). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="10.0"))
    float EnrageTimerSeconds = 180.0f;

    /** Damage multiplier while enraged. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="1.0"))
    float EnrageDamageMultiplier = 1.4f;

    /** Adds summoned at phase 2. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss")
    FName SummonSpeciesId = TEXT("Echo_Gloomfang");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="0"))
    int32 SummonCount = 2;

    // --- Batch 6: definition-driven stats + elemental combat ---

    /** Species definition the boss scales from (set by the dungeon boss room). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss")
    FName BossSpeciesId = TEXT("Echo_Gloomfang");

    /**
     * Event bus TargetId published with Event.HostileDefeated on death. Distinct
     * from the wild species id so quests can require the boss specifically
     * (killing any wild Gloomfang must not complete "defeat the warden").
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss")
    FName DefeatEventTargetId = TEXT("Creature_UnderlightWarden");

    /** Attacking with this element deals ×1.5 (resolved from the species definition). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss")
    EAstrawildElementType WeaknessElement = EAstrawildElementType::Light;

    /** The boss's own element — same-element attacks are resisted (×0.75). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss")
    EAstrawildElementType BossElement = EAstrawildElementType::Ash;

    /** Definition MaxHealth multiplier (bosses tower over their wild kin). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="1.0"))
    float BossHealthScale = 5.0f;

    /** Definition AttackPower multiplier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="1.0"))
    float BossDamageScale = 1.8f;

    /** Bosses shrug off extended CC — incoming status durations scale by this. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss", meta=(ClampMin="0.05", ClampMax="1.0"))
    float BossStatusDurationMultiplier = 0.5f;

    // --- Replicated encounter state ---
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss", Replicated)
    float CurrentHealth = 600.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss", Replicated)
    int32 CurrentPhase = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss", Replicated)
    bool bEnraged = false;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Server-side damage entry with phase transitions. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Boss")
    float ApplyBossDamage(float DamageAmount);

    /**
     * Batch 6: elemental damage entry — resolves the weakness/resist multiplier
     * against the boss's elements, applies damage, then tries to afflict the
     * matching status effect (shared element→status vocabulary).
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Boss")
    float ApplyElementalBossDamage(float DamageAmount, EAstrawildElementType Element);

    /** Batch 6: afflict the boss with a status effect (durations scaled, no stacking). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Boss")
    void ApplyBossStatus(const FAstrawildStatusEffect& Effect);

    /**
     * Batch 6: derive stats from a species definition (BossDefinitionId was cosmetic
     * before — the boss room now feeds the real definition through here).
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Boss")
    void InitializeFromBossDefinition(const UAstrawildEchoDefinition* Definition);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    float GetHealthFraction() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    bool IsDefeated() const { return CurrentHealth <= 0.0f; }

    // --- Batch 6: pure statics (unit-tested — ASTRAWILD.Dungeon.*) ---

    /** Elemental multiplier: weakness ×1.5, same-element resist ×0.75, otherwise ×1. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    static float ComputeBossElementalMultiplier(EAstrawildElementType AttackElement, EAstrawildElementType Weakness, EAstrawildElementType OwnElement);

    /** Target phase for a health fraction (enrage always forces 3). Thresholds 0.66/0.33. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    static int32 ComputePhaseForHealthFraction(float HealthFraction, bool bEnraged);

    /** Attack damage for a phase: enraged ×multiplier, else phase 2 ×1.15. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    static float ComputeBossAttackDamage(float Base, int32 Phase, bool bEnraged, float EnrageMultiplier);

private:
    double LastAttackTime = -BIG_NUMBER;
    float EnrageElapsed = 0.0f;
    bool bPhase2SummonsSpawned = false;
    TArray<TWeakObjectPtr<class AAstrawildEchoCharacter>> Summons;

    /** Walk speed set by the current phase — status slows multiply on top each tick. */
    float PhaseWalkSpeed = 380.0f;

    /** Active status effects (server-side; health replication carries the visible result). */
    TArray<FAstrawildStatusEffect> ActiveStatusEffects;

    void TransitionToPhase(int32 NewPhase);
    void SpawnSummons();
    void ExecuteAttack(float DeltaTime);
    void TickStatusEffects(float DeltaTime);
    float GetStatusSpeedMultiplier() const;
    void RefreshWalkSpeed();
    class AAstrawildPlayerCharacter* FindNearestPlayer() const;
    float GetAttackDamage() const;
};
