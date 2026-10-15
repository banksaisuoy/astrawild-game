#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AstrawildTypes.h"
#include "AstrawildComboSubsystem.generated.h"

class AActor;
class AAstrawildEchoCharacter;
class AAstrawildPlayerCharacter;

/**
 * SCP Phase 6 — Dual-Tech combo reactions (directive [3] Phase 6.3).
 *
 * A player melee mark + a party Echo's ability strike on the SAME target
 * within the combo window resolve a reaction: bonus damage multiplier, an
 * applied status ("hitstop" reads as a hard slow), and a display name for
 * the HUD toast. Twelve authored reactions cover every element x both player
 * attack tiers (Kinetic / Empowered) — the directive's 10-formula minimum,
 * exceeded with the full element matrix.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildComboReaction
{
    GENERATED_BODY()

    /** Display name shown in the HUD toast. */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Combo")
    FString DisplayName;

    /** Bonus damage multiplier applied to the triggering ability hit. */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Combo")
    float DamageMultiplier = 1.0f;

    /** Status id applied to the target (empty = pure damage reaction). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Combo")
    FName StatusId = NAME_None;

    bool IsValid() const { return !DisplayName.IsEmpty() && DamageMultiplier > 1.0f; }
};

UCLASS()
class ASTRAWILDCORE_API UAstrawildComboLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Player attack tiers that can open a combo window. */
    static constexpr float ComboWindowSeconds = 3.0f;

    /** The directive's signature reaction numbers. */
    static constexpr float SteamExplosionMultiplier = 2.5f;
    static constexpr float HitstopSpeedMultiplier = 0.15f;
    static constexpr float HitstopSeconds = 1.5f;

    /**
     * Resolve the reaction for (bPlayerEmpowered, EchoElement).
     * Invalid combos return an empty reaction (IsValid false).
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combo")
    static FAstrawildComboReaction ResolveCombo(bool bPlayerEmpowered, EAstrawildElementType EchoElement);

    /** Number of authored reactions (contract: 12 — the 10-formula directive minimum exceeded). */
    static int32 GetReactionCount();
};

/**
 * World tracker: remembers the player's recent melee marks (target + time +
 * empowered tier) and resolves reactions when a party Echo's ability bolt
 * lands on a marked target.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildComboSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // UWorldSubsystem interface.
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    /** Combat component reports every landed player melee hit (server). */
    void NotifyPlayerMeleeHit(AActor* Target, bool bEmpowered);

    /**
     * Projectile impact query: returns the reaction when the striking echo is
     * a captured party member, the target carries a live mark, and the pair
     * resolves. Marks are consumed on trigger (no double-dips).
     */
    FAstrawildComboReaction TryResolveEchoAbilityCombo(AActor* Target, const AAstrawildEchoCharacter* StrikingEcho);

    /** Last combo display name (HUD toast hook). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combo")
    FString GetLastComboName() const { return LastComboName; }

private:
    struct FComboMark
    {
        TWeakObjectPtr<AActor> Target;
        double MarkTimeSeconds = 0.0;
        bool bEmpowered = false;
    };

    TArray<FComboMark> Marks;

    FString LastComboName;

    void ExpireStaleMarks();
};
