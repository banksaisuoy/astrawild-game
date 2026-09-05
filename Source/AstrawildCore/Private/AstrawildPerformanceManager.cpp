#include "AstrawildPerformanceManager.h"

#include "AstrawildErrorReporter.h"
#include "AstrawildLog.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"

UAstrawildPerformanceManager::UAstrawildPerformanceManager()
{
}

TStatId UAstrawildPerformanceManager::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildPerformanceManager, STATGROUP_Tickables);
}

bool UAstrawildPerformanceManager::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld();
}

float UAstrawildPerformanceManager::GetViewDistanceScaleForTier(int32 Tier)
{
    switch (FMath::Clamp(Tier, 0, 2))
    {
    case 2:
        return 1.0f;
    case 1:
        return 0.7f;
    default:
        return 0.4f;
    }
}

float UAstrawildPerformanceManager::GetFoliageDensityScaleForTier(int32 Tier)
{
    switch (FMath::Clamp(Tier, 0, 2))
    {
    case 2:
        return 1.0f;
    case 1:
        return 0.6f;
    default:
        return 0.3f;
    }
}

int32 UAstrawildPerformanceManager::GetShadowQualityForTier(int32 Tier)
{
    switch (FMath::Clamp(Tier, 0, 2))
    {
    case 2:
        return 3; // sg.ShadowQuality 3 = dynamic all
    case 1:
        return 2;
    default:
        return 1;
    }
}

bool UAstrawildPerformanceManager::ShouldStepDown(int32 ConsecutiveSlowSamples)
{
    // Three slow samples (15s) = the frame rate problem is real, not a hitch.
    return ConsecutiveSlowSamples >= 3;
}

bool UAstrawildPerformanceManager::ShouldStepUp(int32 ConsecutiveFastSamples)
{
    // Climbing back demands a full minute of headroom — no flapping.
    return ConsecutiveFastSamples >= 12;
}

void UAstrawildPerformanceManager::ApplyTier(int32 NewTier)
{
    const int32 ClampedTier = FMath::Clamp(NewTier, 0, 2);
    if (ClampedTier == CurrentTier)
    {
        return;
    }

    CurrentTier = ClampedTier;

    // Write the engine knobs — identical to the scalability system's variables
    // so every responder (foliage, shadows, LODs) follows immediately.
    // FCR-1-d fix (M-d10): variables the USER pinned (console / command line /
    // higher priority than a game setting) are never stomped — the old blind
    // write overrode manual tuning while still logging "tier -> X" (state desync),
    // and a manual-Low user could be RAISED by a "step down".
    IConsoleManager& Console = IConsoleManager::Get();

    auto UserPinned = [](IConsoleVariable* Var) -> bool
    {
        return Var && (Var->GetFlags() & ECVF_SetByMask) > ECVF_SetByGameSetting;
    };

    if (IConsoleVariable* ViewDistance = Console.FindConsoleVariable(TEXT("r.ViewDistanceScale")))
    {
        if (!UserPinned(ViewDistance))
        {
            ViewDistance->Set(GetViewDistanceScaleForTier(CurrentTier), ECVF_SetByGameSetting);
        }
    }
    if (IConsoleVariable* Foliage = Console.FindConsoleVariable(TEXT("foliage.SpawnDensityScale")))
    {
        if (!UserPinned(Foliage))
        {
            Foliage->Set(GetFoliageDensityScaleForTier(CurrentTier), ECVF_SetByGameSetting);
        }
    }
    if (IConsoleVariable* Shadows = Console.FindConsoleVariable(TEXT("sg.ShadowQuality")))
    {
        if (!UserPinned(Shadows))
        {
            Shadows->Set(GetShadowQualityForTier(CurrentTier), ECVF_SetByGameSetting);
        }
    }

    const TCHAR* TierNames[] = { TEXT("Low"), TEXT("Medium"), TEXT("High") };
    UE_LOG(LogAstrawild, Log, TEXT("Performance: tier -> %s (view %.2f, foliage %.2f, shadows %d)"),
        TierNames[CurrentTier],
        GetViewDistanceScaleForTier(CurrentTier),
        GetFoliageDensityScaleForTier(CurrentTier),
        GetShadowQualityForTier(CurrentTier));

    UAstrawildErrorReporterLibrary::ReportInfo(TEXT("Performance"),
        FString::Printf(TEXT("Scalability tier adjusted to %s"), TierNames[CurrentTier]));
}

void UAstrawildPerformanceManager::ProcessSample(float Fps)
{
    if (Fps <= 0.0f)
    {
        return;
    }

    if (Fps < SlowFpsThreshold)
    {
        ++ConsecutiveSlowSamples;
        ConsecutiveFastSamples = 0;
    }
    else if (Fps >= FastFpsThreshold)
    {
        ++ConsecutiveFastSamples;
        ConsecutiveSlowSamples = 0;
    }
    else
    {
        // Comfortable band: reset both ladders (a steady 55 is fine as-is).
        ConsecutiveSlowSamples = 0;
        ConsecutiveFastSamples = 0;
    }

    if (ShouldStepDown(ConsecutiveSlowSamples))
    {
        ConsecutiveSlowSamples = 0;
        ApplyTier(CurrentTier - 1);
    }
    else if (ShouldStepUp(ConsecutiveFastSamples))
    {
        ConsecutiveFastSamples = 0;
        ApplyTier(CurrentTier + 1);
    }
}

void UAstrawildPerformanceManager::Tick(float DeltaTime)
{
    if (!IsTickable())
    {
        return;
    }

    // Count frames inside the sample window; FPS = frames / window seconds —
    // the true average, immune to single-frame spikes in either direction.
    ++FramesInWindow;
    SampleAccumulator += DeltaTime;
    if (SampleAccumulator < SampleIntervalSeconds)
    {
        return;
    }

    const float AverageFps = static_cast<float>(FramesInWindow) / FMath::Max(0.1f, SampleAccumulator);
    SampleAccumulator = 0.0f;
    FramesInWindow = 0;

    ProcessSample(AverageFps);
}
