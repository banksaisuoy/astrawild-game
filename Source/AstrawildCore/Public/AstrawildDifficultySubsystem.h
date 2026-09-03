#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildDifficultySubsystem.generated.h"

/**
 * SCP Phase 3 — Dynamic Difficulty Adjustment (directive [3] Phase 3.2).
 *
 * A skill band derived from the player's actual performance shifts hostile
 * strength and resource generosity:
 *  - Struggling (band 0): hostiles x0.85 HP/damage, resources x1.15
 *  - Standard  (band 1): everything x1.0
 *  - Thriving  (band 2): hostiles x1.15, resources x0.9
 *
 * Metrics come from the event bus (hostile defeats, captures) plus player
 * deaths. The band never swings mid-fight: it is recomputed on a 30-second
 * cadence and requires a lead of at least two metric points.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildDifficultySubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildDifficultySubsystem();

    // FTickableGameObject interface.
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }
    virtual bool IsTickable() const override;

    // --- Static contracts (automation-tested) ---

    /**
     * Skill metric -> band 0/1/2. Deaths pull down hard, hostile defeats and
     * captures push up; the neutral band spans [-1, +1] so the system does not
     * flap around zero.
     */
    static int32 ComputeSkillBand(int32 HostileDefeats, int32 Captures, int32 Deaths);

    /** Hostile HP/damage multiplier for a band. */
    static float GetHostileStrengthMultiplier(int32 Band);

    /** Resource yield multiplier for a band. */
    static float GetResourceYieldMultiplier(int32 Band);

    // --- Runtime state ---

    /** Current band (0 struggling / 1 standard / 2 thriving). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|DDA")
    int32 GetSkillBand() const { return SkillBand; }

    /** Live multipliers (spawner + resource node query these). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|DDA")
    float GetHostileStrengthScale() const { return GetHostileStrengthMultiplier(SkillBand); }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|DDA")
    float GetResourceYieldScale() const { return GetResourceYieldMultiplier(SkillBand); }

    /** Event bus ingestion points (server). */
    void NotifyHostileDefeated() { ++HostileDefeatCount; }
    void NotifyCapture() { ++CaptureCount; }
    void NotifyPlayerDeath() { ++DeathCount; }

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    /** Event bus ingestion (hostile defeats / captures / party losses). */
    UFUNCTION()
    void HandleGameplayEvent(const FAstrawildGameplayEvent& Event);

private:
    /** Band refresh cadence — no mid-fight swings. */
    static constexpr float BandRefreshSeconds = 30.0f;

    int32 HostileDefeatCount = 0;
    int32 CaptureCount = 0;
    int32 DeathCount = 0;
    int32 SkillBand = 1;

    float RefreshAccumulator = 0.0f;

    void RefreshBand();
};
