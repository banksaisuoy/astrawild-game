#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildAttackExecuted, bool, bWasHeavy, float, DamageDealt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildDodgeStateChanged, bool, bIsDodging, float, RemainingInvulnerabilitySeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildBlockingChanged, bool, bIsBlocking);

/**
 * Third-person action combat (directive §9): light/heavy attacks, dodge with
 * invulnerability frames, block mitigation, elemental interactions, stamina gating.
 * All damage resolution is server-side (§28); client sends intent via Server_ RPCs.
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildCombatComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildCombatComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Combat")
    FAstrawildAttackExecuted OnAttackExecuted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Combat")
    FAstrawildDodgeStateChanged OnDodgeStateChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Combat")
    FAstrawildBlockingChanged OnBlockingChanged;

    // --- Attack tunables ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Attack", meta=(ClampMin="1.0"))
    float LightAttackDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Attack", meta=(ClampMin="0.0"))
    float LightAttackCooldown = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Attack", meta=(ClampMin="1.0"))
    float HeavyAttackDamage = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Attack", meta=(ClampMin="0.0"))
    float HeavyAttackCooldown = 1.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Attack", meta=(ClampMin="0.0"))
    float AttackRange = 320.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Attack", meta=(ClampMin="0.0"))
    float HeavyAttackStaminaCost = 25.0f;

    /** Player attack element (equipment can override later). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Attack")
    EAstrawildElementType AttackElement = EAstrawildElementType::Ash;

    // --- Dodge tunables ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Dodge", meta=(ClampMin="0.0"))
    float DodgeCooldown = 0.9f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Dodge", meta=(ClampMin="0.0"))
    float DodgeStaminaCost = 22.0f;

    /** Invulnerability window inside the dodge (directive §9 i-frames). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Dodge", meta=(ClampMin="0.0"))
    float DodgeInvulnerabilitySeconds = 0.4f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Dodge", meta=(ClampMin="0.0"))
    float DodgeImpulseStrength = 900.0f;

    // --- Block tunables ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Block", meta=(ClampMin="0.0", ClampMax="0.9"))
    float BlockMitigation = 0.65f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Block", meta=(ClampMin="0.0", ClampMax="0.5"))
    float BlockSpeedMultiplier = 0.45f;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- Client-side intent entry points (called by input) ---
    void RequestLightAttack();
    void RequestHeavyAttack();
    void RequestDodge(const FVector& Direction);
    void RequestSetBlocking(bool bBlocking);

    // --- Server combat state queries ---
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat")
    bool IsBlocking() const { return bIsBlocking; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat")
    bool IsDodging() const { return bDodgeInvulnerabilityRemaining > 0.0f; }

    /** Mitigated damage from an incoming hit while blocking. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat")
    float GetMitigatedIncomingDamage(float RawDamage) const;

    /** Damage the player deals to a target right now (0 if on cooldown/dead). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat")
    bool CanAttack(bool bHeavy) const;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    UPROPERTY(Replicated)
    bool bIsBlocking = false;

    UPROPERTY(Replicated)
    float bReplicatedDodgeTimer = 0.0f;

    double LastLightAttackTime = -BIG_NUMBER;
    double LastHeavyAttackTime = -BIG_NUMBER;
    double LastDodgeTime = -BIG_NUMBER;
    float DodgeInvulnerabilityRemaining = 0.0f;

    UFUNCTION(Server, Reliable)
    void ServerLightAttack();

    UFUNCTION(Server, Reliable)
    void ServerHeavyAttack();

    UFUNCTION(Server, Reliable)
    void ServerDodge(FVector_NetQuantizeNormal Direction);

    UFUNCTION(Server, Reliable)
    void ServerSetBlocking(bool bBlocking);

    bool ExecuteAttack(bool bHeavy);
    void ApplyDodgeImpulse(const FVector& Direction);
    class UAstrawildSurvivalComponent* GetSurvival() const;
};
