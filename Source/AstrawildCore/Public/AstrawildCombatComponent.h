#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildCombatComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildAttackExecuted, bool, bWasHeavy, float, DamageDealt);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildDodgeStateChanged, bool, bIsDodging, float, RemainingInvulnerabilitySeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildBlockingChanged, bool, bIsBlocking);
// Batch 3 — Item B: player stagger (heavy hits briefly stop movement input effect).
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildStaggerStateChanged, bool, bIsStaggered, float, RemainingSeconds);

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

    /** Batch 3 — Item B: fired when the player enters/leaves stagger. */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Combat")
    FAstrawildStaggerStateChanged OnStaggerStateChanged;

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

    // --- Ranged weapon tunables (final production run — PHASE 12 Pulse Lance) ---

    /** Cooldown between projectile shots (ranged weapons ignore the melee stamina economy). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Ranged", meta=(ClampMin="0.05"))
    float RangedAttackCooldown = 0.35f;

    /** Forward offset from the player capsule where the bolt spawns (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Ranged", meta=(ClampMin="0.0"))
    float ProjectileSpawnOffset = 80.0f;

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
    /** Unarmed block mitigation — a shield overrides this with its own value (wave 3). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Block", meta=(ClampMin="0.0", ClampMax="0.9"))
    float UnarmedBlockMitigation = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Block", meta=(ClampMin="0.0", ClampMax="0.5"))
    float BlockSpeedMultiplier = 0.45f;

    // --- Armor tunables (Batch 3 — Item C) ---
    /**
     * Diminishing-returns constant for the armor formula:
     * ArmorFraction = ArmorRating / (ArmorRating + ArmorConstantK), clamped to 0..ArmorMaxFraction.
     * K=100 means rating 100 → exactly 50% reduction (before the clamp).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Armor", meta=(ClampMin="1.0"))
    float ArmorConstantK = 100.0f;

    /** Hard cap on armor damage reduction — damage is never fully nullified (design sanity). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Armor", meta=(ClampMin="0.0", ClampMax="0.8"))
    float ArmorMaxFraction = 0.6f;

    // --- Stagger tunables (Batch 3 — Item B) ---
    /** Incoming hits at or above this (post-mitigation) stagger the player. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Stagger", meta=(ClampMin="0.0"))
    float StaggerDamageThreshold = 35.0f;

    /** How long the player staggers when the threshold triggers. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat|Stagger", meta=(ClampMin="0.0"))
    float PlayerStaggerSeconds = 0.6f;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // --- Client-side intent entry points (called by input) ---
    void RequestLightAttack();
    void RequestHeavyAttack();
    void RequestRangedAttack();
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

    /** Effective block mitigation: shield value when a shield is equipped, unarmed baseline otherwise (wave 3). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat|Block")
    float GetEffectiveBlockMitigation() const;

    // --- Batch 3 — Item A: element-driven status effects ---

    /**
     * Pure factory: maps an element to its status effect (Ember→Burn DoT,
     * Frost→Chill slow, Flora→Poison DoT, Pulse→Shock hard-slow; others → invalid).
     * One mapping shared by player weapons, Echo attacks and boss hits so the
     * status vocabulary never fragments.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat|Status")
    static FAstrawildStatusEffect MakeElementalStatusEffect(EAstrawildElementType Element, float SourceDamage);

    /** Resolved outgoing element: equipped weapon Element overrides the tunable when set (Item A). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat|Attack")
    EAstrawildElementType GetResolvedAttackElement() const;

    // --- Batch 3 — Item C: armor ---

    /** Pure diminishing-returns formula: Rating / (Rating + K), clamped to 0..MaxFraction. Testable. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat|Armor")
    static float ComputeArmorFraction(float ArmorRating, float K, float MaxFraction);

    /** Current armor damage-reduction fraction from the equipped torso armor (0 when none). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat|Armor")
    float GetEquippedArmorFraction() const;

    // --- Batch 3 — Item B: player stagger ---

    /** True while the player is staggered (movement zeroed by the character). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat|Stagger")
    bool IsStaggering() const { return StaggerRemainingSeconds > 0.0f; }

    /** Server-side stagger entry point (clamped to a sane maximum). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Combat|Stagger")
    void ApplyStagger(float Seconds);

    /** Attack bonus granted by the equipped weapon (wave 3). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat|Attack")
    float GetEquippedWeaponAttackPower() const;

    /** Resolved outgoing damage for an attack: base tunable + equipped weapon bonus (wave 3). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat|Attack")
    float GetOutgoingAttackDamage(bool bHeavy) const;

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
    double LastRangedAttackTime = -BIG_NUMBER;
    double LastDodgeTime = -BIG_NUMBER;
    float DodgeInvulnerabilityRemaining = 0.0f;
    // Batch 3 — Item B: server-side stagger countdown (client feedback via OnStaggerStateChanged).
    float StaggerRemainingSeconds = 0.0f;

    UFUNCTION(Server, Reliable)
    void ServerLightAttack();

    /** Final production run: ranged weapon shot (validated + ammo-gated on the server). */
    UFUNCTION(Server, Reliable)
    void ServerRangedAttack();

    bool ExecuteRangedAttack();

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
