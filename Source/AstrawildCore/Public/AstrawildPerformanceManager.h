#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildPerformanceManager.generated.h"

/**
 * SCP Phase 13 — dynamic performance enforcement (directive [3] Phase 13.1).
 *
 * Watches the frame rate on a 5-second cadence and steps scalability up/down:
 *  - sustained < 50 FPS for 3 samples -> drop one tier (view distance, foliage
 *    density, shadow quality);
 *  - sustained >= 58 FPS for 12 samples -> climb back one tier.
 *
 * Tiers write engine console variables (r.ViewDistanceScale,
 * foliage.SpawnDensityScale, sg.ShadowQuality) through IConsoleManager — the
 * same knobs the engine's own scalability system uses, so every material and
 * every foliage responder follows along. All changes log + report through the
 * error reporter trail for Standalone diagnostics.
 */
UENUM(BlueprintType)
enum class EAstrawildPerformanceTier : uint8
{
    Low UMETA(DisplayName="Low"),
    Medium UMETA(DisplayName="Medium"),
    High UMETA(DisplayName="High")
};

UCLASS()
class ASTRAWILDCORE_API UAstrawildPerformanceManager : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildPerformanceManager();

    // FTickableGameObject interface.
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }
    virtual bool IsTickable() const override;

    // --- Static contracts (automation-tested) ---

    /** The console-variable value a tier maps to (pure). */
    static float GetViewDistanceScaleForTier(int32 Tier);
    static float GetFoliageDensityScaleForTier(int32 Tier);
    static int32 GetShadowQualityForTier(int32 Tier);

    /** Should the manager step down, given the sample history? (pure) */
    static bool ShouldStepDown(int32 ConsecutiveSlowSamples);

    /** Should the manager step up, given the sample history? (pure) */
    static bool ShouldStepUp(int32 ConsecutiveFastSamples);

    // --- Runtime state ---

    /** Current tier (0 Low, 1 Medium, 2 High). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Perf")
    int32 GetCurrentTier() const { return CurrentTier; }

    /** Apply a tier now (logs + console-variable writes). */
    void ApplyTier(int32 NewTier);

private:
    /** Sample cadence. */
    static constexpr float SampleIntervalSeconds = 5.0f;

    /** Slow sample threshold (drop tier below this FPS). */
    static constexpr float SlowFpsThreshold = 50.0f;

    /** Fast sample threshold (climb tier at/above this FPS). */
    static constexpr float FastFpsThreshold = 58.0f;

    int32 CurrentTier = 2; // start optimistic; steps down within 15s if wrong.

    int32 ConsecutiveSlowSamples = 0;
    int32 ConsecutiveFastSamples = 0;

    float SampleAccumulator = 0.0f;
    int32 FramesInWindow = 0;

    void ProcessSample(float Fps);
};
