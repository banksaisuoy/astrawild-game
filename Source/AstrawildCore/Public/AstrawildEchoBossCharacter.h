#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoBossCharacter.generated.h"

class UStaticMeshComponent;

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

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    float GetHealthFraction() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    bool IsDefeated() const { return CurrentHealth <= 0.0f; }

private:
    double LastAttackTime = -BIG_NUMBER;
    float EnrageElapsed = 0.0f;
    bool bPhase2SummonsSpawned = false;
    TArray<TWeakObjectPtr<class AAstrawildEchoCharacter>> Summons;

    void TransitionToPhase(int32 NewPhase);
    void SpawnSummons();
    void ExecuteAttack(float DeltaTime);
    class AAstrawildPlayerCharacter* FindNearestPlayer() const;
    float GetAttackDamage() const;
};
